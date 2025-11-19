// udp_server.h - UDP command server for remote control and status queries

#include "udp_server.h"
#include "sampler.h"

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define UDP_PORT 12345
#define BUF_SIZE 256
#define CMD_MAX  128   // max length we store for last_cmd

static pthread_t s_thread;
static volatile int s_run = 0;

static void send_str(const char *msg, struct sockaddr_in *cli, int sock)
{
    (void)sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)cli, sizeof(*cli));
}

static void safe_copy_cmd(char dst[CMD_MAX], const char *src)
{
    size_t n = strnlen(src, CMD_MAX - 1);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void* srv(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("UDP socket");
        return NULL;
    }

    // set 500ms recv timeout so we can check s_run without pthread_cancel
    struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 500000; // 500 ms
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
   		 perror("UDP setsockopt(SO_RCVTIMEO)");
	}


    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(UDP_PORT);
    serv.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("UDP bind");
        close(sock);
        return NULL;
    }

    printf("[UDP] Server listening on port %d\n", UDP_PORT);

    char buf[BUF_SIZE];
    char last_cmd[CMD_MAX] = {0};

    while (s_run) {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        ssize_t r = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&cli, &clen);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue; // timeout: re-check s_run
            if (!s_run) break;
            perror("UDP recvfrom");
            continue;
        }
        if (r == 0) continue;

        buf[r] = '\0';
        // strip trailing CR/LF
        while (r > 0 && (buf[r-1] == '\n' || buf[r-1] == '\r')) buf[--r] = '\0';

        // Empty line => repeat last command (if any)
        if (buf[0] == '\0') {
            if (last_cmd[0] == '\0') {
                send_str("Error: No previous command.\n", &cli, sock);
                continue;
            }
            safe_copy_cmd(buf, last_cmd);
        } else {
            safe_copy_cmd(last_cmd, buf);
        }

        if (!strcmp(buf, "help") || !strcmp(buf, "?")) {
            send_str(
                "Commands:\n"
                "  help/?   - show this help\n"
                "  count    - total samples taken\n"
                "  length   - samples collected in the previous second\n"
                "  dips     - dips detected in the previous second\n"
                "  history  - get all samples from previous second\n"
                "  stop     - terminate program\n"
                "  <enter>  - repeat last command\n", &cli, sock);
        }
        else if (!strcmp(buf, "count")) {
            char out[96];
            long long total = sampler_get_total_samples();
            snprintf(out, sizeof(out), "Total samples: %lld\n", total);
            send_str(out, &cli, sock);
        }
        else if (!strcmp(buf, "length")) {
            char out[96];
            int n = sampler_get_history_size();
            snprintf(out, sizeof(out), "Samples/sec: %d\n", n);
            send_str(out, &cli, sock);
        }
        else if (!strcmp(buf, "dips")) {
            char out[96];
            int d = sampler_get_last_second_dips();
            snprintf(out, sizeof(out), "Dips: %d\n", d);
            send_str(out, &cli, sock);
        }
        else if (!strcmp(buf, "history")) {
            int n = 0;
            double *hist = sampler_get_history_copy(&n);
            if (n == 0) {
                send_str("No history data available\n", &cli, sock);
            } else {
                // Send history in chunks (comma-separated, 10 per line)
                char chunk[512];
                size_t pos = 0;  // Changed to size_t to fix the warning
                
                for (int i = 0; i < n; i++) {
                    int written = snprintf(chunk + pos, sizeof(chunk) - pos, "%.3f", hist[i]);
                    pos += written;
                    
                    if (i < n - 1) {
                        if ((i + 1) % 10 == 0) {
                            strncat(chunk, ",\n", sizeof(chunk) - strlen(chunk) - 1);
                            pos += 2;
                        } else {
                            strncat(chunk, ", ", sizeof(chunk) - strlen(chunk) - 1);
                            pos += 2;
                        }
                    }
                    
                    // Send chunk if buffer is getting full or at end
                    if (pos > sizeof(chunk) - 50 || i == n - 1) {
                        send_str(chunk, &cli, sock);
                        pos = 0;
                        chunk[0] = '\0';
                    }
                }
                send_str("\n", &cli, sock); // Final newline
            }
            free(hist);
        }
        else if (!strcmp(buf, "stop")) {
            send_str("Program terminating.\n", &cli, sock);
            // Graceful shutdown of the whole app:
            kill(getpid(), SIGINT);
            break;
        }
        else {
            // Bound the echo of the unknown command to keep out <= 128 bytes safely
            char out[128];
            snprintf(out, sizeof(out), "Error: Unknown command '%.96s'\n", buf);
            send_str(out, &cli, sock);
        }
    }

    close(sock);
    printf("[UDP] Server stopped.\n");
    return NULL;
}

void udp_start(void)
{
    if (s_run) return;
    s_run = 1;
    pthread_create(&s_thread, NULL, srv, NULL);
}

void udp_stop(void)
{
    if (!s_run) return;
    s_run = 0;
    // thread wakes due to SO_RCVTIMEO and exits on s_run==0
    pthread_join(s_thread, NULL);
}
