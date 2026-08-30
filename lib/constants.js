// Mirrors the L2CAP_SECURITY_* levels of libl2cap (l2cap.h)
const security = {
  LOW: 0x01,
  MEDIUM: 0x02,
  HIGH: 0x03,
  FIPS: 0x04
}

exports.security = security

exports.isSecurity = function isSecurity(level) {
  return (
    level === security.LOW ||
    level === security.MEDIUM ||
    level === security.HIGH ||
    level === security.FIPS
  )
}
