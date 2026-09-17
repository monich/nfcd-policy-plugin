/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <nfc_plugin_impl.h>
#include <nfc_manager.h>

#define GLOG_MODULE_NAME policy_plugin_log

#include <gutil_log.h>
#include <gio/gio.h>

GLOG_MODULE_DEFINE("policy-plugin");

typedef NfcPluginClass PolicyPluginClass;
typedef struct policy_plugin {
    NfcPlugin parent;
    NfcManager* manager;
    NfcBlockRequest* block;
    GFile* file;
    GFileMonitor* monitor;
    gulong changed_id;
} PolicyPlugin;

G_DEFINE_TYPE(PolicyPlugin, policy_plugin, NFC_TYPE_PLUGIN)
#define THIS_TYPE (policy_plugin_get_type())
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, PolicyPlugin)
#define PARENT_CLASS policy_plugin_parent_class

static const char POLICY_FILE[] = "/var/lib/policy/policy.conf";
static const char POLICY_GROUP[] = "policy";
static const char POLICY_KEY[] = "NfcEnabled";

static
void
policy_plugin_update(
    PolicyPlugin* self)
{
    GError *err = NULL;
    GKeyFile* f = g_key_file_new();
    gboolean blocked = FALSE;

    /* Parse the file */
    if (g_key_file_load_from_file(f, POLICY_FILE, G_KEY_FILE_NONE, &err)) {
        blocked = !g_key_file_get_boolean(f, POLICY_GROUP, POLICY_KEY, &err) &&
            !err;
    } else {
        GWARN("%s", GERRMSG(err));
    }
    g_clear_error(&err);
    g_key_file_free(f);

    /* Adjust the block if necessary */
    if (blocked) {
        if (!self->block) {
            GDEBUG("Blocking NFC");
            self->block = nfc_manager_block_request_new(self->manager);
        }
    } else if (self->block) {
        GDEBUG("Unblocking NFC");
        nfc_manager_block_request_free(self->block);
        self->block = NULL;
    }
}

static
void
policy_plugin_file_changed(
    GFileMonitor* monitor,
    GFile* file,
    GFile* other_file,
    GFileMonitorEvent event_type,
    gpointer user_data)
{
    policy_plugin_update(THIS(user_data));
}

/*==========================================================================*
 * NfcPlugin
 *==========================================================================*/

static
gboolean
policy_plugin_start(
    NfcPlugin* plugin,
    NfcManager* manager)
{
    PolicyPlugin* self = THIS(plugin);

    GVERBOSE("Starting");
    GASSERT(!self->manager);
    self->manager = nfc_manager_ref(manager);

    /* Start watching the file */
    self->monitor = g_file_monitor_file(self->file, G_FILE_MONITOR_NONE,
        NULL, NULL);
    self->changed_id = g_signal_connect(self->monitor, "changed",
        G_CALLBACK(policy_plugin_file_changed), self);

    policy_plugin_update(self);
    return TRUE;
}

static
void
policy_plugin_stop(
    NfcPlugin* plugin)
{
    PolicyPlugin* self = THIS(plugin);

    GVERBOSE("Stopping");

    if (self->block) {
        nfc_manager_block_request_free(self->block);
        self->block = NULL;
    }

    /* Stop watching the file */
    g_signal_handler_disconnect(self->monitor, self->changed_id);
    g_object_unref(self->monitor);
    self->changed_id = 0;
    self->monitor = NULL;

    nfc_manager_unref(self->manager);
    self->manager = NULL;
}

static
void
policy_plugin_init(
    PolicyPlugin* self)
{
    self->file = g_file_new_for_path(POLICY_FILE);
}

static
void
policy_plugin_finalize(
    GObject* object)
{
    PolicyPlugin* self = THIS(object);

    GASSERT(!self->manager);
    g_object_unref(self->file);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
policy_plugin_class_init(
    PolicyPluginClass* klass)
{
    klass->start = policy_plugin_start;
    klass->stop = policy_plugin_stop;
    G_OBJECT_CLASS(klass)->finalize = policy_plugin_finalize;
}

static
NfcPlugin*
policy_plugin_create(
    void)
{
    GDEBUG("Plugin loaded");
    return g_object_new(THIS_TYPE, NULL);
}

NFC_PLUGIN_DEFINE(policy, "policy plugin", policy_plugin_create)

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
