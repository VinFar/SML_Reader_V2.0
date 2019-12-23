#include "main.h"

uint16_t Log2n(uint16_t n)
{
    return (n > 1) ? 1 + Log2n(n / 2) : 0;
}

int16_t isPowerOfTwo(uint16_t n)
{
    return n && (!(n & (n - 1)));
}

int16_t findPosition(uint16_t n)
{
    if (!isPowerOfTwo(n))
        return -1;
    return Log2n(n) + 1;
}
