#include <assert.h>
#include <atomic>
#include <bare.h>
#include <dbus/dbus.h>
#include <js.h>
#include <jstl.h>
#include <l2cap.h>
#include <optional>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <type_traits>
#include <unordered_map>
#include <uv.h>
#include <vector>

#define BLUEZ_BUS                "org.bluez"
#define DBUS_PROP_IFACE          "org.freedesktop.DBus.Properties"
#define DBUS_OM_IFACE            "org.freedesktop.DBus.ObjectManager"
#define BLUEZ_ADAPTER_IFACE      "org.bluez.Adapter1"
#define BLUEZ_DEVICE_IFACE       "org.bluez.Device1"
#define BLUEZ_GATT_SERVICE_IFACE "org.bluez.GattService1"
#define BLUEZ_GATT_CHAR_IFACE    "org.bluez.GattCharacteristic1"
#define BLUEZ_GATT_DESC_IFACE    "org.bluez.GattDescriptor1"
#define BLUEZ_LE_ADV_MGR_IFACE   "org.bluez.LEAdvertisingManager1"
#define BLUEZ_LE_ADV_IFACE       "org.bluez.LEAdvertisement1"
#define BLUEZ_GATT_MGR_IFACE     "org.bluez.GattManager1"
#define BLUEZ_ADV_PATH           "/com/bare/advertisement0"
#define DBUS_TIMEOUT             2000
#define DBUS_CONNECT_TIMEOUT     30000
#define DBUS_POLL_INTERVAL       200

static std::optional<std::string>
dbus_get_string_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
  dbus_message_unref(msg);

  std::optional<std::string> result;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
      const char *val;
      dbus_message_iter_get_basic(&variant, &val);
      result = val;
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);

  return result;
}

static bool
dbus_get_bool_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
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
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);

  return result;
}

template <typename T>
static constexpr int
dbus_type_code() {
  if constexpr (std::is_same_v<T, int16_t>) return DBUS_TYPE_INT16;
  else if constexpr (std::is_same_v<T, uint16_t>) return DBUS_TYPE_UINT16;
  else if constexpr (std::is_same_v<T, int32_t>) return DBUS_TYPE_INT32;
  else if constexpr (std::is_same_v<T, uint32_t>) return DBUS_TYPE_UINT32;
  else if constexpr (std::is_same_v<T, int64_t>) return DBUS_TYPE_INT64;
  else if constexpr (std::is_same_v<T, uint64_t>) return DBUS_TYPE_UINT64;
}

template <typename T>
static std::optional<T>
dbus_get_numeric_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
  dbus_message_unref(msg);

  std::optional<T> result;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == dbus_type_code<T>()) {
      T val;
      dbus_message_iter_get_basic(&variant, &val);
      result = val;
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);

  return result;
}

static std::vector<std::string>
dbus_get_string_array_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
  dbus_message_unref(msg);

  std::vector<std::string> result;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant, array;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
      dbus_message_iter_recurse(&variant, &array);
      while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRING) {
        const char *val;
        dbus_message_iter_get_basic(&array, &val);
        result.push_back(val);
        dbus_message_iter_next(&array);
      }
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);

  return result;
}

static std::optional<std::string>
dbus_find_string_in_props(DBusMessageIter *props_iter, const char *target_prop) {
  while (dbus_message_iter_get_arg_type(props_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter prop_entry;
    dbus_message_iter_recurse(props_iter, &prop_entry);

    const char *prop_name;
    dbus_message_iter_get_basic(&prop_entry, &prop_name);

    if (strcmp(prop_name, target_prop) == 0) {
      dbus_message_iter_next(&prop_entry);
      DBusMessageIter variant;
      dbus_message_iter_recurse(&prop_entry, &variant);
      if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
        const char *val;
        dbus_message_iter_get_basic(&variant, &val);
        return std::string(val);
      }
    }

    dbus_message_iter_next(props_iter);
  }
  return std::nullopt;
}

static void
dbus_set_bool_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop, bool value) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Set");

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
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
  dbus_message_unref(msg);

  if (reply)
    dbus_message_unref(reply);
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);
}

static void
dbus_dict_append_string(DBusMessageIter *dict, const char *key, const char *val) {
  DBusMessageIter entry, variant;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &val);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static void
dbus_dict_append_int16(DBusMessageIter *dict, const char *key, int16_t val) {
  DBusMessageIter entry, variant;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "n", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_INT16, &val);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static void
dbus_dict_append_string_array(DBusMessageIter *dict, const char *key, const std::vector<std::string> &vals) {
  DBusMessageIter entry, variant, array;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "as", &variant);
  dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "s", &array);
  for (const auto &s : vals) {
    const char *val = s.c_str();
    dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &val);
  }
  dbus_message_iter_close_container(&variant, &array);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static DBusPendingCall *
dbus_call_void_method(DBusConnection *conn, const char *path, const char *iface, const char *method, int timeout = DBUS_TIMEOUT) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, iface, method);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(conn, msg, &pending, timeout);
  dbus_message_unref(msg);

  return pending;
}

static std::optional<std::string>
dbus_call_void_method_sync(DBusConnection *conn, const char *path, const char *iface, const char *method, int timeout = DBUS_TIMEOUT) {
  DBusPendingCall *pending = dbus_call_void_method(conn, path, iface, method, timeout);

  dbus_pending_call_block(pending);
  DBusMessage *reply = dbus_pending_call_steal_reply(pending);
  dbus_pending_call_unref(pending);

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    DBusError err;
    dbus_error_init(&err);
    dbus_set_error_from_message(&err, reply);
    std::string error = err.message;
    dbus_error_free(&err);
    dbus_message_unref(reply);
    return error;
  }

  dbus_message_unref(reply);
  return std::nullopt;
}

struct bare_bluetooth_linux_adapter_t;

struct bare_bluetooth_linux_device_added_event_t {
  std::string path;
  std::string address;
  std::string address_type;
};

struct bare_bluetooth_linux_device_removed_event_t {
  std::string path;
};

struct bare_bluetooth_linux_service_added_event_t {
  std::string path;
  std::string uuid;

