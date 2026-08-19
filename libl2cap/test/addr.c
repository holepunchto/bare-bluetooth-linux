// Asserts are the test; keep them in every build type
#undef NDEBUG

#include <assert.h>
#include <string.h>

#include "../include/l2cap.h"

int
main(void) {
  l2cap_addr_t addr;

  assert(l2cap_addr_init("78:AF:08:C0:40:3A", L2CAP_BDADDR_LE_PUBLIC, &addr) == 0);
  assert(addr.type == L2CAP_BDADDR_LE_PUBLIC);
  assert(addr.bdaddr[0] == 0x3A); // little-endian: last octet first
  assert(addr.bdaddr[5] == 0x78);

  char str[18];
  assert(l2cap_addr_to_string(&addr, str) == 0);
  assert(strcmp(str, "78:AF:08:C0:40:3A") == 0);

  assert(l2cap_addr_init("78:AF:08:C0:40", L2CAP_BDADDR_LE_PUBLIC, &addr) < 0);
  assert(l2cap_addr_init("78:AF:08:C0:40:ZZ", L2CAP_BDADDR_LE_PUBLIC, &addr) < 0);
  assert(l2cap_addr_init("", L2CAP_BDADDR_LE_PUBLIC, &addr) < 0);

  return 0;
}
