#pragma once

#include <stdbool.h>

#include <xpc/xpc.h>
#include <dispatch/dispatch.h>

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

typedef void (*ss_log_fn)(const char *format, ...);

typedef struct {
    ss_log_fn log __printflike(1, 2);
    bool      toggle;

    dispatch_queue_t dispatch_queue;
    xpc_connection_t connection;
} ss_context_t;

void ScreenSharing_Toggle(ss_context_t *context);

ss_context_t ScreenSharing_ContextCreateFromEnv(void);
ss_context_t ScreenSharing_ContextCreateFromArgs(int argc, const char **argv);
void         ScreenSharing_ContextDestroy(ss_context_t *context);

bool ScreenSharing_ServiceConnect(ss_context_t *context, const char *service_name);
bool ScreenSharing_ServiceSendRequest(ss_context_t *context, const char *service_name);

bool ScreenSharing_RemoteLoginServiceSendRequest(ss_context_t *context);
bool ScreenSharing_RemoteLoginServiceSet(ss_context_t *context);
