#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

int MAGIC_FUNC ()
{
    int i, x;

    x = 0;

    for (i = 1; i <= 3; i++) {
        x += i;
    }

    return x;
}

int main(void)
{
    uint64_t t0, t1;
    int ret;
    /* int *a, *b; */

    /* a = (int *) malloc(sizeof(int)); */
    /* b = (int *) malloc(sizeof(int)); */

    /* *a = 10; */

    t0 = rdtsc();
    ret = MAGIC_FUNC ();
    t1 = rdtsc();

    printf("result: %d\n", ret);
    printf("time: %zu\n", t1 - t0);

    return 0;
}
