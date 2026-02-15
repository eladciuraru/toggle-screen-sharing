#include "screen_sharing.h"

__attribute__((constructor))
static void constructor(void)
{
    ScreenSharing_Toggle();
}

void ScreenSharing_Toggle(void)
{
    ss_context_t context = ScreenSharing_ContextCreate();
    context.log("toggling screen sharing to %s", (context.toggle) ? "on" : "off");

    const char *service_names[] = {
        "com.apple.tccd.system",
        "com.apple.tccd",
    };

    bool screen_capture_success = false;
    bool post_event_success     = false;

    for (size_t i = 0; i < ARRAY_COUNT(service_names); i++) {
        context.log("connecting to %s", service_names[i]);
        if (!ScreenSharing_ServiceConnect(&context, service_names[i])) {
            continue;
        }

        if (!post_event_success) {
            post_event_success =
                ScreenSharing_ServiceSendRequest(context, "kTCCServicePostEvent");
        }

        if (!screen_capture_success) {
            screen_capture_success =
                ScreenSharing_ServiceSendRequest(context, "kTCCServiceScreenCapture");
        }

        break;
    }

    if (screen_capture_success && post_event_success) {
        context.log("successfuly toggled screen sharing");
    } else {
        context.log("failed to toggle screen sharing");
    }

    ScreenSharing_ContextDestroy(&context);
}

static void ScreenSharing__LogOS(const char *format, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, format);

    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    os_log(OS_LOG_DEFAULT, "%{public}s", buffer);
}

static void ScreenSharing__LogStderr(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

static void ScreenSharing__LogQuiet(const char *format, ...)
{
    (void)format;
}

static void ScreenSharing__LogXpcObject(ss_log_fn log_fn, const char *message, xpc_object_t object)
{
    log_fn("%s", message);

    if (!object) {
        log_fn("<xpc null object>");
        return;
    }

    char *description = xpc_copy_description(object);
    if (description) {
        log_fn("%s", description);
        free(description);
    }
}

ss_context_t ScreenSharing_ContextCreate(void)
{
    ss_context_t context = {0};

    char *screen_sharing_log = getenv("SCREEN_SHARING_LOG");
    if (!screen_sharing_log || !strcasecmp(screen_sharing_log, "stderr")) {
        context.log = ScreenSharing__LogStderr;
    } else if (!strcasecmp(screen_sharing_log, "os")) {
        context.log = ScreenSharing__LogOS;
    } else if (!strcasecmp(screen_sharing_log, "quiet")) {
        context.log = ScreenSharing__LogQuiet;
    } else {
        context.log = ScreenSharing__LogStderr;
        ScreenSharing__LogStderr("unknown \"%s\" log value, defaulting to stderr",
                                 screen_sharing_log);
    }

    char *screen_sharing_toggle = getenv("SCREEN_SHARING_TOGGLE");
    if (!screen_sharing_toggle || !strcasecmp(screen_sharing_toggle, "on")) {
        context.toggle = true;
    } else if (!strcasecmp(screen_sharing_toggle, "off")) {
        context.toggle = false;
    } else {
        context.toggle = true;
        context.log("unknown toggle \"%s\", defaulting to on", screen_sharing_toggle);
    }

    return context;
}

void ScreenSharing_ContextDestroy(ss_context_t *context)
{
    if (context->connection) {
        xpc_connection_cancel(context->connection);
        xpc_release(context->connection);
        context->connection = NULL;
    }

    if (context->dispatch_queue) {
        dispatch_release(context->dispatch_queue);
        context->dispatch_queue = NULL;
    }
}

bool ScreenSharing_ServiceConnect(ss_context_t *context, const char *service_name)
{
    bool result = false;

    dispatch_queue_t dispatch_queue =
        dispatch_queue_create("com.screen-sharing.toggle.queue", DISPATCH_QUEUE_SERIAL);
    if (!dispatch_queue) {
        context->log("failed to create dispatch queue");

    } else {
        xpc_connection_t connection =
            xpc_connection_create_mach_service(service_name, dispatch_queue, 0);
        if (!connection) {
            context->log("failed to create connection");
            dispatch_release(dispatch_queue);

        } else {
            xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
                (void)event;
            });
            xpc_connection_resume(connection);

            ScreenSharing_ContextDestroy(context);
            context->connection = connection;
            context->dispatch_queue = dispatch_queue;
            result = true;
        }
    }

    return result;
}

bool ScreenSharing_ServiceSendRequest(ss_context_t context, const char *service)
{
    xpc_object_t request = xpc_dictionary_create_empty();
    bool         result  = false;

    if (request) {
        xpc_dictionary_set_string(request, "function", "TCCAccessSetInternal");
        xpc_dictionary_set_string(request, "client", "com.apple.screensharing.agent");
        xpc_dictionary_set_string(request, "client_type", "bundle");
        xpc_dictionary_set_string(request, "service", service);
        xpc_dictionary_set_bool(request, "granted", context.toggle);
        ScreenSharing__LogXpcObject(context.log, "sending request: ", request);

        xpc_object_t reply =
            xpc_connection_send_message_with_reply_sync(context.connection, request);
        if (!reply || xpc_get_type(reply) == XPC_TYPE_ERROR) {
            ScreenSharing__LogXpcObject(context.log, "failed to send request with: ", reply);
        } else {
            result = xpc_dictionary_get_bool(reply, "result");
            xpc_release(reply);
        }
        xpc_release(request);
    }

    return result;
}
