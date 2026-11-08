#pragma once

#include <unistd.h>
#include <utility>

class UniqueFD {
public:
    UniqueFD() = default;
    explicit UniqueFD(int fd) noexcept : fd_(fd) {}

    UniqueFD(const UniqueFD&) = delete;
    UniqueFD& operator=(const UniqueFD&) = delete;

    UniqueFD(UniqueFD&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    UniqueFD& operator=(UniqueFD&& other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.fd_, -1));
        }
        return *this;
    }

    ~UniqueFD() noexcept { reset(); }

    [[nodiscard]] bool is_valid() const noexcept { return fd_ >= 0; }
    explicit operator bool() const noexcept { return is_valid(); }

    [[nodiscard]] int release() noexcept { return std::exchange(fd_, -1); }

    void reset(int new_fd = -1) noexcept {
        if (fd_ >= 0 && fd_ != new_fd) ::close(fd_);
        fd_ = new_fd;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }

private:
    int fd_ = -1;
};