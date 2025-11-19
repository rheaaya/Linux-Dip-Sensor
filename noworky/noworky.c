#include <stdio.h>
#include <stdlib.h>

static void swapContent(double *a, double *b)
{
    double t = *a; *a = *b; *b = t;
}

// Original bug was: unsigned i; for (i=size-1; i>=0; i--)  { ... }
// Unsigned never becomes < 0 → wraps → out-of-bounds.
void tradeArrays(double *array1, double *array2, int size)
{
    int i; // FIX: signed
    for (i = size - 1; i >= 0; i--) {
        swapContent(array1 + i, array2 + i);
    }
}

int main(void)
{
    int n = 5;
    double a[5] = {1,2,3,4,5};
    double b[5] = {10,20,30,40,50};

    tradeArrays(a,b,n);

    // Fixed: Added braces to make indentation clear
    for (int i=0;i<n;i++) {
        printf("%.0f ", a[i]);
    }
    puts("");
    
    for (int i=0;i<n;i++) {
        printf("%.0f ", b[i]);
    }
    puts("");
    
    return 0;
}
