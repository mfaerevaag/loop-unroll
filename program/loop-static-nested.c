#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

int MAGIC_FUNC ()
{
    int i, j, x;

    x = 0;

    for (i = 1; i <= MAGIC_TRIP; i++) {
        for (j = 1; j <= i; j++) {
            x += j;
        }
    }

    return x;
}

int main(void)
{
    uint64_t t0, t1;
    int ret;

    t0 = rdtsc();
    ret = MAGIC_FUNC ();
    t1 = rdtsc();

    printf("result: %d\n", ret);
    printf("time: %zu\n", t1 - t0);

    return 0;
}
