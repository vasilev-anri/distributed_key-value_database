#pragma once

#include <cerrno>
#include <system_error>

[[noreturn]] inline void throw_errno(int err, const char* what) {
    throw std::system_error(err, std::system_category(), what);
}

[[noreturn]] inline void throw_errno(const char* what) {
    throw_errno(errno, what);
}