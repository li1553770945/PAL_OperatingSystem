#ifndef POS_TYPES_H
#define POS_TYPES_H

#define PlatformIsBigEndian 0

using Sint8=signed char;
using Sint16=signed short;
using Sint32=signed int;
using Sint64=signed long long;
//using Sint128=__int128_t;
using Uint8=unsigned char;
using Uint16=unsigned short;
using Uint32=unsigned int;
using Uint64=unsigned long long;
//using Uint128=__uint128_t;

using RegisterData=Uint64;
using ClockTime=Uint64;
using TickType=Uint64;
using ErrorType=Sint32;
using PID=Uint32;
using TID=Uint32;
using PtrInt=Uint64;//The UintXX of void*
using PageTableEntryType=Uint64;

#endif