  std::string
  device_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_service_removed_event_t {
  std::string path;

  std::string
  device_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_char_added_event_t {
  std::string path;
  std::string uuid;

  std::string
  service_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_char_removed_event_t {
  std::string path;

  std::string
  service_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_desc_added_event_t {
  std::string path;
  std::string uuid;

  std::string
  char_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_desc_removed_event_t {
  std::string path;

  std::string
  char_path() const { return path.substr(0, path.rfind('/')); }
};

struct bare_bluetooth_linux_char_value_event_t {
  std::string path;
  std::vector<uint8_t> value;
};

struct bare_bluetooth_linux_device_props_changed_event_t {
  std::string path;
  std::optional<bool> connected;
  std::optional<bool> paired;
  std::optional<bool> services_resolved;
  std::optional<int32_t> rssi;
  std::optional<std::string> name;
};

struct bare_bluetooth_linux_adapter_props_changed_event_t {
  std::optional<bool> powered;
  std::optional<bool> discovering;
};

struct bare_bluetooth_linux_async_call_t {
  js_env_t *env;
  js_persistent_t<js_function_t<void, js_object_t>> cb;
  bare_bluetooth_linux_adapter_t *adapter;
  std::optional<std::string> error;
};

struct bare_bluetooth_linux_async_read_call_t {
  js_env_t *env;
  js_persistent_t<js_function_t<void, js_object_t, js_arraybuffer_t>> cb;
  bare_bluetooth_linux_adapter_t *adapter;
  std::optional<std::string> error;
  std::vector<uint8_t> data;
};

static void
bare_bluetooth_linux__noop(js_env_t *env, js_receiver_t) {}

using bare_bluetooth_linux__noop_fn =
  js_function_t<void, js_receiver_t>;

using bare_bluetooth_linux__on_device_added_fn =
  js_function_t<void, js_receiver_t, std::string, std::string, std::string>;

using bare_bluetooth_linux__on_device_removed_fn =
  js_function_t<void, js_receiver_t, std::string>;

using bare_bluetooth_linux__on_service_added_fn =
  js_function_t<void, js_receiver_t, std::string, std::string, std::string>;

using bare_bluetooth_linux__on_service_removed_fn =
  js_function_t<void, js_receiver_t, std::string, std::string>;

using bare_bluetooth_linux__on_char_added_fn =
  js_function_t<void, js_receiver_t, std::string, std::string, std::string>;

using bare_bluetooth_linux__on_char_removed_fn =
  js_function_t<void, js_receiver_t, std::string, std::string>;

using bare_bluetooth_linux__on_desc_added_fn =
  js_function_t<void, js_receiver_t, std::string, std::string, std::string>;

using bare_bluetooth_linux__on_desc_removed_fn =
  js_function_t<void, js_receiver_t, std::string, std::string>;

using bare_bluetooth_linux__on_char_value_fn =
  js_function_t<void, js_receiver_t, std::string, js_arraybuffer_t>;

using bare_bluetooth_linux__on_device_props_changed_fn =
  js_function_t<void, js_receiver_t, std::string, std::optional<bool>, std::optional<bool>, std::optional<bool>, std::optional<int32_t>, std::optional<std::string>>;

using bare_bluetooth_linux__on_adapter_props_changed_fn =
  js_function_t<void, js_receiver_t, std::optional<bool>, std::optional<bool>>;

using bare_bluetooth_linux__on_adv_released_fn =
  js_function_t<void, js_receiver_t>;

struct bare_bluetooth_linux_adv_released_event_t {};

struct bare_bluetooth_linux_advertisement_t {
  std::string type;
  std::optional<std::string> local_name;
  std::vector<std::string> service_uuids;
};

struct bare_bluetooth_linux_local_characteristic_t {
  std::string uuid;
  std::vector<std::string> flags;
  std::vector<uint8_t> value;
  std::string path;
  std::string service_path;
};

struct bare_bluetooth_linux_local_service_t {
  std::string uuid;
  bool primary;
  std::string path;
  std::vector<bare_bluetooth_linux_local_characteristic_t> characteristics;
};

struct bare_bluetooth_linux_gatt_app_t {
  std::string path;
  std::vector<bare_bluetooth_linux_local_service_t> services;
  std::unordered_map<std::string, bare_bluetooth_linux_local_service_t *> service_map;
  std::unordered_map<std::string, bare_bluetooth_linux_local_characteristic_t *> characteristic_map;
};

struct bare_bluetooth_linux_gatt_characteristic_write_event_t {
  std::string path;
  std::vector<uint8_t> value;
};

using bare_bluetooth_linux__on_gatt_characteristic_write_fn =
  js_function_t<void, js_receiver_t, std::string, js_arraybuffer_t>;

struct bare_bluetooth_linux_adapter_t {
  DBusConnection *conn;
  DBusConnection *signal_conn;
  std::string adapter_path;
  std::atomic<bool> running;
  std::atomic<int> tsfn_count;
  uv_thread_t thread;
  uv_async_t cleanup_async;
  js_ref_t *ctx;
  js_threadsafe_function_t *tsfn_device_added;
  js_threadsafe_function_t *tsfn_device_removed;
  js_threadsafe_function_t *tsfn_method_reply;
  js_threadsafe_function_t *tsfn_read_reply;
  js_threadsafe_function_t *tsfn_service_added;
  js_threadsafe_function_t *tsfn_service_removed;
  js_threadsafe_function_t *tsfn_char_added;
  js_threadsafe_function_t *tsfn_char_removed;
  js_threadsafe_function_t *tsfn_desc_added;
  js_threadsafe_function_t *tsfn_desc_removed;
  js_threadsafe_function_t *tsfn_char_value;
  js_threadsafe_function_t *tsfn_device_props_changed;
  js_threadsafe_function_t *tsfn_adapter_props_changed;
  js_threadsafe_function_t *tsfn_adv_released;
  js_threadsafe_function_t *tsfn_gatt_characteristic_write;
  bare_bluetooth_linux_advertisement_t adv;
  bare_bluetooth_linux_gatt_app_t gatt_app;
};

// Exactly one context per threadsafe function, deleted by the finalizer, so
// counting them here keeps the finalize count in step with however many
// threadsafe functions the adapter ends up creating
struct bare_bluetooth_linux_tsfn_ctx_t {
  bare_bluetooth_linux_adapter_t *adapter;

  bare_bluetooth_linux_tsfn_ctx_t(bare_bluetooth_linux_adapter_t *adapter)
      : adapter(adapter) {
    adapter->tsfn_count.fetch_add(1);
  }
};

static void
bare_bluetooth_linux__on_device_added(
  js_env_t *env,
  bare_bluetooth_linux__on_device_added_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_device_added_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->address, event->address_type);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_device_removed(
  js_env_t *env,
  bare_bluetooth_linux__on_device_removed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_device_removed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_service_added(
  js_env_t *env,
  bare_bluetooth_linux__on_service_added_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_service_added_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->device_path(), event->uuid);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_service_removed(
  js_env_t *env,
  bare_bluetooth_linux__on_service_removed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_service_removed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->device_path());

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_char_added(
  js_env_t *env,
  bare_bluetooth_linux__on_char_added_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_char_added_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->service_path(), event->uuid);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_char_removed(
  js_env_t *env,
  bare_bluetooth_linux__on_char_removed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_char_removed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->service_path());

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_desc_added(
  js_env_t *env,
  bare_bluetooth_linux__on_desc_added_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_desc_added_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->char_path(), event->uuid);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_desc_removed(
  js_env_t *env,
  bare_bluetooth_linux__on_desc_removed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_desc_removed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->char_path());

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_char_value(
  js_env_t *env,
  bare_bluetooth_linux__on_char_value_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_char_value_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_arraybuffer_t buffer;
  err = js_create_arraybuffer(env, event->value, buffer);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, buffer);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_device_props_changed(
  js_env_t *env,
  bare_bluetooth_linux__on_device_props_changed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_device_props_changed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->connected, event->paired, event->services_resolved, event->rssi, event->name);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_adapter_props_changed(
  js_env_t *env,
  bare_bluetooth_linux__on_adapter_props_changed_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_adapter_props_changed_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->powered, event->discovering);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_adv_released(
  js_env_t *env,
  bare_bluetooth_linux__on_adv_released_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_adv_released_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver));

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux__on_gatt_characteristic_write(
  js_env_t *env,
  bare_bluetooth_linux__on_gatt_characteristic_write_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_gatt_characteristic_write_event_t *event
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ctx->adapter->ctx, &receiver);
  assert(err == 0);

  js_arraybuffer_t buffer;
  err = js_create_arraybuffer(env, event->value, buffer);
  assert(err == 0);

  js_call_function(env, function, js_receiver_t(receiver), event->path, buffer);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

#define DBUS_EXTRACT_ERROR(reply, target) \
  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) { \
    DBusError _err; \
    dbus_error_init(&_err); \
    dbus_set_error_from_message(&_err, reply); \
    (target) = std::string(_err.message); \
    dbus_error_free(&_err); \
  }

static void
bare_bluetooth_linux__on_gatt_unregister_notify(DBusPendingCall *pending, void *data) {
  auto *call = static_cast<bare_bluetooth_linux_async_call_t *>(data);

  DBusMessage *reply = dbus_pending_call_steal_reply(pending);
  DBUS_EXTRACT_ERROR(reply, call->error);

  dbus_message_unref(reply);
  dbus_pending_call_unref(pending);

  auto *adapter = call->adapter;
  if (!adapter->gatt_app.path.empty()) {
    dbus_connection_unregister_object_path(adapter->signal_conn, adapter->gatt_app.path.c_str());
    adapter->gatt_app.path.clear();
    adapter->gatt_app.services.clear();
    adapter->gatt_app.service_map.clear();
    adapter->gatt_app.characteristic_map.clear();
  }

  js_call_threadsafe_function(adapter->tsfn_method_reply, call, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_linux__on_pending_call_notify(DBusPendingCall *pending, void *data) {
  auto *call = static_cast<bare_bluetooth_linux_async_call_t *>(data);

  DBusMessage *reply = dbus_pending_call_steal_reply(pending);
  DBUS_EXTRACT_ERROR(reply, call->error);

  dbus_message_unref(reply);
  dbus_pending_call_unref(pending);

  js_call_threadsafe_function(call->adapter->tsfn_method_reply, call, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_linux__on_method_reply(
  js_env_t *env,
  bare_bluetooth_linux__noop_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_async_call_t *call
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_function_t<void, js_object_t> callback;
  err = js_get_reference_value(env, call->cb, callback);
  assert(err == 0);

  js_object_t error;

  if (call->error) {
    err = js_create_error(env, call->error->c_str(), error);
    assert(err == 0);
  } else {
    err = js_get_null(env, error);
    assert(err == 0);
  }

  err = js_call_function_with_checkpoint(env, callback, error);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  delete call;
}

static void
bare_bluetooth_linux__on_pending_read_call_notify(DBusPendingCall *pending, void *data) {
  auto *call = static_cast<bare_bluetooth_linux_async_read_call_t *>(data);

  DBusMessage *reply = dbus_pending_call_steal_reply(pending);
  DBUS_EXTRACT_ERROR(reply, call->error);

  if (!call->error) {
    DBusMessageIter reply_iter;
    dbus_message_iter_init(reply, &reply_iter);

    if (dbus_message_iter_get_arg_type(&reply_iter) == DBUS_TYPE_ARRAY) {
      DBusMessageIter array_iter;
      dbus_message_iter_recurse(&reply_iter, &array_iter);

      const uint8_t *bytes;
      int len;
      dbus_message_iter_get_fixed_array(&array_iter, &bytes, &len);
      call->data.assign(bytes, bytes + len);
    }
  }

  dbus_message_unref(reply);
  dbus_pending_call_unref(pending);

  js_call_threadsafe_function(call->adapter->tsfn_read_reply, call, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_linux__on_read_reply(
  js_env_t *env,
  bare_bluetooth_linux__noop_fn function,
  bare_bluetooth_linux_tsfn_ctx_t *ctx,
  bare_bluetooth_linux_async_read_call_t *call
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_function_t<void, js_object_t, js_arraybuffer_t> callback;
  err = js_get_reference_value(env, call->cb, callback);
  assert(err == 0);

  js_object_t error;

  if (call->error) {
    err = js_create_error(env, call->error->c_str(), error);
    assert(err == 0);
  } else {
    err = js_get_null(env, error);
    assert(err == 0);
  }

  js_arraybuffer_t buffer;
  err = js_create_arraybuffer(env, call->data, buffer);
  assert(err == 0);

  err = js_call_function_with_checkpoint(env, callback, error, buffer);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  delete call;
}

static void
bare_bluetooth_linux__on_tsfn_finalize(js_env_t *env, bare_bluetooth_linux_tsfn_ctx_t *ctx) {
  auto *adapter = ctx->adapter;
  delete ctx;

  if (adapter->tsfn_count.fetch_sub(1) > 1) return;

  int err = js_delete_reference(env, adapter->ctx);
  assert(err == 0);

  dbus_connection_close(adapter->signal_conn);
  dbus_connection_unref(adapter->signal_conn);

  dbus_connection_close(adapter->conn);
  dbus_connection_unref(adapter->conn);

  adapter->conn = nullptr;
  adapter->signal_conn = nullptr;

  adapter->~bare_bluetooth_linux_adapter_t();
}

static void
bare_bluetooth_linux__scan_object(bare_bluetooth_linux_adapter_t *adapter, const char *obj_path, DBusMessageIter *ifaces_ptr) {
  if (strncmp(obj_path, adapter->adapter_path.c_str(), adapter->adapter_path.length()) != 0)
    return;

  DBusMessageIter ifaces_iter = *ifaces_ptr;

  bool is_device = false;
  std::string address;
  std::string address_type;

  bool is_service = false;
  std::string service_uuid;

  bool is_char = false;
  std::string char_uuid;

  bool is_desc = false;
  std::string desc_uuid;

  while (dbus_message_iter_get_arg_type(&ifaces_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(&ifaces_iter, &entry);

    const char *iface_name;
    dbus_message_iter_get_basic(&entry, &iface_name);

    if (strcmp(iface_name, BLUEZ_DEVICE_IFACE) == 0) {
      dbus_message_iter_next(&entry);
      DBusMessageIter props_iter;
      dbus_message_iter_recurse(&entry, &props_iter);
      DBusMessageIter props_copy = props_iter;
      auto addr = dbus_find_string_in_props(&props_iter, "Address");
      if (addr) {
        is_device = true;
        address = *addr;
        auto type = dbus_find_string_in_props(&props_copy, "AddressType");
        if (type) address_type = *type;
      }

    } else if (strcmp(iface_name, BLUEZ_GATT_SERVICE_IFACE) == 0) {
      dbus_message_iter_next(&entry);
      DBusMessageIter props_iter;
      dbus_message_iter_recurse(&entry, &props_iter);
      auto svc_uuid = dbus_find_string_in_props(&props_iter, "UUID");
      if (svc_uuid) {
        is_service = true;
        service_uuid = *svc_uuid;
      }

    } else if (strcmp(iface_name, BLUEZ_GATT_CHAR_IFACE) == 0) {
      dbus_message_iter_next(&entry);
      DBusMessageIter props_iter;
      dbus_message_iter_recurse(&entry, &props_iter);
      auto cuuid = dbus_find_string_in_props(&props_iter, "UUID");
      if (cuuid) {
        is_char = true;
        char_uuid = *cuuid;
      }

    } else if (strcmp(iface_name, BLUEZ_GATT_DESC_IFACE) == 0) {
      dbus_message_iter_next(&entry);
      DBusMessageIter props_iter;
      dbus_message_iter_recurse(&entry, &props_iter);
      auto duuid = dbus_find_string_in_props(&props_iter, "UUID");
      if (duuid) {
        is_desc = true;
        desc_uuid = *duuid;
      }
    }

    dbus_message_iter_next(&ifaces_iter);
  }

  if (is_device && !address.empty()) {
    auto *event = new bare_bluetooth_linux_device_added_event_t;
    event->path = obj_path;
    event->address = address;
    event->address_type = address_type;
    js_call_threadsafe_function(adapter->tsfn_device_added, event, js_threadsafe_function_nonblocking);
  }

  if (is_service && !service_uuid.empty()) {
    auto *event = new bare_bluetooth_linux_service_added_event_t;
    event->path = obj_path;
    event->uuid = service_uuid;
    js_call_threadsafe_function(adapter->tsfn_service_added, event, js_threadsafe_function_nonblocking);
  }

  if (is_char && !char_uuid.empty()) {
    auto *event = new bare_bluetooth_linux_char_added_event_t;
    event->path = obj_path;
    event->uuid = char_uuid;
    js_call_threadsafe_function(adapter->tsfn_char_added, event, js_threadsafe_function_nonblocking);
  }

  if (is_desc && !desc_uuid.empty()) {
    auto *event = new bare_bluetooth_linux_desc_added_event_t;
    event->path = obj_path;
    event->uuid = desc_uuid;
    js_call_threadsafe_function(adapter->tsfn_desc_added, event, js_threadsafe_function_nonblocking);
  }
}

static void
bare_bluetooth_linux__on_interfaces_added(bare_bluetooth_linux_adapter_t *adapter, DBusMessage *msg) {
  DBusMessageIter args;
  if (!dbus_message_iter_init(msg, &args)) return;

  const char *obj_path;
  dbus_message_iter_get_basic(&args, &obj_path);
  dbus_message_iter_next(&args);

  DBusMessageIter ifaces_iter;
  dbus_message_iter_recurse(&args, &ifaces_iter);

  bare_bluetooth_linux__scan_object(adapter, obj_path, &ifaces_iter);
}

// Objects that existed before this adapter connected (bluetoothd caches
// discovered devices) never signal InterfacesAdded again; replay them
static void
bare_bluetooth_linux__sync_existing_objects(bare_bluetooth_linux_adapter_t *adapter) {
  DBusMessage *msg = dbus_message_new_method_call(BLUEZ_BUS, "/", DBUS_OM_IFACE, "GetManagedObjects");
  if (msg == nullptr) return;

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(adapter->conn, msg, DBUS_TIMEOUT, nullptr);
  dbus_message_unref(msg);
  if (reply == nullptr) return;

  DBusMessageIter args;
  if (dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
    DBusMessageIter objs;
    dbus_message_iter_recurse(&args, &objs);

    while (dbus_message_iter_get_arg_type(&objs) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter entry;
      dbus_message_iter_recurse(&objs, &entry);

      const char *obj_path;
      dbus_message_iter_get_basic(&entry, &obj_path);
      dbus_message_iter_next(&entry);

      DBusMessageIter ifaces_iter;
      dbus_message_iter_recurse(&entry, &ifaces_iter);

      bare_bluetooth_linux__scan_object(adapter, obj_path, &ifaces_iter);

      dbus_message_iter_next(&objs);
    }
  }

  dbus_message_unref(reply);
}

static void
bare_bluetooth_linux__on_interfaces_removed(bare_bluetooth_linux_adapter_t *adapter, DBusMessage *msg) {
  DBusMessageIter args;
  if (!dbus_message_iter_init(msg, &args)) return;

  const char *obj_path;
  dbus_message_iter_get_basic(&args, &obj_path);
  dbus_message_iter_next(&args);

  if (strncmp(obj_path, adapter->adapter_path.c_str(), adapter->adapter_path.length()) != 0)
    return;

  DBusMessageIter ifaces_iter;
  dbus_message_iter_recurse(&args, &ifaces_iter);

  bool is_device = false;
  bool is_service = false;
  bool is_char = false;
  bool is_desc = false;

  while (dbus_message_iter_get_arg_type(&ifaces_iter) == DBUS_TYPE_STRING) {
    const char *iface_name;
    dbus_message_iter_get_basic(&ifaces_iter, &iface_name);
    if (strcmp(iface_name, BLUEZ_DEVICE_IFACE) == 0) {
      is_device = true;
    } else if (strcmp(iface_name, BLUEZ_GATT_SERVICE_IFACE) == 0) {
      is_service = true;
    } else if (strcmp(iface_name, BLUEZ_GATT_CHAR_IFACE) == 0) {
      is_char = true;
    } else if (strcmp(iface_name, BLUEZ_GATT_DESC_IFACE) == 0) {
      is_desc = true;
    }
    dbus_message_iter_next(&ifaces_iter);
  }

  if (is_device) {
    auto *event = new bare_bluetooth_linux_device_removed_event_t;
    event->path = obj_path;
    js_call_threadsafe_function(adapter->tsfn_device_removed, event, js_threadsafe_function_nonblocking);
  }

  if (is_service) {
    auto *event = new bare_bluetooth_linux_service_removed_event_t;
    event->path = obj_path;
    js_call_threadsafe_function(adapter->tsfn_service_removed, event, js_threadsafe_function_nonblocking);
  }

  if (is_char) {
    auto *event = new bare_bluetooth_linux_char_removed_event_t;
    event->path = obj_path;
    js_call_threadsafe_function(adapter->tsfn_char_removed, event, js_threadsafe_function_nonblocking);
  }

  if (is_desc) {
    auto *event = new bare_bluetooth_linux_desc_removed_event_t;
    event->path = obj_path;
    js_call_threadsafe_function(adapter->tsfn_desc_removed, event, js_threadsafe_function_nonblocking);
  }
}

static void
bare_bluetooth_linux__on_device_props_changed_signal(bare_bluetooth_linux_adapter_t *adapter, const char *obj_path, DBusMessageIter *props_iter) {
  auto *event = new bare_bluetooth_linux_device_props_changed_event_t;
  event->path = obj_path;
  bool has_changes = false;

  while (dbus_message_iter_get_arg_type(props_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(props_iter, &entry);

    const char *prop_name;
    dbus_message_iter_get_basic(&entry, &prop_name);
    dbus_message_iter_next(&entry);

    DBusMessageIter variant;
    dbus_message_iter_recurse(&entry, &variant);

    if (strcmp(prop_name, "Connected") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->connected = val;
      has_changes = true;
    } else if (strcmp(prop_name, "Paired") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->paired = val;
      has_changes = true;
    } else if (strcmp(prop_name, "ServicesResolved") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->services_resolved = val;
      has_changes = true;
    } else if (strcmp(prop_name, "RSSI") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_INT16) {
      int16_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->rssi = val;
      has_changes = true;
    } else if (strcmp(prop_name, "Name") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
      const char *val;
      dbus_message_iter_get_basic(&variant, &val);
      event->name = val;
      has_changes = true;
    }

    dbus_message_iter_next(props_iter);
  }

  if (has_changes) {
    js_call_threadsafe_function(adapter->tsfn_device_props_changed, event, js_threadsafe_function_nonblocking);
  } else {
    // No tracked properties changed; free the event since it won't be consumed by the tsfn callback
    delete event;
  }
}

static void
bare_bluetooth_linux__on_adapter_props_changed_signal(bare_bluetooth_linux_adapter_t *adapter, DBusMessageIter *props_iter) {
  auto *event = new bare_bluetooth_linux_adapter_props_changed_event_t;
  bool has_changes = false;

  while (dbus_message_iter_get_arg_type(props_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(props_iter, &entry);

    const char *prop_name;
    dbus_message_iter_get_basic(&entry, &prop_name);
    dbus_message_iter_next(&entry);

    DBusMessageIter variant;
    dbus_message_iter_recurse(&entry, &variant);

    if (strcmp(prop_name, "Powered") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->powered = val;
      has_changes = true;
    } else if (strcmp(prop_name, "Discovering") == 0 && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_BOOLEAN) {
      dbus_bool_t val;
      dbus_message_iter_get_basic(&variant, &val);
      event->discovering = val;
      has_changes = true;
    }

    dbus_message_iter_next(props_iter);
  }

  if (has_changes) {
    js_call_threadsafe_function(adapter->tsfn_adapter_props_changed, event, js_threadsafe_function_nonblocking);
  } else {
    // No tracked properties changed; free the event since it won't be consumed by the tsfn callback
    delete event;
  }
}

static void
bare_bluetooth_linux__on_char_value_changed_signal(bare_bluetooth_linux_adapter_t *adapter, const char *obj_path, DBusMessageIter *props_iter) {
  while (dbus_message_iter_get_arg_type(props_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(props_iter, &entry);

    const char *prop_name;
    dbus_message_iter_get_basic(&entry, &prop_name);

    if (strcmp(prop_name, "Value") == 0) {
      dbus_message_iter_next(&entry);
      DBusMessageIter variant;
      dbus_message_iter_recurse(&entry, &variant);

      if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&variant, &array_iter);

        const uint8_t *data;
        int len;
        dbus_message_iter_get_fixed_array(&array_iter, &data, &len);

        auto *event = new bare_bluetooth_linux_char_value_event_t;
        event->path = obj_path;
        event->value.assign(data, data + len);
        js_call_threadsafe_function(adapter->tsfn_char_value, event, js_threadsafe_function_nonblocking);
      }
      break;
    }

    dbus_message_iter_next(props_iter);
  }
}

static DBusHandlerResult
bare_bluetooth_linux__advertisement_message_handler(
  DBusConnection *conn, DBusMessage *msg, void *user_data
) {
  auto *adapter = static_cast<bare_bluetooth_linux_adapter_t *>(user_data);

  if (dbus_message_is_method_call(msg, DBUS_PROP_IFACE, "GetAll")) {
    DBusMessage *reply = dbus_message_new_method_return(msg);
    DBusMessageIter iter, dict;
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

    dbus_dict_append_string(&dict, "Type", adapter->adv.type.c_str());

    if (adapter->adv.local_name) {
      dbus_dict_append_string(&dict, "LocalName", adapter->adv.local_name->c_str());
    }

    if (!adapter->adv.service_uuids.empty()) {
      dbus_dict_append_string_array(&dict, "ServiceUUIDs", adapter->adv.service_uuids);
    }

    dbus_message_iter_close_container(&iter, &dict);
    dbus_connection_send(conn, reply, nullptr);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(msg, BLUEZ_LE_ADV_IFACE, "Release")) {
    DBusMessage *reply = dbus_message_new_method_return(msg);
    dbus_connection_send(conn, reply, nullptr);
    dbus_message_unref(reply);

    auto *event = new bare_bluetooth_linux_adv_released_event_t;
    js_call_threadsafe_function(adapter->tsfn_adv_released, event, js_threadsafe_function_nonblocking);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable bare_bluetooth_linux__adv_vtable = {
  nullptr,
  bare_bluetooth_linux__advertisement_message_handler
};

static void
dbus_dict_append_bool(DBusMessageIter *dict, const char *key, dbus_bool_t val) {
  DBusMessageIter entry, variant;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &val);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static void
dbus_dict_append_object_path(DBusMessageIter *dict, const char *key, const char *val) {
  DBusMessageIter entry, variant;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "o", &variant);
  dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH, &val);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static void
dbus_dict_append_byte_array(DBusMessageIter *dict, const char *key, const std::vector<uint8_t> &data) {
  DBusMessageIter entry, variant, array;
  dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
  dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "ay", &variant);
  dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "y", &array);
  for (const auto &b : data) {
    dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &b);
  }
  dbus_message_iter_close_container(&variant, &array);
  dbus_message_iter_close_container(&entry, &variant);
  dbus_message_iter_close_container(dict, &entry);
}

static void
bare_bluetooth_linux__gatt_append_service_props(DBusMessageIter *dict, const bare_bluetooth_linux_local_service_t &svc, const char *uuid_key, const char *primary_key) {
  dbus_dict_append_string(dict, uuid_key, svc.uuid.c_str());
  dbus_bool_t primary = svc.primary ? TRUE : FALSE;
  dbus_dict_append_bool(dict, primary_key, primary);
}

static void
bare_bluetooth_linux__gatt_append_characteristic_props(DBusMessageIter *dict, const bare_bluetooth_linux_local_characteristic_t &ch, const char *uuid_key, const char *service_key, const char *flags_key, const char *value_key) {
  dbus_dict_append_string(dict, uuid_key, ch.uuid.c_str());
  dbus_dict_append_object_path(dict, service_key, ch.service_path.c_str());
  dbus_dict_append_string_array(dict, flags_key, ch.flags);
  dbus_dict_append_byte_array(dict, value_key, ch.value);
}

static DBusHandlerResult
bare_bluetooth_linux__gatt_message_handler(
  DBusConnection *conn, DBusMessage *msg, void *user_data
) {
  auto *adapter = static_cast<bare_bluetooth_linux_adapter_t *>(user_data);
  const char *path = dbus_message_get_path(msg);

  if (dbus_message_is_method_call(msg, DBUS_OM_IFACE, "GetManagedObjects")) {
    DBusMessage *reply = dbus_message_new_method_return(msg);
    DBusMessageIter iter, outer;
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{oa{sa{sv}}}", &outer);

    for (const auto &svc : adapter->gatt_app.services) {
      DBusMessageIter obj_entry, ifaces_dict, iface_entry, props_dict;
      const char *svc_path = svc.path.c_str();

      dbus_message_iter_open_container(&outer, DBUS_TYPE_DICT_ENTRY, nullptr, &obj_entry);
      dbus_message_iter_append_basic(&obj_entry, DBUS_TYPE_OBJECT_PATH, &svc_path);
      dbus_message_iter_open_container(&obj_entry, DBUS_TYPE_ARRAY, "{sa{sv}}", &ifaces_dict);

      const char *svc_iface = BLUEZ_GATT_SERVICE_IFACE;
      dbus_message_iter_open_container(&ifaces_dict, DBUS_TYPE_DICT_ENTRY, nullptr, &iface_entry);
      dbus_message_iter_append_basic(&iface_entry, DBUS_TYPE_STRING, &svc_iface);
      dbus_message_iter_open_container(&iface_entry, DBUS_TYPE_ARRAY, "{sv}", &props_dict);
      bare_bluetooth_linux__gatt_append_service_props(&props_dict, svc, "UUID", "Primary");
      dbus_message_iter_close_container(&iface_entry, &props_dict);
      dbus_message_iter_close_container(&ifaces_dict, &iface_entry);

      dbus_message_iter_close_container(&obj_entry, &ifaces_dict);
      dbus_message_iter_close_container(&outer, &obj_entry);

      for (const auto &ch : svc.characteristics) {
        DBusMessageIter ch_obj_entry, ch_ifaces_dict, ch_iface_entry, ch_props_dict;
        const char *ch_path = ch.path.c_str();

        dbus_message_iter_open_container(&outer, DBUS_TYPE_DICT_ENTRY, nullptr, &ch_obj_entry);
        dbus_message_iter_append_basic(&ch_obj_entry, DBUS_TYPE_OBJECT_PATH, &ch_path);
        dbus_message_iter_open_container(&ch_obj_entry, DBUS_TYPE_ARRAY, "{sa{sv}}", &ch_ifaces_dict);

        const char *ch_iface = BLUEZ_GATT_CHAR_IFACE;
        dbus_message_iter_open_container(&ch_ifaces_dict, DBUS_TYPE_DICT_ENTRY, nullptr, &ch_iface_entry);
        dbus_message_iter_append_basic(&ch_iface_entry, DBUS_TYPE_STRING, &ch_iface);
        dbus_message_iter_open_container(&ch_iface_entry, DBUS_TYPE_ARRAY, "{sv}", &ch_props_dict);
        bare_bluetooth_linux__gatt_append_characteristic_props(&ch_props_dict, ch, "UUID", "Service", "Flags", "Value");
        dbus_message_iter_close_container(&ch_iface_entry, &ch_props_dict);
        dbus_message_iter_close_container(&ch_ifaces_dict, &ch_iface_entry);

        dbus_message_iter_close_container(&ch_obj_entry, &ch_ifaces_dict);
        dbus_message_iter_close_container(&outer, &ch_obj_entry);
      }
    }

    dbus_message_iter_close_container(&iter, &outer);
    dbus_connection_send(conn, reply, nullptr);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(msg, DBUS_PROP_IFACE, "GetAll")) {
    auto svc_it = adapter->gatt_app.service_map.find(path);
    if (svc_it != adapter->gatt_app.service_map.end()) {
      DBusMessage *reply = dbus_message_new_method_return(msg);
      DBusMessageIter iter, dict;
      dbus_message_iter_init_append(reply, &iter);
      dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
      bare_bluetooth_linux__gatt_append_service_props(&dict, *svc_it->second, "UUID", "Primary");
      dbus_message_iter_close_container(&iter, &dict);
      dbus_connection_send(conn, reply, nullptr);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    auto ch_it = adapter->gatt_app.characteristic_map.find(path);
    if (ch_it != adapter->gatt_app.characteristic_map.end()) {
      DBusMessage *reply = dbus_message_new_method_return(msg);
      DBusMessageIter iter, dict;
      dbus_message_iter_init_append(reply, &iter);
      dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
      bare_bluetooth_linux__gatt_append_characteristic_props(&dict, *ch_it->second, "UUID", "Service", "Flags", "Value");
      dbus_message_iter_close_container(&iter, &dict);
      dbus_connection_send(conn, reply, nullptr);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_method_call(msg, BLUEZ_GATT_CHAR_IFACE, "ReadValue")) {
    auto it = adapter->gatt_app.characteristic_map.find(path);
    if (it != adapter->gatt_app.characteristic_map.end()) {
      auto &ch = *it->second;
      DBusMessage *reply = dbus_message_new_method_return(msg);
      DBusMessageIter iter, array;
      dbus_message_iter_init_append(reply, &iter);
      dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array);
      for (const auto &b : ch.value) {
        dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &b);
      }
      dbus_message_iter_close_container(&iter, &array);
      dbus_connection_send(conn, reply, nullptr);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_method_call(msg, BLUEZ_GATT_CHAR_IFACE, "WriteValue")) {
    auto it = adapter->gatt_app.characteristic_map.find(path);
    if (it != adapter->gatt_app.characteristic_map.end()) {
      auto &ch = *it->second;
      DBusMessageIter args;
      if (!dbus_message_iter_init(msg, &args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
        DBusMessage *error = dbus_message_new_error(msg, "org.bluez.Error.InvalidArguments", "Expected byte array");
        dbus_connection_send(conn, error, nullptr);
        dbus_message_unref(error);
        return DBUS_HANDLER_RESULT_HANDLED;
      }

      DBusMessageIter array_iter;
      dbus_message_iter_recurse(&args, &array_iter);

      const uint8_t *bytes;
      int len;
      dbus_message_iter_get_fixed_array(&array_iter, &bytes, &len);
      ch.value.assign(bytes, bytes + len);

      auto *event = new bare_bluetooth_linux_gatt_characteristic_write_event_t;
      event->path = path;
      event->value.assign(bytes, bytes + len);
      js_call_threadsafe_function(adapter->tsfn_gatt_characteristic_write, event, js_threadsafe_function_nonblocking);

      DBusMessage *reply = dbus_message_new_method_return(msg);
      dbus_connection_send(conn, reply, nullptr);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable bare_bluetooth_linux__gatt_vtable = {
  nullptr,
  bare_bluetooth_linux__gatt_message_handler
};

static DBusHandlerResult
bare_bluetooth_linux__signal_filter(DBusConnection *conn, DBusMessage *msg, void *data) {
  auto *adapter = static_cast<bare_bluetooth_linux_adapter_t *>(data);

  if (!adapter->running.load())
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_signal(msg, DBUS_OM_IFACE, "InterfacesAdded")) {
    bare_bluetooth_linux__on_interfaces_added(adapter, msg);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(msg, DBUS_OM_IFACE, "InterfacesRemoved")) {
    bare_bluetooth_linux__on_interfaces_removed(adapter, msg);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(msg, DBUS_PROP_IFACE, "PropertiesChanged")) {
    const char *obj_path = dbus_message_get_path(msg);
    if (!obj_path || strncmp(obj_path, adapter->adapter_path.c_str(), adapter->adapter_path.length()) != 0)
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    DBusMessageIter args;
    if (!dbus_message_iter_init(msg, &args))
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    const char *iface_name;
    dbus_message_iter_get_basic(&args, &iface_name);
    dbus_message_iter_next(&args);

    DBusMessageIter props_iter;
    dbus_message_iter_recurse(&args, &props_iter);

    if (strcmp(iface_name, BLUEZ_ADAPTER_IFACE) == 0) {
      // Only the adapter object itself, not the devices hanging beneath it
      if (strcmp(obj_path, adapter->adapter_path.c_str()) != 0)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

      bare_bluetooth_linux__on_adapter_props_changed_signal(adapter, &props_iter);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(iface_name, BLUEZ_DEVICE_IFACE) == 0) {
      bare_bluetooth_linux__on_device_props_changed_signal(adapter, obj_path, &props_iter);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(iface_name, BLUEZ_GATT_CHAR_IFACE) == 0) {
      bare_bluetooth_linux__on_char_value_changed_signal(adapter, obj_path, &props_iter);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
bare_bluetooth_linux__on_cleanup_close(uv_handle_t *handle) {
  (void) handle;
}

static void
bare_bluetooth_linux__on_cleanup(uv_async_t *async) {
  auto *adapter = static_cast<bare_bluetooth_linux_adapter_t *>(async->data);

  uv_thread_join(&adapter->thread);

  int err;

  err = js_release_threadsafe_function(adapter->tsfn_device_added, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_device_removed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_method_reply, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_read_reply, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_service_added, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_service_removed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_char_added, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_char_removed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_desc_added, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_desc_removed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_char_value, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_device_props_changed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_adapter_props_changed, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_adv_released, js_threadsafe_function_release);
  assert(err == 0);

  err = js_release_threadsafe_function(adapter->tsfn_gatt_characteristic_write, js_threadsafe_function_release);
  assert(err == 0);

  uv_close(reinterpret_cast<uv_handle_t *>(async), bare_bluetooth_linux__on_cleanup_close);
}

static void
bare_bluetooth_linux__dbus_thread(void *data) {
  auto *adapter = static_cast<bare_bluetooth_linux_adapter_t *>(data);

  while (adapter->running.load()) {
    if (!dbus_connection_read_write_dispatch(adapter->signal_conn, DBUS_POLL_INTERVAL)) break;
  }

  uv_async_send(&adapter->cleanup_async);
}

static js_arraybuffer_t
bare_bluetooth_linux_adapter_init(
  js_env_t *env,
  js_receiver_t,
  std::string path,
  js_object_t context,
  bare_bluetooth_linux__on_device_added_fn on_device_added,
  bare_bluetooth_linux__on_device_removed_fn on_device_removed,
  bare_bluetooth_linux__on_service_added_fn on_service_added,
  bare_bluetooth_linux__on_service_removed_fn on_service_removed,
  bare_bluetooth_linux__on_char_added_fn on_char_added,
  bare_bluetooth_linux__on_char_removed_fn on_char_removed,
  bare_bluetooth_linux__on_desc_added_fn on_desc_added,
  bare_bluetooth_linux__on_desc_removed_fn on_desc_removed,
  bare_bluetooth_linux__on_char_value_fn on_char_value,
  bare_bluetooth_linux__on_device_props_changed_fn on_device_props_changed,
  bare_bluetooth_linux__on_adapter_props_changed_fn on_adapter_props_changed,
  bare_bluetooth_linux__on_adv_released_fn on_adv_released,
  bare_bluetooth_linux__on_gatt_characteristic_write_fn on_gatt_characteristic_write
) {
  dbus_threads_init_default();

  js_arraybuffer_t handle;
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_create_arraybuffer(env, adapter, handle);
  assert(err == 0);

  new (adapter) bare_bluetooth_linux_adapter_t();

  adapter->adapter_path = path;
  adapter->running.store(true);
  adapter->tsfn_count.store(0);

  err = js_create_reference(env, static_cast<js_value_t *>(context), 1, &adapter->ctx);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_async_init(loop, &adapter->cleanup_async, bare_bluetooth_linux__on_cleanup);
  assert(err == 0);

  adapter->cleanup_async.data = adapter;

  DBusError dbus_err;

  // TODO: handle D-Bus errors and throw to JS
  dbus_error_init(&dbus_err);
  adapter->conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &dbus_err);
  assert(!dbus_error_is_set(&dbus_err));
  dbus_connection_set_exit_on_disconnect(adapter->conn, FALSE);

  // TODO: handle D-Bus errors and throw to JS
  dbus_error_init(&dbus_err);
  adapter->signal_conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &dbus_err);
  assert(!dbus_error_is_set(&dbus_err));
  dbus_connection_set_exit_on_disconnect(adapter->signal_conn, FALSE);

  dbus_bus_add_match(
    adapter->signal_conn,
    "type='signal',sender='org.bluez',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'",
    nullptr
  );
  dbus_bus_add_match(
    adapter->signal_conn,
    "type='signal',sender='org.bluez',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved'",
    nullptr
  );
  dbus_bus_add_match(
    adapter->signal_conn,
    "type='signal',sender='org.bluez',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'",
    nullptr
  );
  dbus_connection_flush(adapter->signal_conn);

  dbus_connection_add_filter(adapter->signal_conn, bare_bluetooth_linux__signal_filter, adapter, nullptr);

  auto *added_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_device_added,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_device_added_event_t>(env, on_device_added, 0, 1, added_ctx, adapter->tsfn_device_added);
  assert(err == 0);

  auto *removed_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_device_removed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_device_removed_event_t>(env, on_device_removed, 0, 1, removed_ctx, adapter->tsfn_device_removed);
  assert(err == 0);

  bare_bluetooth_linux__noop_fn noop;
  err = js_create_function<bare_bluetooth_linux__noop>(env, noop);
  assert(err == 0);

  auto *reply_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_method_reply,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_async_call_t>(env, noop, 0, 1, reply_ctx, adapter->tsfn_method_reply);
  assert(err == 0);

  auto *read_reply_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_read_reply,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_async_read_call_t>(env, noop, 0, 1, read_reply_ctx, adapter->tsfn_read_reply);
  assert(err == 0);

  auto *service_added_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_service_added,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_service_added_event_t>(env, on_service_added, 0, 1, service_added_ctx, adapter->tsfn_service_added);
  assert(err == 0);

  auto *service_removed_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_service_removed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_service_removed_event_t>(env, on_service_removed, 0, 1, service_removed_ctx, adapter->tsfn_service_removed);
  assert(err == 0);

  auto *char_added_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_char_added,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_char_added_event_t>(env, on_char_added, 0, 1, char_added_ctx, adapter->tsfn_char_added);
  assert(err == 0);

  auto *char_removed_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_char_removed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_char_removed_event_t>(env, on_char_removed, 0, 1, char_removed_ctx, adapter->tsfn_char_removed);
  assert(err == 0);

  auto *desc_added_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_desc_added,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_desc_added_event_t>(env, on_desc_added, 0, 1, desc_added_ctx, adapter->tsfn_desc_added);
  assert(err == 0);

  auto *desc_removed_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_desc_removed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_desc_removed_event_t>(env, on_desc_removed, 0, 1, desc_removed_ctx, adapter->tsfn_desc_removed);
  assert(err == 0);

  auto *char_value_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_char_value,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_char_value_event_t>(env, on_char_value, 0, 1, char_value_ctx, adapter->tsfn_char_value);
  assert(err == 0);

  auto *device_props_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_device_props_changed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_device_props_changed_event_t>(env, on_device_props_changed, 0, 1, device_props_ctx, adapter->tsfn_device_props_changed);
  assert(err == 0);

  auto *adapter_props_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_adapter_props_changed,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_adapter_props_changed_event_t>(env, on_adapter_props_changed, 0, 1, adapter_props_ctx, adapter->tsfn_adapter_props_changed);
  assert(err == 0);

  auto *adv_released_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_adv_released,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_adv_released_event_t>(env, on_adv_released, 0, 1, adv_released_ctx, adapter->tsfn_adv_released);
  assert(err == 0);

  auto *gatt_characteristic_write_ctx = new bare_bluetooth_linux_tsfn_ctx_t{adapter};
  err = js_create_threadsafe_function<
    bare_bluetooth_linux__on_gatt_characteristic_write,
    bare_bluetooth_linux__on_tsfn_finalize,
    bare_bluetooth_linux_tsfn_ctx_t,
    bare_bluetooth_linux_gatt_characteristic_write_event_t>(env, on_gatt_characteristic_write, 0, 1, gatt_characteristic_write_ctx, adapter->tsfn_gatt_characteristic_write);
  assert(err == 0);

  bare_bluetooth_linux__sync_existing_objects(adapter);

  uv_thread_create(&adapter->thread, bare_bluetooth_linux__dbus_thread, adapter);

  return handle;
}

static void
bare_bluetooth_linux_adapter_destroy(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  if (!adapter->running.load()) return;

  adapter->running.store(false);
}

static bool
bare_bluetooth_linux_adapter_get_powered(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  return dbus_get_bool_prop(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "Powered");
}

static void
bare_bluetooth_linux_adapter_set_powered(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, bool value
) {
  dbus_set_bool_prop(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "Powered", value);
}

static bool
bare_bluetooth_linux_adapter_get_discovering(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  return dbus_get_bool_prop(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "Discovering");
}

static std::optional<std::string>
bare_bluetooth_linux_adapter_get_address(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  return dbus_get_string_prop(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "Address");
}

static void
bare_bluetooth_linux_adapter_set_discovery_filter(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::vector<std::string> uuids, std::optional<int32_t> rssi, std::optional<std::string> transport
) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "SetDiscoveryFilter");

  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

  if (!uuids.empty()) dbus_dict_append_string_array(&dict, "UUIDs", uuids);
  if (rssi) dbus_dict_append_int16(&dict, "RSSI", static_cast<int16_t>(*rssi));
  if (transport) dbus_dict_append_string(&dict, "Transport", transport->c_str());

  dbus_message_iter_close_container(&iter, &dict);

  DBusError dbus_err;
  dbus_error_init(&dbus_err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(adapter->conn, msg, DBUS_TIMEOUT, &dbus_err);
  dbus_message_unref(msg);

  if (reply) dbus_message_unref(reply);

  if (dbus_error_is_set(&dbus_err)) {
    int err = js_throw_error(env, nullptr, dbus_err.message);
    assert(err == 0);
    dbus_error_free(&dbus_err);
  }
}

static void
bare_bluetooth_linux_adapter_start_discovery(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  auto error = dbus_call_void_method_sync(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "StartDiscovery");
  if (error) {
    int err = js_throw_error(env, nullptr, error->c_str());
    assert(err == 0);
  }
}

static void
bare_bluetooth_linux_adapter_stop_discovery(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  auto error = dbus_call_void_method_sync(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "StopDiscovery");
  if (error) {
    int err = js_throw_error(env, nullptr, error->c_str());
    assert(err == 0);
  }
}

static std::optional<std::string>
bare_bluetooth_linux_device_get_address(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "Address");
}

static std::optional<std::string>
bare_bluetooth_linux_device_get_address_type(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "AddressType");
}

static std::optional<std::string>
bare_bluetooth_linux_device_get_name(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "Name");
}

static std::optional<int32_t>
bare_bluetooth_linux_device_get_rssi(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_numeric_prop<int16_t>(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "RSSI");
}

static bool
bare_bluetooth_linux_device_get_paired(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_bool_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "Paired");
}

static bool
bare_bluetooth_linux_device_get_connected(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_bool_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "Connected");
}

static bool
bare_bluetooth_linux_device_get_services_resolved(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_bool_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "ServicesResolved");
}

static std::vector<std::string>
bare_bluetooth_linux_device_get_uuids(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_array_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "UUIDs");
}

static void
bare_bluetooth_linux_device_get_manufacturer_data(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_object_t result
) {
  int err;

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), DBUS_PROP_IFACE, "Get");
  const char *iface = BLUEZ_DEVICE_IFACE;
  const char *prop = "ManufacturerData";
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError dbus_err;
  dbus_error_init(&dbus_err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(adapter->conn, msg, DBUS_TIMEOUT, &dbus_err);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&dbus_err)) {
    if (dbus_error_is_set(&dbus_err)) dbus_error_free(&dbus_err);
    return;
  }

  DBusMessageIter iter, variant, dict;
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse(&iter, &variant);

  if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
    dbus_message_iter_recurse(&variant, &dict);

    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter entry;
      dbus_message_iter_recurse(&dict, &entry);

      uint16_t company_id;
      dbus_message_iter_get_basic(&entry, &company_id);
      dbus_message_iter_next(&entry);

      DBusMessageIter val_variant;
      dbus_message_iter_recurse(&entry, &val_variant);

      if (dbus_message_iter_get_arg_type(&val_variant) == DBUS_TYPE_ARRAY) {
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&val_variant, &array_iter);

        const uint8_t *bytes;
        int len;
        dbus_message_iter_get_fixed_array(&array_iter, &bytes, &len);

        if (len > 0) {
          js_value_t *buffer;
          void *data;
          err = js_create_arraybuffer(env, len, &data, &buffer);
          assert(err == 0);
          memcpy(data, bytes, len);

          char key[16];
          snprintf(key, sizeof(key), "%u", company_id);
          err = js_set_named_property(env, static_cast<js_value_t *>(result), key, buffer);
          assert(err == 0);
        }
      }

      dbus_message_iter_next(&dict);
    }
  }

  dbus_message_unref(reply);
}

static void
bare_bluetooth_linux_device_get_service_data(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_object_t result
) {
  int err;

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), DBUS_PROP_IFACE, "Get");
  const char *iface = BLUEZ_DEVICE_IFACE;
  const char *prop = "ServiceData";
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError dbus_err;
  dbus_error_init(&dbus_err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(adapter->conn, msg, DBUS_TIMEOUT, &dbus_err);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&dbus_err)) {
    if (dbus_error_is_set(&dbus_err)) dbus_error_free(&dbus_err);
    return;
  }

  DBusMessageIter iter, variant, dict;
  dbus_message_iter_init(reply, &iter);
  dbus_message_iter_recurse(&iter, &variant);

  if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY) {
    dbus_message_iter_recurse(&variant, &dict);

    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter entry;
      dbus_message_iter_recurse(&dict, &entry);

      const char *uuid;
      dbus_message_iter_get_basic(&entry, &uuid);
      dbus_message_iter_next(&entry);

      DBusMessageIter val_variant;
      dbus_message_iter_recurse(&entry, &val_variant);

      if (dbus_message_iter_get_arg_type(&val_variant) == DBUS_TYPE_ARRAY) {
        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&val_variant, &array_iter);

        const uint8_t *bytes;
        int len;
        dbus_message_iter_get_fixed_array(&array_iter, &bytes, &len);

        if (len > 0) {
          js_value_t *buffer;
          void *data;
          err = js_create_arraybuffer(env, len, &data, &buffer);
          assert(err == 0);
          memcpy(data, bytes, len);

          err = js_set_named_property(env, static_cast<js_value_t *>(result), uuid, buffer);
          assert(err == 0);
        }
      }

      dbus_message_iter_next(&dict);
    }
  }

  dbus_message_unref(reply);
}

