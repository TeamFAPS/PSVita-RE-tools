#ifndef NET_LOGGING_MGR_INTERNAL_H
#define NET_LOGGING_MGR_INTERNAL_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

typedef struct {
	uint32_t magic;
	uint32_t IPv4;
	uint32_t flags;
	uint16_t port
} NetLoggingMgrConfig_t;

#define NLM_CONFIG_FLAGS_BIT_QAF_DEBUG_PRINTF			(1 << 0)
#define DEFAULT_PORT 8080
#endif