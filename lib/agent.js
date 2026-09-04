module.exports = exports = class Agent {
  requestPinCode(device) {
    throw new Error('requestPinCode is not implemented')
  }

  requestPasskey(device) {
    throw new Error('requestPasskey is not implemented')
  }

  requestConfirmation(device, passkey) {
    throw new Error('requestConfirmation is not implemented')
  }

  requestAuthorization(device) {
    throw new Error('requestAuthorization is not implemented')
  }

  authorizeService(device, uuid) {
    throw new Error('authorizeService is not implemented')
  }

  displayPinCode(device, pincode) {}

  displayPasskey(device, passkey, entered) {}

  release() {}

  cancel() {}
}
