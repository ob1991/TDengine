/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TDENGINE_DNODE_LOG_H
#define TDENGINE_DNODE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif
#include "taoserror.h"
#include "taosmsg.h"
#include "tlog.h"
#include "trpc.h"
#include "tglobal.h"
#include "dnode.h"
#include "vnode.h"

extern int32_t dDebugFlag;

#ifdef LOG_LINE_OPEN
#define dFatal(...) { if (dDebugFlag & DEBUG_FATAL) { taosPrintLog("DND FATAL ", 255, __FILE__, __LINE__, __VA_ARGS__); dnodeIncDnodeError(); }}
#define dError(...) { if (dDebugFlag & DEBUG_ERROR) { taosPrintLog("DND ERROR ", 255, __FILE__, __LINE__, __VA_ARGS__); dnodeIncDnodeError(); }}
#define dWarn(...)  { if (dDebugFlag & DEBUG_WARN)  { taosPrintLog("DND WARN ", 255, __FILE__, __LINE__, __VA_ARGS__); }}
#define dInfo(...)  { if (dDebugFlag & DEBUG_INFO)  { taosPrintLog("DND ", 255, __FILE__, __LINE__, __VA_ARGS__); }}
#define dDebug(...) { if (dDebugFlag & DEBUG_DEBUG) { taosPrintLog("DND ", dDebugFlag, __FILE__, __LINE__, __VA_ARGS__); }}
#define dTrace(...) { if (dDebugFlag & DEBUG_TRACE) { taosPrintLog("DND ", dDebugFlag, __FILE__, __LINE__, __VA_ARGS__); }}
#else
#define dFatal(...) { if (dDebugFlag & DEBUG_FATAL) { taosPrintLog("DND FATAL ", 255, __VA_ARGS__); dnodeIncDnodeError(); }}
#define dError(...) { if (dDebugFlag & DEBUG_ERROR) { taosPrintLog("DND ERROR ", 255, __VA_ARGS__); dnodeIncDnodeError(); }}
#define dWarn(...)  { if (dDebugFlag & DEBUG_WARN)  { taosPrintLog("DND WARN ", 255, __VA_ARGS__); }}
#define dInfo(...)  { if (dDebugFlag & DEBUG_INFO)  { taosPrintLog("DND ", 255, __VA_ARGS__); }}
#define dDebug(...) { if (dDebugFlag & DEBUG_DEBUG) { taosPrintLog("DND ", dDebugFlag, __VA_ARGS__); }}
#define dTrace(...) { if (dDebugFlag & DEBUG_TRACE) { taosPrintLog("DND ", dDebugFlag, __VA_ARGS__); }}
#endif

typedef enum {
  TSDB_RUN_STATUS_INITIALIZE,
  TSDB_RUN_STATUS_RUNING,
  TSDB_RUN_STATUS_STOPPED
} SRunStatus;

SRunStatus dnodeGetRunStatus();

#ifdef __cplusplus
}
#endif

#endif
