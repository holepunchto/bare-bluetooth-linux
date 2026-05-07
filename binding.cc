#include <js.h>
#include <bare.h>
#include <jstl.h>

#include <dbus/dbus.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

#define BLUEZ_BUS        "org.bluez"
#define DBUS_PROP_IFACE  "org.freedesktop.DBus.Properties"
#define BLUEZ_ADAPTER_IFACE "org.bluez.Adapter1"

// --- D-Bus property helpers ---

static char *
dbus_get_string_prop (DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg = dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg,
    DBUS_TYPE_STRING, &iface,
    DBUS_TYPE_STRING, &prop,
    DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
  dbus_message_unref(msg);

  char *result = nullptr;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
      const char *val;
      dbus_message_iter_get_basic(&variant, &val);
      result = strdup(val);
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err)) dbus_error_free(&err);

  return result;
}

static bool
dbus_get_bool_prop (DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg = dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg,
    DBUS_TYPE_STRING, &iface,
    DBUS_TYPE_STRING, &prop,
    DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
  dbus_message_unref(msg);

  bool result = false;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      result = val;
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err)) dbus_error_free(&err);

  return result;
}

static void
dbus_set_bool_prop (DBusConnection *conn, const char *path, const char *iface, const char *prop, bool value) {
  DBusMessage *msg = dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Set");

  DBusMessageIter iter, variant;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &prop);

  dbus_bool_t val = value ? TRUE : FALSE;
  dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val);
  dbus_message_iter_close_container(&iter, &variant);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
  dbus_message_unref(msg);

  if (reply) dbus_message_unref(reply);
  if (dbus_error_is_set(&err)) dbus_error_free(&err);
}

// --- Adapter handle ---

struct bare_bluetooth_linux_adapter_t {
  js_env_t *env;
  DBusConnection *conn;
  char *adapter_path;
};

// --- Adapter functions ---

static js_external_t<bare_bluetooth_linux_adapter_t>
bare_bluetooth_linux_adapter_init (
  js_env_t *env,
  js_receiver_t,
  std::string path
) {
  auto *adapter = new bare_bluetooth_linux_adapter_t;
  adapter->env = env;
  adapter->adapter_path = strdup(path.c_str());

  DBusError err;
  dbus_error_init(&err);
  adapter->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  assert(!dbus_error_is_set(&err));

  js_external_t<bare_bluetooth_linux_adapter_t> handle;
  int e = js_create_external(env, adapter, handle);
  assert(e == 0);

  return handle;
}

static void
bare_bluetooth_linux_adapter_destroy (
  js_env_t *env,
  js_receiver_t,
  js_external_t<bare_bluetooth_linux_adapter_t> handle
) {
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_get_value(env, handle, adapter);
  assert(err == 0);

  if (adapter->conn) {
    dbus_connection_unref(adapter->conn);
    adapter->conn = nullptr;
  }

  free(adapter->adapter_path);
  delete adapter;
}

static bool
bare_bluetooth_linux_adapter_get_powered (
  js_env_t *env,
  js_receiver_t,
  js_external_t<bare_bluetooth_linux_adapter_t> handle
) {
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_get_value(env, handle, adapter);
  assert(err == 0);

  return dbus_get_bool_prop(adapter->conn, adapter->adapter_path, BLUEZ_ADAPTER_IFACE, "Powered");
}

static void
bare_bluetooth_linux_adapter_set_powered (
  js_env_t *env,
  js_receiver_t,
  js_external_t<bare_bluetooth_linux_adapter_t> handle,
  bool value
) {
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_get_value(env, handle, adapter);
  assert(err == 0);

  dbus_set_bool_prop(adapter->conn, adapter->adapter_path, BLUEZ_ADAPTER_IFACE, "Powered", value);
}

static bool
bare_bluetooth_linux_adapter_get_discovering (
  js_env_t *env,
  js_receiver_t,
  js_external_t<bare_bluetooth_linux_adapter_t> handle
) {
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_get_value(env, handle, adapter);
  assert(err == 0);

  return dbus_get_bool_prop(adapter->conn, adapter->adapter_path, BLUEZ_ADAPTER_IFACE, "Discovering");
}

static std::string
bare_bluetooth_linux_adapter_get_address (
  js_env_t *env,
  js_receiver_t,
  js_external_t<bare_bluetooth_linux_adapter_t> handle
) {
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_get_value(env, handle, adapter);
  assert(err == 0);

  char *addr = dbus_get_string_prop(adapter->conn, adapter->adapter_path, BLUEZ_ADAPTER_IFACE, "Address");
  std::string result(addr ? addr : "");
  free(addr);

  return result;
}

// --- Export table ---

static js_value_t *
bare_bluetooth_linux_exports (js_env_t *env, js_value_t *exports) {
  int err;

#define V(name, fn) \
  err = js_set_property<fn>(env, exports, name); \
  assert(err == 0);

  V("adapterInit", bare_bluetooth_linux_adapter_init)
  V("adapterDestroy", bare_bluetooth_linux_adapter_destroy)
  V("adapterGetPowered", bare_bluetooth_linux_adapter_get_powered)
  V("adapterSetPowered", bare_bluetooth_linux_adapter_set_powered)
  V("adapterGetDiscovering", bare_bluetooth_linux_adapter_get_discovering)
  V("adapterGetAddress", bare_bluetooth_linux_adapter_get_address)

#undef V

  return exports;
}

BARE_MODULE(bare_bluetooth_linux, bare_bluetooth_linux_exports)
