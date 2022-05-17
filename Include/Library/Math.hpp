#include "../Types.hpp"

namespace POS
{
	inline Uint64 klog2(Uint64 x)
	{
	    Uint64 y = 0;
	    while(x >>= 1)
	    {
	        y++;
	    }
	
	    return y;
	}
	
	inline Uint64 kpow2(int x)
	{return 1ull<<x;}
};
