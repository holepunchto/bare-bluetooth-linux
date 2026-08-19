#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/l2cap.h"

// The kernel exposes no uapi headers for Bluetooth; these definitions are the
// stable socket ABI normally shipped by the BlueZ userspace headers.

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH 31
#endif
#ifndef BTPROTO_L2CAP
#define BTPROTO_L2CAP 0
#endif
#ifndef SOL_BLUETOOTH
#define SOL_BLUETOOTH 274
#endif
#ifndef BT_RCVMTU
#define BT_RCVMTU 13
#endif
#ifndef L2CAP_DEFAULT_MTU
#define L2CAP_DEFAULT_MTU 672
#endif

struct l2cap_sockaddr_l2 {
  sa_family_t l2_family;
  uint16_t l2_psm;
  uint8_t l2_bdaddr[6];
  uint16_t l2_cid;
  uint8_t l2_bdaddr_type;
};

struct l2cap_chunk_s {
  l2cap_chunk_t *next;
  size_t len;
  uint8_t data[];
};

enum {
  L2CAP_STATE_IDLE = 0,
  L2CAP_STATE_CONNECTING,
  L2CAP_STATE_OPEN,
  L2CAP_STATE_FAILED,
  L2CAP_STATE_CLOSED,
};

int
l2cap_addr_init(const char *str, uint8_t type, l2cap_addr_t *addr) {
  if (strlen(str) != 17) return -EINVAL;

  // bdaddr_t is little-endian: byte 0 is the last octet of the string
  for (int i = 0; i < 6; i++) {
    const char *at = str + (5 - i) * 3;
    char *end;
    long value = strtol(at, &end, 16);
    if (end != at + 2 || value < 0 || value > 255) return -EINVAL;
    addr->bdaddr[i] = (uint8_t) value;
  }

  addr->type = type;

  return 0;
}

int
l2cap_addr_to_string(const l2cap_addr_t *addr, char *str) {
  const uint8_t *b = addr->bdaddr;
  snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X", b[5], b[4], b[3], b[2], b[1], b[0]);
  return 0;
}

static void
l2cap_channel__opened(l2cap_channel_t *channel) {
  uint16_t mtu = 0;
  socklen_t len = sizeof(mtu);
  if (getsockopt(channel->fd, SOL_BLUETOOTH, BT_RCVMTU, &mtu, &len) != 0 || mtu == 0) {
    mtu = L2CAP_DEFAULT_MTU;
  }

  channel->mtu = mtu;
  channel->read_buf = malloc(mtu);
  channel->state = channel->read_buf ? L2CAP_STATE_OPEN : L2CAP_STATE_FAILED;
}

int
l2cap_channel_connect(l2cap_channel_t *channel, const l2cap_addr_t *local, const l2cap_addr_t *peer, uint16_t psm, l2cap_connect_cb cb) {
  memset(&channel->fd, 0, sizeof(*channel) - offsetof(l2cap_channel_t, fd));
  channel->fd = -1;

  int fd = socket(AF_BLUETOOTH, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, BTPROTO_L2CAP);
  if (fd < 0) return -errno;

  struct l2cap_sockaddr_l2 local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.l2_family = AF_BLUETOOTH;
  local_addr.l2_bdaddr_type = local->type;
  memcpy(local_addr.l2_bdaddr, local->bdaddr, 6);

  if (bind(fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) != 0) {
    int err = errno;
    close(fd);
    return -err;
  }

  struct l2cap_sockaddr_l2 peer_addr;
  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.l2_family = AF_BLUETOOTH;
  peer_addr.l2_psm = psm; // Little-endian on every platform Linux Bluetooth supports
  peer_addr.l2_bdaddr_type = peer->type;
  memcpy(peer_addr.l2_bdaddr, peer->bdaddr, 6);

  if (connect(fd, (struct sockaddr *) &peer_addr, sizeof(peer_addr)) != 0 && errno != EINPROGRESS) {
    int err = errno;
    close(fd);
    return -err;
  }

  channel->fd = fd;
  channel->state = L2CAP_STATE_CONNECTING;
  channel->psm = psm;
  channel->peer = *peer;
  channel->on_connect = cb;

  return 0;
}

int
l2cap_channel_accept(l2cap_channel_t *channel, int fd) {
  memset(&channel->fd, 0, sizeof(*channel) - offsetof(l2cap_channel_t, fd));
  channel->fd = -1;

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -errno;

  channel->fd = fd;

  l2cap_channel__opened(channel);
  if (channel->state != L2CAP_STATE_OPEN) return -ENOMEM;

  return 0;
}

