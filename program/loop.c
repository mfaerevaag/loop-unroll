#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

void MAGIC_FUNC (int *a, int *b)
{
    int i, x, y;

    x = 0;

    for (i = 0; i < *a; i++) {
        x += i;
    }

    *b = x;
}

int main(void)
{
    uint64_t t0, t1;
    int *a, *b;

    a = (int *) malloc(sizeof(int));
    b = (int *) malloc(sizeof(int));

    *a = 10;

    t0 = rdtsc();
    MAGIC_FUNC (a, b);
    t1 = rdtsc();

    printf("%zu\n", t1 - t0);

    return 0;
}
