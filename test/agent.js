const test = require('brittle')
const { Adapter, Agent } = require('..')
const { isCI } = require('./helpers')

class JustWorks extends Agent {
  requestConfirmation(device, passkey) {}

  requestAuthorization(device) {}
}

const agent = new JustWorks()

test('registerAgent and unregisterAgent', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await adapter.registerAgent(agent, 'NoInputNoOutput')
  t.pass('agent registered')

  await adapter.unregisterAgent()
  t.pass('agent unregistered')
})

test('registerAgent rejects if already registered', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await adapter.registerAgent(agent, 'NoInputNoOutput')

  await t.exception(() => adapter.registerAgent(agent), /already registered/)

  await adapter.unregisterAgent()
})

test('unregisterAgent rejects without an agent', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await t.exception(() => adapter.unregisterAgent(), /No agent is registered/)
})

test('requestDefaultAgent rejects without an agent', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await t.exception(() => adapter.requestDefaultAgent(), /No agent is registered/)
})

test('requestDefaultAgent after registering', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await adapter.registerAgent(agent, 'NoInputNoOutput')

  await adapter.requestDefaultAgent()
  t.pass('became the default agent')

  await adapter.unregisterAgent()
})

test('registerAgent rejects an unknown capability', { skip: isCI }, async (t) => {
  using adapter = new Adapter()

  await t.exception(() => adapter.registerAgent(agent, 'Telepathy'))
})

test('Agent is exported', (t) => {
  t.is(typeof Agent, 'function')
  t.is(Agent.name, 'Agent')
})

test('Agent refuses everything it was not taught', (t) => {
  const bare = new Agent()

  t.exception(() => bare.requestPinCode('/org/bluez/hci0/dev_00'), /not implemented/)
  t.exception(() => bare.requestPasskey('/org/bluez/hci0/dev_00'), /not implemented/)
  t.exception(() => bare.requestConfirmation('/org/bluez/hci0/dev_00', 0), /not implemented/)
  t.exception(() => bare.requestAuthorization('/org/bluez/hci0/dev_00'), /not implemented/)
  t.exception(() => bare.authorizeService('/org/bluez/hci0/dev_00', '180a'), /not implemented/)
})

test('Agent ignores what needs no answer', (t) => {
  const bare = new Agent()

  t.execution(() => bare.displayPinCode('/org/bluez/hci0/dev_00', '0000'))
  t.execution(() => bare.displayPasskey('/org/bluez/hci0/dev_00', 0, 0))
  t.execution(() => bare.release())
  t.execution(() => bare.cancel())
})
