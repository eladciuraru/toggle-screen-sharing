#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include <stdbool.h>

#include <os/log.h>
#include <xpc/xpc.h>
#include <dispatch/dispatch.h>
#include <Block.h>

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))


typedef void (*ss_log_fn)(const char *format, ...);

typedef struct {
    ss_log_fn log __printflike(1, 2);
    bool      toggle;

    dispatch_queue_t dispatch_queue;
    xpc_connection_t connection;
} ss_context_t;

void ScreenSharing_Toggle(void);

ss_context_t ScreenSharing_ContextCreate(void);
void         ScreenSharing_ContextDestroy(ss_context_t *context);

bool ScreenSharing_ServiceConnect(ss_context_t *context, const char *service_name);
bool ScreenSharing_ServiceSendRequest(ss_context_t context, const char *service_name);
