#include <gtest/gtest.h>

#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <sys/eventfd.h>
#include <type_traits>
#include <unistd.h>

#include "../src/utils/unique_fd.h"

namespace {

bool is_open(int fd) {
    return ::fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

int make_fd() {
    int fd = ::eventfd(0, EFD_CLOEXEC);
    if (fd < 0) throw std::runtime_error("eventfd failed");
    return fd;
}

}  // namespace

// ── Compile-time guarantees ─────────────────────────────────────────────

static_assert(!std::is_copy_constructible_v<UniqueFD>);
static_assert(!std::is_copy_assignable_v<UniqueFD>);
static_assert(std::is_nothrow_move_constructible_v<UniqueFD>);
static_assert(std::is_nothrow_move_assignable_v<UniqueFD>);

// ── Basic state ─────────────────────────────────────────────────────────

TEST(UniqueFD, DefaultConstructedIsInvalid) {
    UniqueFD fd;
    EXPECT_FALSE(fd.is_valid());
    EXPECT_FALSE(fd);
    EXPECT_EQ(fd.get(), -1);
}

TEST(UniqueFD, WrapsDescriptor) {
    int raw = make_fd();
    UniqueFD fd(raw);
    EXPECT_TRUE(fd.is_valid());
    EXPECT_TRUE(fd);
    EXPECT_EQ(fd.get(), raw);
}

// ── Destructor ──────────────────────────────────────────────────────────

TEST(UniqueFD, DestructorClosesDescriptor) {
    int raw = make_fd();
    {
        UniqueFD fd(raw);
        EXPECT_TRUE(is_open(raw));
    }  // fd destroyed here
    EXPECT_FALSE(is_open(raw));
}

// ── Move constructor ────────────────────────────────────────────────────

TEST(UniqueFD, MoveConstructorTransfersOwnership) {
    int raw = make_fd();
    UniqueFD dst;
    {
        UniqueFD src(raw);
        dst = UniqueFD(std::move(src));

        EXPECT_FALSE(src.is_valid());  // source gave it up
        EXPECT_EQ(dst.get(), raw);     // destination owns it
    }  // src destroyed here — must NOT close raw

    EXPECT_TRUE(is_open(raw));  // still open: no double close
}

// ── Move assignment ─────────────────────────────────────────────────────

TEST(UniqueFD, MoveAssignmentClosesOldAndTakesNew) {
    int raw_a = make_fd();
    int raw_b = make_fd();

    UniqueFD a(raw_a);
    UniqueFD b(raw_b);

    b = std::move(a);

    EXPECT_FALSE(is_open(raw_b));  // b's old descriptor was closed
    EXPECT_TRUE(is_open(raw_a));   // a's descriptor is alive...
    EXPECT_EQ(b.get(), raw_a);     // ...and now owned by b
    EXPECT_FALSE(a.is_valid());    // a is empty
}

TEST(UniqueFD, SelfMoveAssignmentKeepsDescriptor) {
    int raw = make_fd();
    UniqueFD fd(raw);

    UniqueFD& alias = fd;          // alias avoids a self-move compiler warning
    fd = std::move(alias);

    EXPECT_EQ(fd.get(), raw);
    EXPECT_TRUE(is_open(raw));
}

// ── release() ───────────────────────────────────────────────────────────

TEST(UniqueFD, ReleaseGivesUpOwnershipWithoutClosing) {
    int raw = make_fd();
    int released = -1;
    {
        UniqueFD fd(raw);
        released = fd.release();
        EXPECT_FALSE(fd.is_valid());
    }  // fd destroyed — must not close anything

    EXPECT_EQ(released, raw);
    EXPECT_TRUE(is_open(raw));

    ::close(raw);  // we own it now, so we clean up manually
}

// ── reset() ─────────────────────────────────────────────────────────────

TEST(UniqueFD, ResetWithoutArgumentCloses) {
    int raw = make_fd();
    UniqueFD fd(raw);

    fd.reset();

    EXPECT_FALSE(fd.is_valid());
    EXPECT_FALSE(is_open(raw));
}

TEST(UniqueFD, ResetWithNewDescriptorClosesOld) {
    int raw_old = make_fd();
    int raw_new = make_fd();
    UniqueFD fd(raw_old);

    fd.reset(raw_new);

    EXPECT_FALSE(is_open(raw_old));
    EXPECT_TRUE(is_open(raw_new));
    EXPECT_EQ(fd.get(), raw_new);
}

TEST(UniqueFD, ResetWithSameDescriptorDoesNotClose) {
    int raw = make_fd();
    UniqueFD fd(raw);

    fd.reset(raw);

    EXPECT_TRUE(is_open(raw));
    EXPECT_EQ(fd.get(), raw);
}