const binding = require('../binding')

const security = {
  LOW: binding.L2CAP_SECURITY_LOW,
  MEDIUM: binding.L2CAP_SECURITY_MEDIUM,
  HIGH: binding.L2CAP_SECURITY_HIGH,
  FIPS: binding.L2CAP_SECURITY_FIPS
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
