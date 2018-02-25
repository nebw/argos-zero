#ifndef TO_STRING_H_
#define TO_STRING_H_

#include <sstream>
#include <string>

template <typename T>
std::string ToString(const T& val) {
    std::ostringstream s;
    s << val;
    return s.str();
}

#endif