static void
bare_bluetooth_linux__call_method_async(
  js_env_t *env,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  const char *iface,
  std::string method,
  int timeout,
  js_function_t<void, js_object_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusPendingCall *pending = dbus_call_void_method(adapter->signal_conn, path.c_str(), iface, method.c_str(), timeout);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_device_connect(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, js_object_t> callback
) {
  bare_bluetooth_linux__call_method_async(env, adapter, path, BLUEZ_DEVICE_IFACE, "Connect", DBUS_CONNECT_TIMEOUT, callback);
}

static void
bare_bluetooth_linux_device_disconnect(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, js_object_t> callback
) {
  bare_bluetooth_linux__call_method_async(env, adapter, path, BLUEZ_DEVICE_IFACE, "Disconnect", DBUS_CONNECT_TIMEOUT, callback);
}

static void
bare_bluetooth_linux_device_pair(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, js_object_t> callback
) {
  bare_bluetooth_linux__call_method_async(env, adapter, path, BLUEZ_DEVICE_IFACE, "Pair", DBUS_CONNECT_TIMEOUT, callback);
}

static bool
bare_bluetooth_linux_service_is_primary(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_bool_prop(adapter->conn, path.c_str(), BLUEZ_GATT_SERVICE_IFACE, "Primary");
}

static void
bare_bluetooth_linux_char_read(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_function_t<void, js_object_t, js_arraybuffer_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_read_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), BLUEZ_GATT_CHAR_IFACE, "ReadValue");

  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_read_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_char_write(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_typedarray_t<uint8_t> value, std::optional<std::string> type, js_function_t<void, js_object_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  uint8_t *data;
  size_t len;
  err = js_get_typedarray_info(env, value, data, len);
  assert(err == 0);

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), BLUEZ_GATT_CHAR_IFACE, "WriteValue");

  DBusMessageIter iter, array, dict;
  dbus_message_iter_init_append(msg, &iter);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array);
  for (size_t i = 0; i < len; i++) {
    dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &data[i]);
  }
  dbus_message_iter_close_container(&iter, &array);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

  if (type) dbus_dict_append_string(&dict, "type", type->c_str());

  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_char_start_notify(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_function_t<void, js_object_t> callback
) {
  bare_bluetooth_linux__call_method_async(env, adapter, path, BLUEZ_GATT_CHAR_IFACE, "StartNotify", DBUS_TIMEOUT, callback);
}

