#include "handlers/signal_handler.h"

#include <cerrno>
#include <csignal>

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "utils/error_utils.h"

SignalHandler::SignalHandler(std::initializer_list<int> signals, Callback on_signal)
    : on_signal_(std::move(on_signal)) {
    sigset_t mask;
    ::sigemptyset(&mask);
    for (const int signo : signals) ::sigaddset(&mask, signo);

    // Block normal delivery, so the signals queue up for the signalfd instead.
    if (const int err = ::pthread_sigmask(SIG_BLOCK, &mask, nullptr); err != 0)
        throw_errno(err, "pthread_sigmask");

    fd_.reset(::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    if (!fd_) throw_errno("signalfd");
}

void SignalHandler::handle_event(std::uint32_t events) {
    if (!(events & EPOLLIN)) return;

    for (;;) {
        signalfd_siginfo info{};
        const ssize_t n = ::read(fd_.get(), &info, sizeof info);

        if (n == static_cast<ssize_t>(sizeof info)) {
            if (on_signal_) on_signal_(static_cast<int>(info.ssi_signo));
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        return;  // EAGAIN: no more pending signals
    }
}