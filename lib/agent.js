module.exports = exports = class Agent {
  requestPinCode(device) {
    return Promise.reject(new Error('requestPinCode is not implemented'))
  }

  requestPasskey(device) {
    return Promise.reject(new Error('requestPasskey is not implemented'))
  }

  requestConfirmation(device, passkey) {
    return Promise.reject(new Error('requestConfirmation is not implemented'))
  }

  requestAuthorization(device) {
    return Promise.reject(new Error('requestAuthorization is not implemented'))
  }

  authorizeService(device, uuid) {
    return Promise.reject(new Error('authorizeService is not implemented'))
  }

  displayPinCode(device, pincode) {
    return Promise.reject(new Error('displayPinCode is not implemented'))
  }

  displayPasskey(device, passkey, entered) {
    return Promise.resolve()
  }

  release() {
    return Promise.resolve()
  }

  cancel() {
    return Promise.resolve()
  }
}