static void
bare_bluetooth_linux_char_stop_notify(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_function_t<void, js_object_t> callback
) {
  bare_bluetooth_linux__call_method_async(env, adapter, path, BLUEZ_GATT_CHAR_IFACE, "StopNotify", DBUS_TIMEOUT, callback);
}

static std::vector<std::string>
bare_bluetooth_linux_char_get_flags(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_array_prop(adapter->conn, path.c_str(), BLUEZ_GATT_CHAR_IFACE, "Flags");
}

static std::optional<int32_t>
bare_bluetooth_linux_char_get_mtu(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_numeric_prop<uint16_t>(adapter->conn, path.c_str(), BLUEZ_GATT_CHAR_IFACE, "MTU");
}

static void
bare_bluetooth_linux_desc_read(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_function_t<void, js_object_t, js_arraybuffer_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_read_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), BLUEZ_GATT_DESC_IFACE, "ReadValue");

  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_read_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_desc_write(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path, js_typedarray_t<uint8_t> value, js_function_t<void, js_object_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  uint8_t *data;
  size_t len;
  err = js_get_typedarray_info(env, value, data, len);
  assert(err == 0);

  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path.c_str(), BLUEZ_GATT_DESC_IFACE, "WriteValue");

  DBusMessageIter iter, array, dict;
  dbus_message_iter_init_append(msg, &iter);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array);
  for (size_t i = 0; i < len; i++) {
    dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &data[i]);
  }
  dbus_message_iter_close_container(&iter, &array);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static std::vector<std::string>
