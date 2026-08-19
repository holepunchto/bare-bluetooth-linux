// Asserts are the test; keep them in every build type
#undef NDEBUG

// Fill the kernel buffer until a write gets queued, drain the peer, and check
// the drain callback fires exactly once.

#include <assert.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/l2cap.h"

static int drained = 0;

static void
on_drain(l2cap_channel_t *channel) {
  drained++;
}

int
main(void) {
  int fds[2];
  assert(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) == 0);

  int sndbuf = 4096;
  assert(setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) == 0);

  l2cap_channel_t a;
  assert(l2cap_channel_accept(&a, fds[0]) == 0);

  uint8_t msg[512];
  memset(msg, 0xab, sizeof(msg));

  // Write until the kernel pushes back and the chunk gets queued
  int r = 0;
  int sent = 0;
  while ((r = l2cap_channel_write(&a, msg, sizeof(msg), on_drain)) == 0) {
    sent++;
    assert(sent < 10000);
  }
  assert(r == 1);
  assert(l2cap_channel_events(&a) & L2CAP_WRITABLE);

  // Drain the peer side, then let the channel flush
  uint8_t buf[sizeof(msg)];
  while (drained == 0) {
    while (recv(fds[1], buf, sizeof(buf), MSG_DONTWAIT) > 0);

    struct pollfd p = {l2cap_channel_fd(&a), POLLOUT, 0};
    assert(poll(&p, 1, 1000) >= 0);
    assert(l2cap_channel_process(&a, L2CAP_WRITABLE) == 0);
  }

  assert(drained == 1);
  assert((l2cap_channel_events(&a) & L2CAP_WRITABLE) == 0);

  l2cap_channel_close(&a);

  return 0;
}
