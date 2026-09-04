const EventEmitter = require('bare-events')
const binding = require('../binding')
const Device = require('./device')
const L2CAPChannel = require('./channel')
const L2CAPServer = require('./l2cap-server')
const constants = require('./constants')
const errors = require('./errors')

const AGENT_PATH = '/com/bare/agent0'
const Service = require('./service')
const Characteristic = require('./characteristic')
const Descriptor = require('./descriptor')

module.exports = exports = class Adapter extends EventEmitter {
  constructor(opts = {}) {
    super()

    this._path = opts.path || '/org/bluez/hci0'
    this._destroyed = false
    this._devices = new Map()
    this._gattApp = null
    this._agent = null
    this._l2capServers = new Map()
    this._l2capChannels = new Set()

    this._handle = binding.adapterInit(
      this._path,
      this,
      this._ondeviceadded,
      this._ondeviceremoved,
      this._onserviceadded,
      this._onserviceremoved,
      this._oncharadded,
      this._oncharremoved,
      this._ondescadded,
      this._ondescremoved,
      this._oncharvalue,
      this._ondevicepropschanged,
      this._onadapterpropschanged,
      this._onadvertisementrelease,
      this._ongattcharacteristicwrite,
      this._ongattcharacteristicnotifying,
      this._ongattcharacteristicread,
      this._onagentrequest
    )
  }

  get path() {
    return this._path
  }

  get powered() {
    if (this._destroyed) return
    return binding.adapterGetPowered(this._handle)
  }

  set powered(val) {
    if (this._destroyed) return
    binding.adapterSetPowered(this._handle, val)
  }

  get discovering() {
    if (this._destroyed) return
    return binding.adapterGetDiscovering(this._handle)
  }

  get address() {
    if (this._destroyed) return this._address
    if (this._address === undefined) {
      this._address = binding.adapterGetAddress(this._handle) ?? null
    }
    return this._address
  }

  get devices() {
    return this._devices
  }

  setDiscoveryFilter({ uuids = [], rssi, transport } = {}) {
    if (this._destroyed) return
    binding.adapterSetDiscoveryFilter(this._handle, uuids, rssi, transport)
  }

  startDiscovery() {
    if (this._destroyed) return
    binding.adapterStartDiscovery(this._handle)
  }

  stopDiscovery() {
    if (this._destroyed) return
    binding.adapterStopDiscovery(this._handle)
  }

  registerAdvertisement(advertisement) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))

    return new Promise((resolve, reject) => {
      binding.advertisementRegister(
        this._handle,
        advertisement.type,
        advertisement.serviceUUIDs,
        advertisement.localName,
        Object.entries(advertisement.serviceData),
        (err) => {
          if (err) reject(err)
          else resolve()
        }
      )
    })
  }

  unregisterAdvertisement(advertisement) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    return new Promise((resolve, reject) => {
      binding.advertisementUnregister(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  registerApplication(application) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    if (this._gattApp) return Promise.reject(new Error('An application is already registered'))
    this._gattApp = application

    for (let i = 0; i < application.services.length; i++) {
      const svc = application.services[i]
      const svcIdx = binding.gattServiceAdd(this._handle, application.path, svc.uuid, svc.primary)

      for (let j = 0; j < svc.characteristics.length; j++) {
        const ch = svc.characteristics[j]
        ch._path = binding.gattCharacteristicAdd(
          this._handle,
          svcIdx,
          ch.uuid,
          ch.flags,
          ch.value instanceof Uint8Array ? ch.value : new Uint8Array(ch.value)
        )
        ch._adapter = this
      }
    }

    return new Promise((resolve, reject) => {
      binding.gattRegister(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  unregisterApplication(application) {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    return new Promise((resolve, reject) => {
      binding.gattUnregister(this._handle, (err) => {
        if (err) {
          reject(err)
        } else {
          this._gattApp._reset()
          this._gattApp = null
          resolve()
        }
      })
    })
  }

  publishL2CAPChannel({ psm = 0, security } = {}) {
    if (this._destroyed) return

    if (security !== undefined && !constants.isSecurity(security)) {
      queueMicrotask(() => {
        this.emit('error', errors.CHANNEL_PUBLISH_FAILED('Unknown security level'))
      })
      return
    }

    binding.l2capPublish(
      this._handle,
      psm,
      security,
      this,
      this._onl2capconnection,
      this._onl2caperror,
      // The native callback fires synchronously: register state now so a
      // same-tick destroy() sees the server, and only defer the events
      (err, serverHandle) => {
        if (err || !serverHandle) {
          queueMicrotask(() => {
            if (this._destroyed) return
            this.emit('error', errors.CHANNEL_PUBLISH_FAILED(err ? err.message : 'Publish failed'))
          })
          return
        }

        const server = new L2CAPServer(serverHandle)
        this._l2capServers.set(server.psm, server)

        queueMicrotask(() => {
          if (this._destroyed) return // destroy() already unpublished it
          this.emit('channelPublish', server.psm)
        })
      }
    )
  }

  unpublishL2CAPChannel(psm) {
    const server = this._l2capServers.get(psm)
    if (!server) return
    this._l2capServers.delete(psm)
    server.unpublish()
  }

  _onl2capconnection(channelHandle) {
    const channel = new L2CAPChannel(channelHandle)

    if (this.listenerCount('channelOpen') === 0) {
      // Nobody will ever receive it: close instead of leaking the descriptor
      channel.destroy()
      return
    }

    this._l2capChannels.add(channel)
    channel.on('close', () => this._l2capChannels.delete(channel))

    this.emit('channelOpen', channel)
  }

  // The native side has already closed the server when this fires
  _onl2caperror(psm, message) {
    this._l2capServers.delete(psm)
    this.emit('error', errors.CHANNEL_FAILED(`Accept failed: ${message}`))
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    this._devices.clear()

    for (const psm of [...this._l2capServers.keys()]) {
      this.unpublishL2CAPChannel(psm)
    }

    for (const channel of [...this._l2capChannels]) {
      channel.destroy()
    }
    this._l2capChannels.clear()

    if (this._gattApp) {
      binding.gattUnregister(this._handle, () => {})
      this._gattApp._reset()
      this._gattApp = null
    }

    if (this._agent) {
      binding.agentUnregister(this._handle, () => {})
      this._agent = null
    }

    binding.adapterDestroy(this._handle)
  }

  [Symbol.dispose]() {
    this.destroy()
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Adapter },
      path: this.path,
      powered: this.powered,
      discovering: this.discovering
    }
  }

  _ondeviceadded(path, address, addressType) {
    if (this._devices.has(path)) return
    const device = new Device(this, path, address, addressType)
    this._devices.set(path, device)
    this.emit('device', device)
  }

  _ondeviceremoved(path) {
    const device = this._devices.get(path)
    if (device) {
      this._devices.delete(path)
      this.emit('deviceRemoved', device)
    }
  }

  _onserviceadded(path, devicePath, uuid) {
    const device = this._devices.get(devicePath)
    if (device) {
      const service = new Service(device, path, uuid)
      device._services.set(path, service)
      device.emit('service', service)
    }
  }

  _onserviceremoved(path, devicePath) {
    const device = this._devices.get(devicePath)
    if (device) {
      const service = device._services.get(path)
      if (service) {
        device._services.delete(path)
        device.emit('serviceRemoved', service)
      }
    }
  }

  _oncharadded(path, servicePath, uuid) {
    for (const device of this._devices.values()) {
      const service = device._services.get(servicePath)
      if (service) {
        const characteristic = new Characteristic(this, path, uuid)
        service._characteristics.set(path, characteristic)
        service.emit('characteristic', characteristic)
        return
      }
    }
  }

  _oncharremoved(path, servicePath) {
    for (const device of this._devices.values()) {
      const service = device._services.get(servicePath)
      if (service) {
        const characteristic = service._characteristics.get(path)
        if (characteristic) {
          service._characteristics.delete(path)
          service.emit('characteristicRemoved', characteristic)
        }
        return
      }
    }
  }

  _ondescadded(path, charPath, uuid) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(charPath)
        if (characteristic) {
          const descriptor = new Descriptor(this, path, uuid)
          characteristic._descriptors.set(path, descriptor)
          characteristic.emit('descriptor', descriptor)
          return
        }
      }
    }
  }

  _ondescremoved(path, charPath) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(charPath)
        if (characteristic) {
          const descriptor = characteristic._descriptors.get(path)
          if (descriptor) {
            characteristic._descriptors.delete(path)
            characteristic.emit('descriptorRemoved', descriptor)
          }
          return
        }
      }
    }
  }

  _ondevicepropschanged(path, connected, paired, servicesResolved, rssi, name) {
    const device = this._devices.get(path)
    if (!device) return

    if (connected !== undefined) device.emit('connected', connected)
    if (paired !== undefined) device.emit('paired', paired)
    if (servicesResolved !== undefined) device.emit('servicesResolved', servicesResolved)
    if (rssi !== undefined) device.emit('rssi', rssi)
    if (name !== undefined) device.emit('name', name)
  }

  _onadapterpropschanged(powered, discovering) {
    if (powered !== undefined) this.emit('powered', powered)
    if (discovering !== undefined) this.emit('discovering', discovering)
  }

  _onadvertisementrelease() {
    this.emit('advertisementReleased')
  }

  registerAgent(agent, capability = '') {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    if (this._agent) return Promise.reject(new Error('An agent is already registered'))

    this._agent = agent

    return new Promise((resolve, reject) => {
      binding.agentRegister(this._handle, AGENT_PATH, capability, (err) => {
        if (err) {
          this._agent = null
          reject(err)
        } else {
          resolve()
        }
      })
    })
  }

  requestDefaultAgent() {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    if (!this._agent) return Promise.reject(new Error('No agent is registered'))

    return new Promise((resolve, reject) => {
      binding.agentRequestDefault(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  unregisterAgent() {
    if (this._destroyed) return Promise.reject(new Error('Adapter is destroyed'))
    if (!this._agent) return Promise.reject(new Error('No agent is registered'))

    this._agent = null

    return new Promise((resolve, reject) => {
      binding.agentUnregister(this._handle, (err) => {
        if (err) reject(err)
        else resolve()
      })
    })
  }

  async _onagentrequest(method, id, device, text, number, entered) {
    if (this._destroyed) return

    const agent = this._agent

    if (agent === null) {
      if (id !== 0) {
        binding.agentRespondError(
          this._handle,
          id,
          'org.bluez.Error.Canceled',
          'No agent is registered'
        )
      }
      return
    }

    try {
      switch (method) {
        case 'Release':
          this._agent = null
          return await agent.release()
        case 'Cancel':
          return await agent.cancel()
        case 'DisplayPinCode':
          return await agent.displayPinCode(device, text)
        case 'DisplayPasskey':
          return await agent.displayPasskey(device, number, entered)
        case 'RequestPinCode': {
          const pinCode = await agent.requestPinCode(device)
          if (this._destroyed) return
          return binding.agentRespondString(this._handle, id, pinCode)
        }
        case 'RequestPasskey': {
          const passkey = await agent.requestPasskey(device)
          if (this._destroyed) return
          return binding.agentRespondNumber(this._handle, id, passkey)
        }
        case 'RequestConfirmation':
          await agent.requestConfirmation(device, number)
          if (this._destroyed) return
          return binding.agentRespond(this._handle, id)
        case 'RequestAuthorization':
          await agent.requestAuthorization(device)
          if (this._destroyed) return
          return binding.agentRespond(this._handle, id)
        case 'AuthorizeService':
          await agent.authorizeService(device, text)
          if (this._destroyed) return
          return binding.agentRespond(this._handle, id)
      }
    } catch (err) {
      if (this._destroyed) return

      if (id === 0) this.emit('error', err)
      else binding.agentRespondError(this._handle, id, 'org.bluez.Error.Rejected', err.message)
    }
  }

  _ongattcharacteristicread(path, id, offset, mtu, device, link) {
    if (!this._gattApp) return
    for (const svc of this._gattApp.services) {
      for (const ch of svc.characteristics) {
        if (ch._path === path) {
          ch._onread(id, { offset, mtu, device, link })
          return
        }
      }
    }
  }

  _ongattcharacteristicnotifying(path, notifying) {
    if (!this._gattApp) return
    for (const svc of this._gattApp.services) {
      for (const ch of svc.characteristics) {
        if (ch._path === path) {
          ch._notifying = notifying
          ch.emit('notifying', notifying)
          return
        }
      }
    }
  }

  _ongattcharacteristicwrite(path, value, offset, type, mtu, device, link, prepareAuthorize) {
    if (!this._gattApp) return
    for (const svc of this._gattApp.services) {
      for (const ch of svc.characteristics) {
        if (ch._path === path) {
          ch._value = value
          ch.emit('write', value, { offset, type, mtu, device, link, prepareAuthorize })
          return
        }
      }
    }
  }

  _oncharvalue(path, buffer) {
    for (const device of this._devices.values()) {
      for (const service of device._services.values()) {
        const characteristic = service._characteristics.get(path)
        if (characteristic) {
          characteristic.emit('data', buffer)
          return
        }
      }
    }
  }
}