bare_bluetooth_linux_desc_get_flags(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_array_prop(adapter->conn, path.c_str(), BLUEZ_GATT_DESC_IFACE, "Flags");
}

static void
bare_bluetooth_linux_advertisement_register(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string type,
  std::vector<std::string> service_uuids,
  std::optional<std::string> local_name,
  js_function_t<void, js_object_t> callback
) {
  adapter->adv.type = type;
  adapter->adv.service_uuids = service_uuids;
  adapter->adv.local_name = local_name;

  dbus_connection_register_object_path(
    adapter->signal_conn, BLUEZ_ADV_PATH, &bare_bluetooth_linux__adv_vtable, &*adapter
  );

  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg = dbus_message_new_method_call(
    BLUEZ_BUS, adapter->adapter_path.c_str(), BLUEZ_LE_ADV_MGR_IFACE, "RegisterAdvertisement"
  );

  const char *adv_path = BLUEZ_ADV_PATH;
  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &adv_path);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_advertisement_unregister(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  js_function_t<void, js_object_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg = dbus_message_new_method_call(
    BLUEZ_BUS, adapter->adapter_path.c_str(), BLUEZ_LE_ADV_MGR_IFACE, "UnregisterAdvertisement"
  );

  const char *adv_path = BLUEZ_ADV_PATH;
  DBusMessageIter iter;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &adv_path);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_connection_unregister_object_path(adapter->signal_conn, BLUEZ_ADV_PATH);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static int32_t
