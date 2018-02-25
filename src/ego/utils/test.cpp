#ifdef NDEBUG
#undef NDEBUG
#endif

#include "test.hpp"
#include <cassert>
#include <cstdio>
#include <stdexcept>

// TODO replace __assert_fail with a backtrace and core dump
// http://stackoverflow.com/questions/18265/

void TestFail(const char* msg, const char* file, int line, const char* pf) {
    char buff[1000];
    snprintf(buff, sizeof(buff), "ASSERT FAIL: %s %s %d %s\n", msg, file, line, pf);
    throw std::runtime_error(buff);
}
