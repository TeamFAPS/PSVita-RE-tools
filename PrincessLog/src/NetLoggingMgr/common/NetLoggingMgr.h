#ifndef NET_LOGGING_MGR_H
#define NET_LOGGING_MGR_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include "NetLoggingMgrInternal.h"

int NetLoggingMgrReadConfig(NetLoggingMgrConfig_t *new_config);

int NetLoggingMgrUpdateConfig(NetLoggingMgrConfig_t *new_config);

#endif