bare_bluetooth_linux_gatt_service_add(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string app_path, std::string uuid, bool primary
) {
  if (adapter->gatt_app.path.empty()) {
    adapter->gatt_app.path = app_path;
  } else if (adapter->gatt_app.path != app_path) {
    js_throw_error(env, nullptr, "All services must share the same application path");
    return -1;
  }

  bare_bluetooth_linux_local_service_t svc;
  svc.uuid = uuid;
  svc.primary = primary;
  int32_t idx = static_cast<int32_t>(adapter->gatt_app.services.size());
  svc.path = app_path + "/service" + std::to_string(idx);
  adapter->gatt_app.services.push_back(std::move(svc));
  adapter->gatt_app.service_map[adapter->gatt_app.services.back().path] = &adapter->gatt_app.services.back();
  return idx;
}

static std::optional<std::string>
bare_bluetooth_linux_gatt_characteristic_add(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, int32_t service_index, std::string uuid, std::vector<std::string> flags, js_typedarray_t<uint8_t> value
) {
  if (service_index < 0 || service_index >= static_cast<int32_t>(adapter->gatt_app.services.size())) {
    js_throw_error(env, nullptr, "service_index out of bounds");
    return std::nullopt;
  }

  auto &svc = adapter->gatt_app.services[service_index];

  bare_bluetooth_linux_local_characteristic_t ch;
  ch.uuid = uuid;
  ch.flags = flags;
  ch.service_path = svc.path;

  uint8_t *data;
  size_t len;
  int err = js_get_typedarray_info(env, value, data, len);
  assert(err == 0);
  ch.value.assign(data, data + len);

  int32_t idx = static_cast<int32_t>(svc.characteristics.size());
  ch.path = svc.path + "/char" + std::to_string(idx);
  svc.characteristics.push_back(std::move(ch));
  adapter->gatt_app.characteristic_map[svc.characteristics.back().path] = &svc.characteristics.back();
  return svc.characteristics.back().path;
}

static void
bare_bluetooth_linux_gatt_register(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, js_function_t<void, js_object_t> callback
) {
  dbus_connection_register_fallback(
    adapter->signal_conn, adapter->gatt_app.path.c_str(), &bare_bluetooth_linux__gatt_vtable, &*adapter
  );

  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg = dbus_message_new_method_call(
    BLUEZ_BUS, adapter->adapter_path.c_str(), BLUEZ_GATT_MGR_IFACE, "RegisterApplication"
  );

  const char *app_path = adapter->gatt_app.path.c_str();
  DBusMessageIter iter, dict;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &app_path);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&iter, &dict);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_pending_call_notify, call, NULL);
}

static void
bare_bluetooth_linux_gatt_unregister(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, js_function_t<void, js_object_t> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t();
  call->env = env;
  call->adapter = &*adapter;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  DBusMessage *msg = dbus_message_new_method_call(
    BLUEZ_BUS, adapter->adapter_path.c_str(), BLUEZ_GATT_MGR_IFACE, "UnregisterApplication"
  );

  const char *app_path = adapter->gatt_app.path.c_str();
  DBusMessageIter iter;
  dbus_message_iter_init_append(msg, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &app_path);

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(adapter->signal_conn, msg, &pending, DBUS_TIMEOUT);
  dbus_message_unref(msg);

  dbus_pending_call_set_notify(pending, bare_bluetooth_linux__on_gatt_unregister_notify, call, NULL);
}

static void
bare_bluetooth_linux_gatt_characteristic_set_value(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string char_path, js_typedarray_t<uint8_t> value
) {
  for (auto &svc : adapter->gatt_app.services) {
    for (auto &ch : svc.characteristics) {
      if (ch.path == char_path) {
        uint8_t *data;
        size_t len;
        int err = js_get_typedarray_info(env, value, data, len);
        assert(err == 0);

        ch.value.assign(data, data + len);
        return;
      }
    }
  }
}

// The L2CAP transport lives in libl2cap (I/O agnostic, kernel sockets). This
// section is only the JS glue: reference lifetimes, teardown, and the uv_poll
// wiring that drives the library's fd/events/process contract.

using bare_bluetooth_linux_l2cap__on_data_fn = js_function_t<void, js_receiver_t, js_uint8array_t>;
using bare_bluetooth_linux_l2cap__on_drain_fn = js_function_t<void, js_receiver_t>;
using bare_bluetooth_linux_l2cap__on_end_fn = js_function_t<void, js_receiver_t>;
using bare_bluetooth_linux_l2cap__on_error_fn = js_function_t<void, js_receiver_t, std::string>;
using bare_bluetooth_linux_l2cap__on_close_fn = js_function_t<void, js_receiver_t>;
using bare_bluetooth_linux_l2cap__on_open_fn = js_function_t<void, js_receiver_t>;
using bare_bluetooth_linux_l2cap__on_channel_fn = js_function_t<void, std::optional<js_object_t>, std::optional<js_arraybuffer_t>>;

// Lives inside an arraybuffer: constructed with placement new and ended with
// an explicit destructor call - the GC owns the storage, so never delete
struct bare_bluetooth_linux_l2cap_t {
  js_env_t *env = nullptr;
  l2cap_channel_t channel = {};
  uv_poll_t poll;
  bool opened = false;
  bool closing = false;
  bool closed = false;
  bool torn_down = false;
  js_deferred_teardown_t *teardown = nullptr;
  js_ref_t *ctx = nullptr;
  js_persistent_t<js_arraybuffer_t> self;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_channel_fn> on_channel;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_data_fn> on_data;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_drain_fn> on_drain;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_end_fn> on_end;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_error_fn> on_error;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_close_fn> on_close;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_open_fn> on_open;
};

template <typename Fn, typename... Args>
static void
bare_bluetooth_linux_l2cap__emit(bare_bluetooth_linux_l2cap_t *ch, js_persistent_t<Fn> &fn_ref, Args... args) {
  if (ch->torn_down || ch->ctx == nullptr) return;

  int err;
  js_env_t *env = ch->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ch->ctx, &receiver);
  assert(err == 0);

  Fn fn;
  err = js_get_reference_value(env, fn_ref, fn);
  assert(err == 0);

  err = js_call_function_with_checkpoint(env, fn, js_receiver_t(receiver), args...);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap__emit_data(bare_bluetooth_linux_l2cap_t *ch, const uint8_t *bytes, size_t len) {
  if (ch->torn_down || ch->ctx == nullptr) return;

  int err;
  js_env_t *env = ch->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, ch->ctx, &receiver);
  assert(err == 0);

  bare_bluetooth_linux_l2cap__on_data_fn fn;
  err = js_get_reference_value(env, ch->on_data, fn);
  assert(err == 0);

  uint8_t *data;
  js_uint8array_t typedarray;
  err = js_create_typedarray(env, len, data, typedarray);
  assert(err == 0);

  memcpy(data, bytes, len);

  err = js_call_function_with_checkpoint(env, fn, js_receiver_t(receiver), typedarray);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap__on_poll_close(uv_handle_t *handle) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(handle->data);

  l2cap_channel_close(&ch->channel);
  ch->closed = true;
  ch->closing = false;

  bare_bluetooth_linux_l2cap__emit(ch, ch->on_close);

  int err;
  if (ch->ctx) {
    err = js_delete_reference(ch->env, ch->ctx);
    assert(err == 0);
    ch->ctx = nullptr;
  }

  auto *teardown = ch->teardown;

  ch->~bare_bluetooth_linux_l2cap_t();

  err = js_finish_deferred_teardown_callback(teardown);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap__close(bare_bluetooth_linux_l2cap_t *ch) {
  if (ch->closing || ch->closed) return;
  ch->closing = true;

  // The actual close is deferred to the uv callback; stop the library's read
  // loop now so no more data is emitted in between
  l2cap_channel_read_stop(&ch->channel);

  uv_close(reinterpret_cast<uv_handle_t *>(&ch->poll), bare_bluetooth_linux_l2cap__on_poll_close);
}

