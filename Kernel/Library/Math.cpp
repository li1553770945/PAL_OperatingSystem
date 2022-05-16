#include <Library/Math.hpp>

Uint64 klog2(Uint64 x)
{
    Uint64 y = 0;
    while(x >>= 1)
    {
        y++;
    }
    return y;
}

Uint64 kpow2(int x)
{
    Uint64 y = 1;
    return y<<x;
}