int
l2cap_channel_fd(const l2cap_channel_t *channel) {
  return channel->fd;
}

int
l2cap_channel_events(const l2cap_channel_t *channel) {
  switch (channel->state) {
  case L2CAP_STATE_CONNECTING:
    return L2CAP_WRITABLE;
  case L2CAP_STATE_OPEN:
    return (channel->reading ? L2CAP_READABLE : 0) | (channel->write_head ? L2CAP_WRITABLE : 0);
  default:
    return 0;
  }
}

static int
l2cap_channel__flush(l2cap_channel_t *channel) {
  while (channel->write_head) {
    l2cap_chunk_t *chunk = channel->write_head;
    if (send(channel->fd, chunk->data, chunk->len, MSG_DONTWAIT | MSG_NOSIGNAL) < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
      if (errno == EINTR) continue;
      return -errno;
    }

    channel->write_head = chunk->next;
    if (channel->write_head == NULL) channel->write_tail = NULL;
    free(chunk);
  }

  if (channel->on_drain) {
    l2cap_drain_cb cb = channel->on_drain;
    channel->on_drain = NULL;
    cb(channel);
  }

  return 0;
}

int
l2cap_channel_process(l2cap_channel_t *channel, int events) {
  if (channel->state == L2CAP_STATE_CONNECTING && (events & L2CAP_WRITABLE)) {
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(channel->fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0) err = errno;

    if (err) {
      channel->state = L2CAP_STATE_FAILED;
      channel->on_connect(channel, -err);
      return 0;
    }

    l2cap_channel__opened(channel);
    if (channel->state != L2CAP_STATE_OPEN) return -ENOMEM;

    channel->on_connect(channel, 0);
  }

  if (channel->state != L2CAP_STATE_OPEN) return 0;

  if (events & L2CAP_WRITABLE) {
    int err = l2cap_channel__flush(channel);
    if (err < 0) return err;
    if (channel->state != L2CAP_STATE_OPEN) return 0;
  }

  if (events & L2CAP_READABLE) {
    while (channel->reading && channel->state == L2CAP_STATE_OPEN) {
      ssize_t n = recv(channel->fd, channel->read_buf, channel->mtu, MSG_DONTWAIT);
      if (n > 0) {
        channel->on_read(channel, (size_t) n, channel->read_buf);
      } else if (n == 0) {
        channel->on_read(channel, 0, NULL);
        break;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else if (errno == EINTR) {
        continue;
      } else {
        return -errno;
      }
    }
  }

  return 0;
}

int
l2cap_channel_read_start(l2cap_channel_t *channel, l2cap_read_cb cb) {
  if (channel->state != L2CAP_STATE_OPEN) return -ENOTCONN;
  channel->reading = 1;
  channel->on_read = cb;
  return 0;
}

int
l2cap_channel_read_stop(l2cap_channel_t *channel) {
  channel->reading = 0;
  return 0;
}

int
l2cap_channel_write(l2cap_channel_t *channel, const uint8_t *data, size_t len, l2cap_drain_cb cb) {
  if (channel->state != L2CAP_STATE_OPEN) return -ENOTCONN;
  if (len == 0) return 0;

  if (channel->write_head == NULL) {
    if (send(channel->fd, data, len, MSG_DONTWAIT | MSG_NOSIGNAL) >= 0) return 0;
    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return -errno;
  }

  l2cap_chunk_t *chunk = malloc(sizeof(l2cap_chunk_t) + len);
  if (chunk == NULL) return -ENOMEM;

  chunk->next = NULL;
  chunk->len = len;
  memcpy(chunk->data, data, len);

  if (channel->write_tail) channel->write_tail->next = chunk;
  else channel->write_head = chunk;
  channel->write_tail = chunk;

  channel->on_drain = cb;

  return 1;
}

uint16_t
l2cap_channel_psm(const l2cap_channel_t *channel) {
  return channel->psm;
}

uint16_t
l2cap_channel_mtu(const l2cap_channel_t *channel) {
  return channel->mtu;
}

const l2cap_addr_t *
l2cap_channel_peer(const l2cap_channel_t *channel) {
  return &channel->peer;
}

void
l2cap_channel_close(l2cap_channel_t *channel) {
  if (channel->state == L2CAP_STATE_CLOSED) return;

  if (channel->fd >= 0) close(channel->fd);
  channel->fd = -1;

  free(channel->read_buf);
  channel->read_buf = NULL;

  while (channel->write_head) {
    l2cap_chunk_t *chunk = channel->write_head;
    channel->write_head = chunk->next;
    free(chunk);
  }
  channel->write_tail = NULL;

  channel->state = L2CAP_STATE_CLOSED;
}