static void
bare_bluetooth_linux_l2cap__on_teardown(js_deferred_teardown_t *, void *data) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(data);
  ch->torn_down = true;
  bare_bluetooth_linux_l2cap__close(ch);
}

static void
bare_bluetooth_linux_l2cap__on_poll(uv_poll_t *poll, int status, int events);

static void
bare_bluetooth_linux_l2cap__update_poll(bare_bluetooth_linux_l2cap_t *ch) {
  if (ch->closing || ch->closed) return;

  int err;
  int events = l2cap_channel_events(&ch->channel);

  if (events == 0) {
    err = uv_poll_stop(&ch->poll);
    assert(err == 0);
    return;
  }

  int mask = 0;
  if (events & L2CAP_READABLE) mask |= UV_READABLE;
  if (events & L2CAP_WRITABLE) mask |= UV_WRITABLE;

  err = uv_poll_start(&ch->poll, mask, bare_bluetooth_linux_l2cap__on_poll);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap__on_poll(uv_poll_t *poll, int status, int events) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(poll->data);

  if (ch->closing || ch->closed) return;

  // libuv reports POLLERR as UV_EBADF; hand everything to the library, which
  // reads the real failure off the socket
  int fired = 0;
  if (status < 0) {
    fired = L2CAP_READABLE | L2CAP_WRITABLE;
  } else {
    if (events & UV_READABLE) fired |= L2CAP_READABLE;
    if (events & UV_WRITABLE) fired |= L2CAP_WRITABLE;
  }

  int err = l2cap_channel_process(&ch->channel, fired);

  if (ch->closing || ch->closed) return; // a callback may have closed us

  if (err < 0) {
    bare_bluetooth_linux_l2cap__emit(ch, ch->on_error, std::string(strerror(-err)));
    bare_bluetooth_linux_l2cap__close(ch);
    return;
  }

  bare_bluetooth_linux_l2cap__update_poll(ch);
}

static void
bare_bluetooth_linux_l2cap__on_connect(l2cap_channel_t *channel, int status) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(channel->data);
  int err;

  js_env_t *env = ch->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  bare_bluetooth_linux_l2cap__on_channel_fn callback;
  err = js_get_reference_value(env, ch->on_channel, callback);
  assert(err == 0);

  // One-shot: the connect callback never fires again, release it so it does
  // not pin the caller's object graph for the channel's lifetime
  ch->on_channel.reset();

  if (status < 0) {
    js_object_t error;
    err = js_create_error(env, strerror(-status), error);
    assert(err == 0);

    // close before calling into JS: the callback may tear down the
    // environment, destroying the channel beneath us
    bare_bluetooth_linux_l2cap__close(ch);

    err = js_call_function_with_checkpoint(env, callback, std::optional<js_object_t>(error), std::optional<js_arraybuffer_t>());
    assert(err != js_pending_exception);
  } else {
    js_arraybuffer_t handle;
    err = js_get_reference_value(env, ch->self, handle);
    assert(err == 0);

    err = js_call_function_with_checkpoint(env, callback, std::optional<js_object_t>(), std::optional<js_arraybuffer_t>(handle));
    assert(err != js_pending_exception);
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap__on_read(l2cap_channel_t *channel, size_t len, const uint8_t *data) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(channel->data);

  if (len == 0) {
    bare_bluetooth_linux_l2cap__emit(ch, ch->on_end);
    bare_bluetooth_linux_l2cap__close(ch);
    return;
  }

  bare_bluetooth_linux_l2cap__emit_data(ch, data, len);
}

static void
bare_bluetooth_linux_l2cap__on_channel_drain(l2cap_channel_t *channel) {
  auto *ch = reinterpret_cast<bare_bluetooth_linux_l2cap_t *>(channel->data);
  bare_bluetooth_linux_l2cap__emit(ch, ch->on_drain);
}

static void
bare_bluetooth_linux_device_open_l2cap_channel(
  js_env_t *env,
  js_receiver_t,
  std::string local,
  std::string address,
  std::string address_type,
  uint32_t psm,
  std::optional<uint32_t> security,
  bare_bluetooth_linux_l2cap__on_channel_fn callback
) {
  int err;

  auto fail = [&](const char *message) {
    js_object_t error;
    err = js_create_error(env, message, error);
    assert(err == 0);

    err = js_call_function_with_checkpoint(env, callback, std::optional<js_object_t>(error), std::optional<js_arraybuffer_t>());
    assert(err != js_pending_exception);
  };

  l2cap_addr_t local_addr;
  if (l2cap_addr_init(local.c_str(), L2CAP_BDADDR_LE_PUBLIC, &local_addr) < 0) {
    return fail("Invalid adapter address");
  }

  uint8_t peer_type = address_type == "random" ? L2CAP_BDADDR_LE_RANDOM : L2CAP_BDADDR_LE_PUBLIC;

  l2cap_addr_t peer_addr;
  if (l2cap_addr_init(address.c_str(), peer_type, &peer_addr) < 0) {
    return fail("Invalid device address");
  }

  js_arraybuffer_t handle;
  bare_bluetooth_linux_l2cap_t *ch;
  err = js_create_arraybuffer(env, ch, handle);
  assert(err == 0);

  new (ch) bare_bluetooth_linux_l2cap_t();

  ch->env = env;

  l2cap_channel_init(&ch->channel, ch);

  if (security) {
    int res = l2cap_channel_set_security(&ch->channel, static_cast<uint8_t>(*security));
    if (res < 0) {
      const char *message = strerror(-res);
      ch->~bare_bluetooth_linux_l2cap_t();
      return fail(message);
    }
  }

  int res = l2cap_channel_connect(&ch->channel, &local_addr, &peer_addr, static_cast<uint16_t>(psm), bare_bluetooth_linux_l2cap__on_connect);
  if (res < 0) {
    const char *message = strerror(-res);
    ch->~bare_bluetooth_linux_l2cap_t();
    return fail(message);
  }

  err = js_create_reference(env, handle, ch->self);
  assert(err == 0);

  err = js_create_reference(env, callback, ch->on_channel);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_linux_l2cap__on_teardown, ch, &ch->teardown);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_poll_init(loop, &ch->poll, l2cap_channel_fd(&ch->channel));
  assert(err == 0);

  ch->poll.data = ch;

  bare_bluetooth_linux_l2cap__update_poll(ch);
}

