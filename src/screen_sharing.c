#include "screen_sharing.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>

#include <os/log.h>
#include <Security/Security.h>
#include <ServiceManagement/ServiceManagement.h>
#include <CoreFoundation/CoreFoundation.h>


#ifdef SCREEN_SHARING_EXECUTABLE

int main(int argc, const char **argv)
{
    ss_context_t context = ScreenSharing_ContextCreateFromArgs(argc, argv);
    ScreenSharing_Toggle(&context);
    ScreenSharing_ContextDestroy(&context);
}

#else

__attribute__((constructor))
static void constructor(void)
{
    ss_context_t context = ScreenSharing_ContextCreateFromEnv();
    ScreenSharing_Toggle(&context);
    ScreenSharing_ContextDestroy(&context);
}

#endif  // SCREEN_SHARING_EXECUTABLE

void ScreenSharing_Toggle(ss_context_t *context)
{
    context->log("toggling screen sharing to %s", (context->toggle) ? "on" : "off");

    const char *service_names[] = {
        "com.apple.tccd.system",
        "com.apple.tccd",
    };

    bool screen_capture_success = false;
    bool post_event_success     = false;
    bool remote_login_success   = false;
    bool requests_success       = false;

    for (size_t i = 0; i < ARRAY_COUNT(service_names) && !requests_success; i++) {
        context->log("connecting to %s", service_names[i]);
        if (!ScreenSharing_ServiceConnect(context, service_names[i])) {
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

        if (!remote_login_success) {
            remote_login_success =
                ScreenSharing_RemoteLoginServiceSendRequest(context);
        }

        requests_success = post_event_success && screen_capture_success & remote_login_success;
    }

    bool remote_login_serivce_success = ScreenSharing_RemoteLoginServiceSet(context);

    if (requests_success & remote_login_serivce_success) {
        context->log("successfuly toggled screen sharing & remote login");
    } else {
        context->log("failed to toggle screen sharing");
        context->log("post_event_success = %d", post_event_success);
        context->log("screen_capture_success = %d", screen_capture_success);
        context->log("remote_login_success = %d", remote_login_success);
        context->log("remote_login_serivce_success = %d", remote_login_serivce_success);
    }
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

static void ScreenSharing__LogStdout(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");

    va_end(args);
}

static void ScreenSharing__LogQuiet(const char *format, ...)
{
    (void)format;
}

static ss_log_fn ScreenSharing__LogFnFromString(const char *name, ss_log_fn default_fn)
{
    if (!name) {
        return default_fn;
    } else if (!strcasecmp(name, "stderr")) {
        return ScreenSharing__LogStderr;
    } else if (!strcasecmp(name, "stdout")) {
        return ScreenSharing__LogStdout;
    } else if (!strcasecmp(name, "os")) {
        return ScreenSharing__LogOS;
    } else if (!strcasecmp(name, "quiet")) {
        return ScreenSharing__LogQuiet;
    }

    return default_fn;
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

ss_context_t ScreenSharing_ContextCreateFromEnv(void)
{
    ss_context_t context = {0};

    char *screen_sharing_log = getenv("SCREEN_SHARING_LOG");
    context.log =
        ScreenSharing__LogFnFromString(screen_sharing_log, ScreenSharing__LogStderr);

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

ss_context_t ScreenSharing_ContextCreateFromArgs(int argc, const char **argv)
{
    ss_context_t context = {
        .log = ScreenSharing__LogStdout,
        .toggle = true,
    };

    if (argc < 1) {
        return context;
    }

    const char *program_name = argv[0];
    bool        show_usage   = false;

    for (int i = 1; i < argc && !show_usage; i++) {
        if (!strcmp(argv[i], "--log")) {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                ScreenSharing__LogStderr("missing value for log");
                show_usage = true;
            } else {
                context.log =
                    ScreenSharing__LogFnFromString(argv[++i], context.log);
            }
        } else if (!strcmp(argv[i], "--toggle")) {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                ScreenSharing__LogStderr("missing value for toggle");
                show_usage = true;
            } else if (!strcasecmp(argv[++i], "on")) {
                context.toggle = true;
            } else if (!strcasecmp(argv[i], "off")) {
                context.toggle = false;
            } else {
                ScreenSharing__LogStderr("unknown value for toggle");
            }
        } else if (!strcmp(argv[i], "--help") ||
                   !strcmp(argv[i], "-h")) {
            show_usage = true;
        } else {
            ScreenSharing__LogStderr("unknown argument '%s'", argv[i]);
            show_usage = true;
        }
    }

    if (show_usage) {
        ScreenSharing__LogStderr(
            "Usage: %s [OPTIONS]\n\n"
            "Toggle on/off screen sharing.\n\n"
            "Options:\n"
            "  --log <mode>        Set logging output.\n"
            "                      Supported modes stdout/stderr/os/quiet.\n"
            "                      Default: stdout\n"
            "  --toggle <on|off>   Enable or disable screen sharing toggle.\n"
            "                      Default: on\n"
            "  -h, --help          Show this help message and exit.\n\n",
            program_name
        );
        exit(1);
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

bool ScreenSharing_ServiceSendRequest(ss_context_t *context, const char *service)
{
    xpc_object_t request = xpc_dictionary_create_empty();
    bool         result  = false;

    if (request) {
        xpc_dictionary_set_string(request, "function", "TCCAccessSetInternal");
        xpc_dictionary_set_string(request, "client", "com.apple.screensharing.agent");
        xpc_dictionary_set_string(request, "client_type", "bundle");
        xpc_dictionary_set_string(request, "service", service);
        xpc_dictionary_set_bool(request, "granted", context->toggle);
        ScreenSharing__LogXpcObject(context->log, "sending request: ", request);

        xpc_object_t reply =
            xpc_connection_send_message_with_reply_sync(context->connection, request);
        if (!reply) {
            context->log("failed to send request with missing reply");
        } else {
            if (xpc_get_type(reply) == XPC_TYPE_ERROR) {
                ScreenSharing__LogXpcObject(context->log, "failed to send request with: ", reply);
            } else {
                ScreenSharing__LogXpcObject(context->log, "reply: ", reply);
                result = xpc_dictionary_get_bool(reply, "result");
            }
            xpc_release(reply);
        }
        xpc_release(request);
    }

    return result;
}

bool ScreenSharing_RemoteLoginServiceSendRequest(ss_context_t *context)
{
    xpc_object_t request = xpc_dictionary_create_empty();
    bool         result  = false;

    if (request) {
        xpc_dictionary_set_string(request, "function", "TCCAccessSetInternal");
        xpc_dictionary_set_string(request, "client", "/usr/libexec/sshd-keygen-wrapper");
        xpc_dictionary_set_string(request, "client_type", "path");
        xpc_dictionary_set_string(request, "service", "kTCCServiceSystemPolicyAllFiles");
        xpc_dictionary_set_bool(request, "granted", context->toggle);
        ScreenSharing__LogXpcObject(context->log, "sending request: ", request);

        xpc_object_t reply =
            xpc_connection_send_message_with_reply_sync(context->connection, request);
        if (!reply) {
            context->log("failed to send request with missing reply");
        } else {
            if (xpc_get_type(reply) == XPC_TYPE_ERROR) {
                ScreenSharing__LogXpcObject(context->log, "failed to send request with: ", reply);
            } else {
                ScreenSharing__LogXpcObject(context->log, "reply: ", reply);
                result = xpc_dictionary_get_bool(reply, "result");
            }
            xpc_release(reply);
        }
        xpc_release(request);
    }

    return result;
}

extern Boolean SMJobSetEnabled(CFStringRef domain, AuthorizationRef auth,
                               CFStringRef service_name, Boolean, int unknown,
                               CFErrorRef *error);

bool ScreenSharing_RemoteLoginServiceSet(ss_context_t *context)
{
    static AuthorizationItem auth_item = {
        .name = "com.apple.ServiceManagement.daemons.modify",
    };
    static const AuthorizationRights auth_rigths = {
        .count = 1,
        .items = &auth_item,
    };

    bool result = false;

    AuthorizationRef auth = NULL;
    OSStatus status =
        AuthorizationCreate(&auth_rigths, NULL,
                            kAuthorizationFlagPartialRights, &auth);
    if (status) {
        CFStringRef error_desc = SecCopyErrorMessageString(status, NULL);
        if (error_desc) {
            char error_buffer[1024] = {0};
            CFStringGetCString(error_desc, error_buffer, sizeof(error_buffer),
                               kCFStringEncodingUTF8);

            context->log("failed to create authorization with: %d - %s",
                         status,
                         error_buffer);
            CFRelease(error_desc);
        } else {
            context->log("failed to create authorization with: %d", status);
        }
        return false;
    }

    CFErrorRef error = NULL;
    if (!SMJobSetEnabled(kSMDomainSystemLaunchd, auth,
                         CFSTR("com.openssh.sshd"), context->toggle,
                         1, &error)) {
        CFStringRef error_desc = CFErrorCopyDescription(error);
        if (error_desc) {
            char error_buffer[1024] = {0};
            CFStringGetCString(error_desc, error_buffer, sizeof(error_buffer),
                               kCFStringEncodingUTF8);

            context->log("failed to set service with: %s", error_buffer);
            CFRelease(error_desc);
        } else {
            context->log("failed to set service with unknown error");
        }

        CFRelease(error);
    } else {
        result = true;
    }

    AuthorizationFree(auth, kAuthorizationFlagDefaults);

    return result;
}
