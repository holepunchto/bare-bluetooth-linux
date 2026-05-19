#include <assert.h>
#include <atomic>
#include <bare.h>
#include <dbus/dbus.h>
#include <js.h>
#include <jstl.h>
#include <optional>
#include <string>
#include <uv.h>

#define BLUEZ_BUS            "org.bluez"
#define DBUS_PROP_IFACE      "org.freedesktop.DBus.Properties"
#define DBUS_OM_IFACE        "org.freedesktop.DBus.ObjectManager"
#define BLUEZ_ADAPTER_IFACE  "org.bluez.Adapter1"
#define BLUEZ_DEVICE_IFACE   "org.bluez.Device1"
#define DBUS_TIMEOUT         2000
#define DBUS_CONNECT_TIMEOUT 30000
#define DBUS_POLL_INTERVAL   200

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

static std::optional<int32_t>
dbus_get_int16_prop(DBusConnection *conn, const char *path, const char *iface, const char *prop) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, DBUS_PROP_IFACE, "Get");
  dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT, &err);
  dbus_message_unref(msg);

  std::optional<int32_t> result;
  if (reply && !dbus_error_is_set(&err)) {
    DBusMessageIter iter, variant;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_recurse(&iter, &variant);
    if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_INT16) {
      int16_t val;
      dbus_message_iter_get_basic(&variant, &val);
      result = val;
    }
    dbus_message_unref(reply);
  }
  if (dbus_error_is_set(&err))
    dbus_error_free(&err);

  return result;
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

static std::optional<std::string>
dbus_call_void_method(DBusConnection *conn, const char *path, const char *iface, const char *method, int timeout = DBUS_TIMEOUT) {
  DBusMessage *msg =
    dbus_message_new_method_call(BLUEZ_BUS, path, iface, method);

  DBusError err;
  dbus_error_init(&err);
  DBusMessage *reply =
    dbus_connection_send_with_reply_and_block(conn, msg, timeout, &err);
  dbus_message_unref(msg);

  if (dbus_error_is_set(&err)) {
    std::string error = err.message;
    dbus_error_free(&err);
    return error;
  }

  if (reply)
    dbus_message_unref(reply);

  return std::nullopt;
}

struct bare_bluetooth_linux_adapter_t;

struct bare_bluetooth_linux_device_added_event_t {
  std::string path;
  std::string address;
};

struct bare_bluetooth_linux_device_removed_event_t {
  std::string path;
};

struct bare_bluetooth_linux_tsfn_ctx_t {
  bare_bluetooth_linux_adapter_t *adapter;
};

struct bare_bluetooth_linux_async_call_t {
  uv_work_t work;
  js_env_t *env;
  js_persistent_t<js_function_t<void, std::optional<std::string>>> cb;
  js_deferred_teardown_t *teardown;
  std::string path;
  std::string iface;
  std::string method;
  int timeout;
  std::optional<std::string> error;
  bool exiting = false;

  bare_bluetooth_linux_async_call_t(js_env_t *env) : env(env) {
    int err;
    err = js_add_teardown_callback<on_teardown, bare_bluetooth_linux_async_call_t>(env, this, teardown);
    assert(err == 0);
  }

  ~bare_bluetooth_linux_async_call_t() {
    int err;
    err = js_finish_teardown_callback(teardown);
    assert(err == 0);
  }

private:
  static void
  on_teardown(js_deferred_teardown_t *teardown, bare_bluetooth_linux_async_call_t *req) {
    req->exiting = true;
  }
};

using bare_bluetooth_linux__on_device_added_fn =
  js_function_t<void, js_receiver_t, std::string, std::string>;

using bare_bluetooth_linux__on_device_removed_fn =
  js_function_t<void, js_receiver_t, std::string>;

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

  js_call_function(env, function, js_receiver_t(receiver), event->path, event->address);

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
bare_bluetooth_linux__async_call_execute(uv_work_t *uv_req) {
  auto *call = reinterpret_cast<bare_bluetooth_linux_async_call_t *>(uv_req);

  DBusError err;
  dbus_error_init(&err);
  DBusConnection *conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

  if (dbus_error_is_set(&err)) {
    call->error = std::string(err.message);
    dbus_error_free(&err);
    return;
  }

  dbus_connection_set_exit_on_disconnect(conn, FALSE);

  call->error = dbus_call_void_method(conn, call->path.c_str(), call->iface.c_str(), call->method.c_str(), call->timeout);

  dbus_connection_close(conn);
  dbus_connection_unref(conn);
}

