//
// Copyright 2006 and onwards, Lukasz Lew
//

#include <cstdio>

#include "fast_random.hpp"
#include "test.hpp"

#ifdef __MINGW32__
#include <time.h>
#endif

// tr1::minstd_rand0 mt; // this is eqivalent when #include <tr1/random>

FastRandom::FastRandom(uint seed) : seed(seed) {}

FastRandom::FastRandom() : seed(TimeSeed()) {}

void FastRandom::SetSeed(uint new_seed) { seed = new_seed; }

uint FastRandom::GetSeed() { return seed; }

uint FastRandom::GetNextUint() {  // a number between  0 ... (uint(1)<<31) - 1 - 1
    uint hi, lo;
    lo = 16807 * (seed & 0xffff);
    hi = 16807 * (seed >> 16);
    lo += (hi & 0x7fff) << 16;
    lo += hi >> 15;
    seed = (lo & 0x7FFFFFFF) + (lo >> 31);
    return seed;
}

// n must be between 1 .. (1<<16) + 1
uint FastRandom::GetNextUint(uint n) {  // 0 .. n-1
    ASSERT(n > 0);
    ASSERT(n <= (1 << 16) + 1);
    return ((GetNextUint() & 0xffff) * n) >> 16;
}

double FastRandom::NextDouble() {
    const double inv_max_uint = 1.0 / double(uint(1) << 31);  // TODO - 1 - 1
    return GetNextUint() * inv_max_uint;
}

double FastRandom::NextDouble(double scale) {
    const double inv_max_uint = 1.0 / double(uint(1) << 31);  // TODO - 1 - 1
    uint s = GetNextUint();
    double ret = double(s) * (inv_max_uint * scale);
    return ret;
}

uint64 getCcTime() {
#ifdef _MSC_VER
    return __rdtsc();
#else
    if (sizeof(long) == 8) {
        uint64 a, d;
        asm volatile("rdtsc\n\t" : "=a"(a), "=d"(d));
        return (d << 32) | (a & 0xffffffff);
    } else {
        uint64 l;
        asm volatile("rdtsc\n\t" : "=A"(l));
        return l;
    }
#endif  //_MSC_VER
}

int TimeSeed() { return (int)getCcTime(); }
