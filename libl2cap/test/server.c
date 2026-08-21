// The server machinery — attach, events, process, accept — runs against a
// listening Unix SOCK_SEQPACKET socket: same accept semantics as L2CAP,
// no adapter required. Only l2cap_server_listen() itself needs Bluetooth.

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <l2cap.h>

static int connections = 0;
static char received[64];
static size_t received_len = 0;

static void
on_connection(l2cap_server_t *server) {
  (void) server;
  connections++;
}

static void
on_read(l2cap_channel_t *channel, size_t len, const uint8_t *data) {
  (void) channel;

  if (len == 0) return;

  assert(len < sizeof(received));
  memcpy(received, data, len);
  received_len = len;
}

int
main(void) {
  // An abstract-namespace address: no filesystem entry to clean up
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  addr.sun_path[0] = '\0';
  memcpy(addr.sun_path + 1, "libl2cap-test", 13);
  socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + 1 + 13;

  int listen_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  assert(listen_fd >= 0);
  assert(bind(listen_fd, (struct sockaddr *) &addr, addr_len) == 0);
  assert(listen(listen_fd, 4) == 0);

  l2cap_server_t server;
  l2cap_server_init(&server, NULL);
  assert(l2cap_server_events(&server) == 0); // idle

  assert(l2cap_server_attach(&server, listen_fd, on_connection) == 0);
  assert(l2cap_server_events(&server) == L2CAP_READABLE);

  // Nothing pending yet
  l2cap_channel_t accepted;
  l2cap_channel_init(&accepted, NULL);
  assert(l2cap_server_accept(&server, &accepted) == -EAGAIN);

  // A client connects
  int client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  assert(client_fd >= 0);
  assert(connect(client_fd, (struct sockaddr *) &addr, addr_len) == 0);

  struct pollfd p = {l2cap_server_fd(&server), POLLIN, 0};
  assert(poll(&p, 1, 1000) == 1);

  assert(l2cap_server_process(&server, L2CAP_READABLE) == 0);
  assert(connections == 1);

  assert(l2cap_server_accept(&server, &accepted) == 0);
  assert(l2cap_channel_read_start(&accepted, on_read) == 0);

  // Data flows from the client into the accepted channel
  const uint8_t msg[] = "hello from client";
  assert(send(client_fd, msg, sizeof(msg), 0) >= 0);

  p.fd = l2cap_channel_fd(&accepted);
  assert(poll(&p, 1, 1000) == 1);
  assert(l2cap_channel_process(&accepted, L2CAP_READABLE) == 0);
  assert(received_len == sizeof(msg));
  assert(memcmp(received, msg, sizeof(msg)) == 0);

  // And back
  const uint8_t reply[] = "hello from server";
  assert(l2cap_channel_write(&accepted, reply, sizeof(reply), NULL) == L2CAP_WRITE_SENT);

  uint8_t buf[64];
  assert(recv(client_fd, buf, sizeof(buf), 0) == sizeof(reply));
  assert(memcmp(buf, reply, sizeof(reply)) == 0);

  // Closing the server leaves the accepted channel alive
  l2cap_server_close(&server);
  assert(l2cap_server_events(&server) == 0);
  l2cap_server_close(&server); // idempotent

  assert(l2cap_channel_write(&accepted, reply, sizeof(reply), NULL) == L2CAP_WRITE_SENT);
  assert(recv(client_fd, buf, sizeof(buf), 0) == sizeof(reply));

  l2cap_channel_close(&accepted);
  close(client_fd);

  return 0;
}
