#ifndef L2CAP_H
#define L2CAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define L2CAP_BDADDR_LE_PUBLIC 0x01
#define L2CAP_BDADDR_LE_RANDOM 0x02

/**
 * Readiness bits exchanged with the caller's event loop: `l2cap_channel_events()`
 * says which ones the channel currently waits for, `l2cap_channel_process()`
 * takes the ones that fired.
 */
#define L2CAP_READABLE 0x1
#define L2CAP_WRITABLE 0x2

typedef struct l2cap_addr_s l2cap_addr_t;
typedef struct l2cap_channel_s l2cap_channel_t;
typedef struct l2cap_chunk_s l2cap_chunk_t;

/**
 * The result of a `l2cap_channel_connect()`. `status` is 0 on success or a
 * negated errno on failure; a failed channel must still be closed.
 */
typedef void (*l2cap_connect_cb)(l2cap_channel_t *channel, int status);

/**
 * An inbound SDU. `len == 0` means the peer closed the channel. The buffer is
 * owned by the channel and only valid for the duration of the call.
 */
typedef void (*l2cap_read_cb)(l2cap_channel_t *channel, size_t len, const uint8_t *data);

/**
 * The write queue drained after `l2cap_channel_write()` returned 1.
 */
typedef void (*l2cap_drain_cb)(l2cap_channel_t *channel);

struct l2cap_addr_s {
  uint8_t bdaddr[6]; // Little-endian, as the kernel stores it
  uint8_t type;      // L2CAP_BDADDR_LE_*
};

/**
 * A connection-oriented channel. Allocate it yourself; every field except
 * `data` is private.
 */
struct l2cap_channel_s {
  void *data;

  // Private
  int fd;
  int state;
  int reading;
  uint16_t psm;
  uint16_t mtu;
  l2cap_addr_t peer;
  uint8_t *read_buf;
  l2cap_connect_cb on_connect;
  l2cap_read_cb on_read;
  l2cap_drain_cb on_drain;
  l2cap_chunk_t *write_head;
  l2cap_chunk_t *write_tail;
};

/**
 * Parse "AA:BB:CC:DD:EE:FF" into an address of the given type.
 */
int
l2cap_addr_init(const char *str, uint8_t type, l2cap_addr_t *addr);

/**
 * Format an address back into `str`, which must hold at least 18 bytes.
 */
int
l2cap_addr_to_string(const l2cap_addr_t *addr, char *str);

/**
 * Start a non-blocking connect to `peer` on `psm`, bound to the `local`
 * adapter address. Poll `l2cap_channel_fd()` for `l2cap_channel_events()`
 * and feed the results to `l2cap_channel_process()`; `cb` fires when the
 * connect settles.
 */
int
l2cap_channel_connect(l2cap_channel_t *channel, const l2cap_addr_t *local, const l2cap_addr_t *peer, uint16_t psm, l2cap_connect_cb cb);

/**
 * Adopt an already-connected SOCK_SEQPACKET descriptor. The channel takes
 * ownership and switches it to non-blocking mode.
 */
int
l2cap_channel_accept(l2cap_channel_t *channel, int fd);

int
l2cap_channel_fd(const l2cap_channel_t *channel);

/**
 * The readiness bits the channel currently needs, or 0 when it needs none.
 */
int
l2cap_channel_events(const l2cap_channel_t *channel);

/**
 * Advance the channel after the caller's loop reported `events`. Invokes the
 * registered callbacks synchronously. Returns 0, or a negated errno on a
 * fatal socket error, after which the channel must be closed.
 */
int
l2cap_channel_process(l2cap_channel_t *channel, int events);

int
l2cap_channel_read_start(l2cap_channel_t *channel, l2cap_read_cb cb);

int
l2cap_channel_read_stop(l2cap_channel_t *channel);

/**
 * Send one SDU. Returns 0 when fully handed to the kernel, 1 when queued
 * (`cb` fires once the queue drains), or a negated errno.
 */
int
l2cap_channel_write(l2cap_channel_t *channel, const uint8_t *data, size_t len, l2cap_drain_cb cb);

uint16_t
l2cap_channel_psm(const l2cap_channel_t *channel);

uint16_t
l2cap_channel_mtu(const l2cap_channel_t *channel);

const l2cap_addr_t *
l2cap_channel_peer(const l2cap_channel_t *channel);

/**
 * Close the descriptor and free the channel's internal buffers. Synchronous
 * and idempotent; the channel struct itself is the caller's to reclaim.
 */
void
l2cap_channel_close(l2cap_channel_t *channel);

#ifdef __cplusplus
}
#endif

#endif // L2CAP_H
