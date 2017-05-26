#include <stdio.h>
#include <stdint.h>

uint64_t rdtsc()
{
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t) hi << 32) | lo;
}

void MAGIC_FUNC (int *a, int *b)
{
    int x;

    x = *a;
    x = x + 7;
    *a = x;

    x = *b;
    x = x + 7;
    *b = x;
}

int main(void)
{
    uint64_t t0, t1;
    int a, b;

    a = 33;
    b = 66;

    t0 = rdtsc();
    MAGIC_FUNC (&a, &b);
    t1 = rdtsc();

    printf("%zu\n", t1 - t0);

    return 0;
}
