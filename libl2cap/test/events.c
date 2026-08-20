// Asserts are the test; keep them in every build type
#undef NDEBUG

// events() must track the channel state exactly: nothing when idle or newly
// open, READABLE while reading, WRITABLE while the queue is non-empty,
// nothing once closed.

#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <l2cap.h>

static void
on_read(l2cap_channel_t *channel, size_t len, const uint8_t *data) {
  (void) channel;
  (void) len;
  (void) data;
}

int
main(void) {
  int fds[2];
  assert(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) == 0);

  l2cap_channel_t a;
  l2cap_channel_init(&a, NULL);
  assert(l2cap_channel_events(&a) == 0); // idle

  assert(l2cap_channel_accept(&a, fds[0]) == 0);
  assert(l2cap_channel_events(&a) == 0); // open, not reading, nothing queued

  assert(l2cap_channel_read_start(&a, on_read) == 0);
  assert(l2cap_channel_events(&a) == L2CAP_READABLE);

  assert(l2cap_channel_read_stop(&a) == 0);
  assert(l2cap_channel_events(&a) == 0);

  // Shrink the buffer and fill it so a chunk gets queued
  int sndbuf = 4096;
  assert(setsockopt(l2cap_channel_fd(&a), SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) == 0);

  uint8_t msg[512];
  memset(msg, 0xab, sizeof(msg));

  int r = 0;
  int sent = 0;
  while ((r = l2cap_channel_write(&a, msg, sizeof(msg), NULL)) == 0) {
    sent++;
    assert(sent < 10000);
  }
  assert(r == 1);
  assert(l2cap_channel_events(&a) == L2CAP_WRITABLE);

  assert(l2cap_channel_read_start(&a, on_read) == 0);
  assert(l2cap_channel_events(&a) == (L2CAP_READABLE | L2CAP_WRITABLE));

  l2cap_channel_close(&a);
  assert(l2cap_channel_events(&a) == 0); // closed

  close(fds[1]);

  return 0;
}
