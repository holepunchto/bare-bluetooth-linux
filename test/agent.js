const test = require('brittle')
const { Adapter } = require('..')
const { isCI } = require('./helpers')

const agent = {
  requestConfirmation() {},
  requestAuthorization() {}
}

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