static js_arraybuffer_t
bare_bluetooth_linux_l2cap_init(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch,
  js_object_t context,
  bare_bluetooth_linux_l2cap__on_data_fn on_data,
  bare_bluetooth_linux_l2cap__on_drain_fn on_drain,
  bare_bluetooth_linux_l2cap__on_end_fn on_end,
  bare_bluetooth_linux_l2cap__on_error_fn on_error,
  bare_bluetooth_linux_l2cap__on_close_fn on_close,
  bare_bluetooth_linux_l2cap__on_open_fn on_open
) {
  int err;

  err = js_create_reference(env, static_cast<js_value_t *>(context), 1, &ch->ctx);
  assert(err == 0);

  err = js_create_reference(env, on_data, ch->on_data);
  assert(err == 0);

  err = js_create_reference(env, on_drain, ch->on_drain);
  assert(err == 0);

  err = js_create_reference(env, on_end, ch->on_end);
  assert(err == 0);

  err = js_create_reference(env, on_error, ch->on_error);
  assert(err == 0);

  err = js_create_reference(env, on_close, ch->on_close);
  assert(err == 0);

  err = js_create_reference(env, on_open, ch->on_open);
  assert(err == 0);

  js_arraybuffer_t handle;
  err = js_get_reference_value(env, ch->self, handle);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_linux_l2cap_open(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch
) {
  if (ch->closing || ch->closed) return;

  int err = l2cap_channel_read_start(&ch->channel, bare_bluetooth_linux_l2cap__on_read);
  assert(err == 0);

  ch->opened = true;

  bare_bluetooth_linux_l2cap__update_poll(&*ch);
  bare_bluetooth_linux_l2cap__emit(&*ch, ch->on_open);
}

static int32_t
bare_bluetooth_linux_l2cap_write(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch,
  js_typedarray_t<uint8_t> buf
) {
  if (!ch->opened || ch->closing || ch->closed) return -1;

  uint8_t *data;
  size_t len;
  int err = js_get_typedarray_info(env, buf, data, len);
  assert(err == 0);

  int res = l2cap_channel_write(&ch->channel, data, len, bare_bluetooth_linux_l2cap__on_channel_drain);

  if (res < 0) {
    bare_bluetooth_linux_l2cap__emit(&*ch, ch->on_error, std::string(strerror(-res)));
    bare_bluetooth_linux_l2cap__close(&*ch);
    return res;
  }

  bare_bluetooth_linux_l2cap__update_poll(&*ch);

  return res;
}

static void
bare_bluetooth_linux_l2cap_end(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch
) {
  bare_bluetooth_linux_l2cap__close(&*ch);
}

static uint32_t
bare_bluetooth_linux_l2cap_psm(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch
) {
  return l2cap_channel_psm(&ch->channel);
}

static uint32_t
bare_bluetooth_linux_l2cap_mtu(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch
) {
  return l2cap_channel_snd_mtu(&ch->channel);
}

static std::string
bare_bluetooth_linux_l2cap_peer(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_t, 1> ch
) {
  char str[sizeof("00:00:00:00:00:00")];
  l2cap_addr_to_string(l2cap_channel_peer(&ch->channel), str);
  return str;
}

using bare_bluetooth_linux_l2cap__on_connection_fn = js_function_t<void, js_receiver_t, js_arraybuffer_t>;
using bare_bluetooth_linux_l2cap__on_publish_fn = js_function_t<void, std::optional<js_object_t>, std::optional<js_arraybuffer_t>>;
using bare_bluetooth_linux_l2cap__on_server_error_fn = js_function_t<void, js_receiver_t, uint32_t, std::string>;

// Same lifetime contract as the channel struct: placement new into an
// arraybuffer, explicit destructor call, storage owned by the GC
struct bare_bluetooth_linux_l2cap_server_t {
  js_env_t *env = nullptr;
  l2cap_server_t handle = {};
  uv_poll_t poll;
  bool closing = false;
  bool closed = false;
  bool torn_down = false;
  js_deferred_teardown_t *teardown = nullptr;
  js_ref_t *ctx = nullptr;
  js_persistent_t<js_arraybuffer_t> self;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_connection_fn> on_connection;
  js_persistent_t<bare_bluetooth_linux_l2cap__on_server_error_fn> on_error;
};

static void
bare_bluetooth_linux_l2cap_server__on_poll_close(uv_handle_t *handle) {
  auto *srv = reinterpret_cast<bare_bluetooth_linux_l2cap_server_t *>(handle->data);

  l2cap_server_close(&srv->handle);
  srv->closed = true;
  srv->closing = false;

  int err;
  if (srv->ctx) {
    err = js_delete_reference(srv->env, srv->ctx);
    assert(err == 0);
    srv->ctx = nullptr;
  }

  auto *teardown = srv->teardown;

  srv->~bare_bluetooth_linux_l2cap_server_t();

  err = js_finish_deferred_teardown_callback(teardown);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap_server__close(bare_bluetooth_linux_l2cap_server_t *srv) {
  if (srv->closing || srv->closed) return;
  srv->closing = true;
  uv_close(reinterpret_cast<uv_handle_t *>(&srv->poll), bare_bluetooth_linux_l2cap_server__on_poll_close);
}

static void
bare_bluetooth_linux_l2cap_server__on_teardown(js_deferred_teardown_t *, void *data) {
  auto *srv = reinterpret_cast<bare_bluetooth_linux_l2cap_server_t *>(data);
  srv->torn_down = true;
  bare_bluetooth_linux_l2cap_server__close(srv);
}

static void
bare_bluetooth_linux_l2cap_server__emit_error(bare_bluetooth_linux_l2cap_server_t *srv, uint32_t psm, std::string message) {
  if (srv->torn_down || srv->closing || srv->closed || srv->ctx == nullptr) return;

  int err;
  js_env_t *env = srv->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, srv->ctx, &receiver);
  assert(err == 0);

  bare_bluetooth_linux_l2cap__on_server_error_fn fn;
  err = js_get_reference_value(env, srv->on_error, fn);
  assert(err == 0);

  err = js_call_function_with_checkpoint(env, fn, js_receiver_t(receiver), psm, message);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap_server__on_connection(l2cap_server_t *server) {
  auto *srv = reinterpret_cast<bare_bluetooth_linux_l2cap_server_t *>(server->data);

  if (srv->torn_down || srv->closing || srv->closed || srv->ctx == nullptr) return;

  int err;
  js_env_t *env = srv->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_arraybuffer_t handle;
  bare_bluetooth_linux_l2cap_t *ch;
  err = js_create_arraybuffer(env, ch, handle);
  assert(err == 0);

  new (ch) bare_bluetooth_linux_l2cap_t();

  ch->env = env;

  l2cap_channel_init(&ch->channel, ch);

  int res = l2cap_server_accept(&srv->handle, &ch->channel);
  if (res < 0) {
    ch->~bare_bluetooth_linux_l2cap_t();

    err = js_close_handle_scope(env, scope);
    assert(err == 0);

    // The library stops accepting on failures that cannot clear on their
    // own; the descriptor stays readable, so polling on would spin
    if (!l2cap_server_failed(&srv->handle)) return;

    bare_bluetooth_linux_l2cap_server__emit_error(srv, l2cap_server_psm(&srv->handle), strerror(-res));
    bare_bluetooth_linux_l2cap_server__close(srv);
    return;
  }

  err = js_create_reference(env, handle, ch->self);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_linux_l2cap__on_teardown, ch, &ch->teardown);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_poll_init(loop, &ch->poll, l2cap_channel_fd(&ch->channel));
  assert(err == 0);

  ch->poll.data = ch;

  js_value_t *receiver;
  err = js_get_reference_value(env, srv->ctx, &receiver);
  assert(err == 0);

  bare_bluetooth_linux_l2cap__on_connection_fn fn;
  err = js_get_reference_value(env, srv->on_connection, fn);
  assert(err == 0);

  err = js_call_function_with_checkpoint(env, fn, js_receiver_t(receiver), handle);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap_server__on_poll(uv_poll_t *poll, int status, int events) {
  auto *srv = reinterpret_cast<bare_bluetooth_linux_l2cap_server_t *>(poll->data);

  if (srv->closing || srv->closed) return;

  int fired = 0;
  if (status < 0 || (events & UV_READABLE)) fired = L2CAP_READABLE;

  int err = l2cap_server_process(&srv->handle, fired);
  assert(err == 0);
}

static void
bare_bluetooth_linux_l2cap_publish(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  uint32_t psm,
  std::optional<uint32_t> security,
  js_object_t context,
  bare_bluetooth_linux_l2cap__on_connection_fn on_connection,
  bare_bluetooth_linux_l2cap__on_server_error_fn on_error,
  bare_bluetooth_linux_l2cap__on_publish_fn callback
) {
  int err;

  auto fail = [&](const char *message) {
    js_object_t error;
    err = js_create_error(env, message, error);
    assert(err == 0);

    err = js_call_function_with_checkpoint(env, callback, std::optional<js_object_t>(error), std::optional<js_arraybuffer_t>());
    assert(err != js_pending_exception);
  };

  auto local = dbus_get_string_prop(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "Address");
  if (!local) return fail("Unknown adapter address");

  l2cap_addr_t local_addr;
  if (l2cap_addr_init(local->c_str(), L2CAP_BDADDR_LE_PUBLIC, &local_addr) < 0) {
    return fail("Invalid adapter address");
  }

  js_arraybuffer_t handle;
  bare_bluetooth_linux_l2cap_server_t *srv;
  err = js_create_arraybuffer(env, srv, handle);
  assert(err == 0);

  new (srv) bare_bluetooth_linux_l2cap_server_t();

  srv->env = env;

  l2cap_server_init(&srv->handle, srv);

  if (security) {
    int res = l2cap_server_set_security(&srv->handle, static_cast<uint8_t>(*security));
    if (res < 0) {
      const char *message = strerror(-res);
      srv->~bare_bluetooth_linux_l2cap_server_t();
      return fail(message);
    }
  }

  int res = l2cap_server_listen(&srv->handle, &local_addr, static_cast<uint16_t>(psm), 4);
  if (res < 0) {
    const char *message = strerror(-res);
    srv->~bare_bluetooth_linux_l2cap_server_t();
    return fail(message);
  }

  err = l2cap_server_accept_start(&srv->handle, bare_bluetooth_linux_l2cap_server__on_connection);
  assert(err == 0);

  err = js_create_reference(env, handle, srv->self);
  assert(err == 0);

  err = js_create_reference(env, static_cast<js_value_t *>(context), 1, &srv->ctx);
  assert(err == 0);

  err = js_create_reference(env, on_connection, srv->on_connection);
  assert(err == 0);

  err = js_create_reference(env, on_error, srv->on_error);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_linux_l2cap_server__on_teardown, srv, &srv->teardown);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_poll_init(loop, &srv->poll, l2cap_server_fd(&srv->handle));
  assert(err == 0);

  srv->poll.data = srv;

  err = uv_poll_start(&srv->poll, UV_READABLE, bare_bluetooth_linux_l2cap_server__on_poll);
  assert(err == 0);

  err = js_call_function_with_checkpoint(env, callback, std::optional<js_object_t>(), std::optional<js_arraybuffer_t>(handle));
  assert(err != js_pending_exception);
}

static void
bare_bluetooth_linux_l2cap_unpublish(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_server_t, 1> srv
) {
  bare_bluetooth_linux_l2cap_server__close(&*srv);
}

static uint32_t
bare_bluetooth_linux_l2cap_server_psm(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_l2cap_server_t, 1> srv
) {
  return l2cap_server_psm(&srv->handle);
}

// TODO: bare-tcp exposes socketpair() but hardcodes SOCK_STREAM; once it
// accepts a type parameter (SOCK_SEQPACKET), this can shrink to an fd-adopting
// l2capAccept(fd) fed from JS
static std::vector<js_arraybuffer_t>
bare_bluetooth_linux_l2cap_pair(js_env_t *env, js_receiver_t) {
  int err;

  int fds[2];
  err = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  std::vector<js_arraybuffer_t> handles;
  handles.reserve(2);

  for (int fd : fds) {
    js_arraybuffer_t handle;
    bare_bluetooth_linux_l2cap_t *ch;
    err = js_create_arraybuffer(env, ch, handle);
    assert(err == 0);

    new (ch) bare_bluetooth_linux_l2cap_t();

    ch->env = env;

    l2cap_channel_init(&ch->channel, ch);

    err = l2cap_channel_accept(&ch->channel, fd);
    assert(err == 0);

    err = js_create_reference(env, handle, ch->self);
    assert(err == 0);

    err = js_add_deferred_teardown_callback(env, bare_bluetooth_linux_l2cap__on_teardown, ch, &ch->teardown);
    assert(err == 0);

    err = uv_poll_init(loop, &ch->poll, l2cap_channel_fd(&ch->channel));
    assert(err == 0);

    ch->poll.data = ch;

    handles.push_back(handle);
  }

  return handles;
}

static js_value_t *
bare_bluetooth_linux_exports(js_env_t *env, js_value_t *exports) {
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
  V("adapterSetDiscoveryFilter", bare_bluetooth_linux_adapter_set_discovery_filter)
  V("adapterStartDiscovery", bare_bluetooth_linux_adapter_start_discovery)
  V("adapterStopDiscovery", bare_bluetooth_linux_adapter_stop_discovery)

  V("deviceGetAddress", bare_bluetooth_linux_device_get_address)
  V("deviceGetAddressType", bare_bluetooth_linux_device_get_address_type)
  V("deviceGetName", bare_bluetooth_linux_device_get_name)
  V("deviceGetRSSI", bare_bluetooth_linux_device_get_rssi)
  V("deviceGetPaired", bare_bluetooth_linux_device_get_paired)
  V("deviceGetConnected", bare_bluetooth_linux_device_get_connected)
  V("deviceGetServicesResolved", bare_bluetooth_linux_device_get_services_resolved)
  V("deviceGetUUIDs", bare_bluetooth_linux_device_get_uuids)
  V("deviceGetManufacturerData", bare_bluetooth_linux_device_get_manufacturer_data)
  V("deviceGetServiceData", bare_bluetooth_linux_device_get_service_data)
  V("deviceConnect", bare_bluetooth_linux_device_connect)
  V("deviceOpenL2CAPChannel", bare_bluetooth_linux_device_open_l2cap_channel)
  V("deviceDisconnect", bare_bluetooth_linux_device_disconnect)
  V("devicePair", bare_bluetooth_linux_device_pair)

  V("serviceIsPrimary", bare_bluetooth_linux_service_is_primary)

  V("charRead", bare_bluetooth_linux_char_read)
  V("charWrite", bare_bluetooth_linux_char_write)
  V("charStartNotify", bare_bluetooth_linux_char_start_notify)
  V("charStopNotify", bare_bluetooth_linux_char_stop_notify)
  V("charGetFlags", bare_bluetooth_linux_char_get_flags)
  V("charGetMTU", bare_bluetooth_linux_char_get_mtu)

  V("descRead", bare_bluetooth_linux_desc_read)
  V("descWrite", bare_bluetooth_linux_desc_write)
  V("descGetFlags", bare_bluetooth_linux_desc_get_flags)

  V("advertisementRegister", bare_bluetooth_linux_advertisement_register)
  V("advertisementUnregister", bare_bluetooth_linux_advertisement_unregister)

  V("gattServiceAdd", bare_bluetooth_linux_gatt_service_add)
  V("gattCharacteristicAdd", bare_bluetooth_linux_gatt_characteristic_add)
  V("gattRegister", bare_bluetooth_linux_gatt_register)
  V("gattUnregister", bare_bluetooth_linux_gatt_unregister)
  V("gattCharacteristicSetValue", bare_bluetooth_linux_gatt_characteristic_set_value)

  V("l2capInit", bare_bluetooth_linux_l2cap_init)
  V("l2capOpen", bare_bluetooth_linux_l2cap_open)
  V("l2capWrite", bare_bluetooth_linux_l2cap_write)
  V("l2capEnd", bare_bluetooth_linux_l2cap_end)
  V("l2capPsm", bare_bluetooth_linux_l2cap_psm)
  V("l2capMtu", bare_bluetooth_linux_l2cap_mtu)
  V("l2capPeer", bare_bluetooth_linux_l2cap_peer)

  V("l2capPair", bare_bluetooth_linux_l2cap_pair)

  V("l2capPublish", bare_bluetooth_linux_l2cap_publish)
  V("l2capUnpublish", bare_bluetooth_linux_l2cap_unpublish)
  V("l2capServerPsm", bare_bluetooth_linux_l2cap_server_psm)

#undef V

#define V(name, value) \
  err = js_set_property(env, exports, name, value); \
  assert(err == 0);

  V("L2CAP_SECURITY_LOW", L2CAP_SECURITY_LOW)
  V("L2CAP_SECURITY_MEDIUM", L2CAP_SECURITY_MEDIUM)
  V("L2CAP_SECURITY_HIGH", L2CAP_SECURITY_HIGH)
  V("L2CAP_SECURITY_FIPS", L2CAP_SECURITY_FIPS)

#undef V

  return exports;
}

BARE_MODULE(bare_bluetooth_linux, bare_bluetooth_linux_exports)