static void
bare_bluetooth_linux__async_call_complete(uv_work_t *uv_req, int status) {
  auto *call = reinterpret_cast<bare_bluetooth_linux_async_call_t *>(uv_req);

  if (call->exiting) return delete call;

  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(call->env, &scope);
  assert(err == 0);

  js_function_t<void, std::optional<std::string>> callback;
  err = js_get_reference_value(call->env, call->cb, callback);
  assert(err == 0);

  err = js_call_function_with_checkpoint(call->env, callback, call->error);
  assert(err != js_pending_exception);

  err = js_close_handle_scope(call->env, scope);
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
bare_bluetooth_linux__on_interfaces_added(bare_bluetooth_linux_adapter_t *adapter, DBusMessage *msg) {
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
  std::string address;

  while (dbus_message_iter_get_arg_type(&ifaces_iter) == DBUS_TYPE_DICT_ENTRY) {
    DBusMessageIter entry;
    dbus_message_iter_recurse(&ifaces_iter, &entry);

    const char *iface_name;
    dbus_message_iter_get_basic(&entry, &iface_name);

    if (strcmp(iface_name, BLUEZ_DEVICE_IFACE) == 0) {
      is_device = true;
      dbus_message_iter_next(&entry);

      DBusMessageIter props_iter;
      dbus_message_iter_recurse(&entry, &props_iter);

      while (dbus_message_iter_get_arg_type(&props_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter prop_entry;
        dbus_message_iter_recurse(&props_iter, &prop_entry);

        const char *prop_name;
        dbus_message_iter_get_basic(&prop_entry, &prop_name);

        if (strcmp(prop_name, "Address") == 0) {
          dbus_message_iter_next(&prop_entry);
          DBusMessageIter variant;
          dbus_message_iter_recurse(&prop_entry, &variant);
          if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING) {
            const char *addr;
            dbus_message_iter_get_basic(&variant, &addr);
            address = addr;
          }
        }

        dbus_message_iter_next(&props_iter);
      }
    }

    dbus_message_iter_next(&ifaces_iter);
  }

  if (is_device && !address.empty()) {
    auto *event = new bare_bluetooth_linux_device_added_event_t;
    event->path = obj_path;
    event->address = address;
    js_call_threadsafe_function(adapter->tsfn_device_added, event, js_threadsafe_function_nonblocking);
  }
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
  while (dbus_message_iter_get_arg_type(&ifaces_iter) == DBUS_TYPE_STRING) {
    const char *iface_name;
    dbus_message_iter_get_basic(&ifaces_iter, &iface_name);
    if (strcmp(iface_name, BLUEZ_DEVICE_IFACE) == 0) {
      is_device = true;
      break;
    }
    dbus_message_iter_next(&ifaces_iter);
  }

  if (is_device) {
    auto *event = new bare_bluetooth_linux_device_removed_event_t;
    event->path = obj_path;
    js_call_threadsafe_function(adapter->tsfn_device_removed, event, js_threadsafe_function_nonblocking);
  }
}

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
  bare_bluetooth_linux__on_device_removed_fn on_device_removed
) {
  dbus_threads_init_default();

  js_arraybuffer_t handle;
  bare_bluetooth_linux_adapter_t *adapter;
  int err = js_create_arraybuffer(env, adapter, handle);
  assert(err == 0);

  new (adapter) bare_bluetooth_linux_adapter_t();

  adapter->adapter_path = path;
  adapter->running.store(true);
  adapter->tsfn_count.store(2);

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
bare_bluetooth_linux_adapter_start_discovery(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  auto error = dbus_call_void_method(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "StartDiscovery");
  if (error) {
    int err = js_throw_error(env, nullptr, error->c_str());
    assert(err == 0);
  }
}

static void
bare_bluetooth_linux_adapter_stop_discovery(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter
) {
  auto error = dbus_call_void_method(adapter->conn, adapter->adapter_path.c_str(), BLUEZ_ADAPTER_IFACE, "StopDiscovery");
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
bare_bluetooth_linux_device_get_name(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_string_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "Name");
}

static std::optional<int32_t>
bare_bluetooth_linux_device_get_rssi(
  js_env_t *env, js_receiver_t, js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter, std::string path
) {
  return dbus_get_int16_prop(adapter->conn, path.c_str(), BLUEZ_DEVICE_IFACE, "RSSI");
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

static void
bare_bluetooth_linux_device_connect(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, std::optional<std::string>> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t(env);
  call->path = path;
  call->iface = BLUEZ_DEVICE_IFACE;
  call->method = "Connect";
  call->timeout = DBUS_CONNECT_TIMEOUT;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_queue_work(loop, &call->work, bare_bluetooth_linux__async_call_execute, bare_bluetooth_linux__async_call_complete);
  assert(err == 0);
}

static void
bare_bluetooth_linux_device_disconnect(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, std::optional<std::string>> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t(env);
  call->path = path;
  call->iface = BLUEZ_DEVICE_IFACE;
  call->method = "Disconnect";
  call->timeout = DBUS_CONNECT_TIMEOUT;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_queue_work(loop, &call->work, bare_bluetooth_linux__async_call_execute, bare_bluetooth_linux__async_call_complete);
  assert(err == 0);
}

static void
bare_bluetooth_linux_device_pair(
  js_env_t *env,
  js_receiver_t,
  js_arraybuffer_span_of_t<bare_bluetooth_linux_adapter_t, 1> adapter,
  std::string path,
  js_function_t<void, std::optional<std::string>> callback
) {
  auto *call = new bare_bluetooth_linux_async_call_t(env);
  call->path = path;
  call->iface = BLUEZ_DEVICE_IFACE;
  call->method = "Pair";
  call->timeout = DBUS_CONNECT_TIMEOUT;

  int err;
  err = js_create_reference(env, callback, call->cb);
  assert(err == 0);

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  err = uv_queue_work(loop, &call->work, bare_bluetooth_linux__async_call_execute, bare_bluetooth_linux__async_call_complete);
  assert(err == 0);
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
  V("adapterStartDiscovery", bare_bluetooth_linux_adapter_start_discovery)
  V("adapterStopDiscovery", bare_bluetooth_linux_adapter_stop_discovery)

  V("deviceGetAddress", bare_bluetooth_linux_device_get_address)
  V("deviceGetName", bare_bluetooth_linux_device_get_name)
  V("deviceGetRSSI", bare_bluetooth_linux_device_get_rssi)
  V("deviceGetPaired", bare_bluetooth_linux_device_get_paired)
  V("deviceGetConnected", bare_bluetooth_linux_device_get_connected)
  V("deviceConnect", bare_bluetooth_linux_device_connect)
  V("deviceDisconnect", bare_bluetooth_linux_device_disconnect)
  V("devicePair", bare_bluetooth_linux_device_pair)

#undef V

  return exports;
}

BARE_MODULE(bare_bluetooth_linux, bare_bluetooth_linux_exports)
