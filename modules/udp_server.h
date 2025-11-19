// udp_server.h - UDP command server for remote control and status queries

#ifndef MODULE_UDP_SERVER_H
#define MODULE_UDP_SERVER_H

#define UDP_PORT 12345
#define UDP_BUFFER_SIZE 256
#define UDP_CMD_MAX_LEN 128

void udp_start(void);
void udp_stop(void);

#endif
