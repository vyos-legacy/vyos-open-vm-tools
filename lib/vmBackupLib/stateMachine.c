/* *************************************************************************
 * Copyright 2007 VMware, Inc.  All rights reserved. 
 * *************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 */

/*
 * stateMachine.c --
 *
 * Implements a generic state machine for executing backup operations
 * asynchronously. Since VSS is based on an asynchronous poolling model,
 * we're basing all backup operations on a similar model controlled by this
 * state machine, even if it would be more eficient to use an event-driven
 * approach in some cases.
 *
 * Overall order of execution for when no errors occur:
 *
 * Start -> OnFreeze -> run sync provider -> OnThaw -> Finalize
 *
 * The sync provider state machine depends on the particular implementation.
 * For the sync driver, it enables the driver and waits for a "snapshot done"
 * message before finishing. For the VSS subsystem, the sync provider just
 * implements a VSS backup cycle.
 */

#include "vmBackup.h"
#include "vmbackup_def.h"

#include <string.h>

#include "vm_basic_defs.h"
#include "vm_assert.h"

#include "debug.h"
#include "eventManager.h"
#include "rpcin.h"
#include "rpcout.h"
#include "str.h"
#include "util.h"

typedef enum {
   VMBACKUP_SUCCESS = 0,
   VMBACKUP_INVALID_STATE,
   VMBACKUP_SCRIPT_ERROR,
   VMBACKUP_SYNC_ERROR,
   VMBACKUP_REMOTE_ABORT,
   VMBACKUP_UNEXPECTED_ERROR
} VmBackupStatus;


#define VMBACKUP_ENQUEUE_EVENT() {                                      \
   gBackupState->timerEvent = EventManager_Add(gEventQueue,             \
                                               gBackupState->pollPeriod,\
                                               VmBackupAsyncCallback,   \
                                               NULL);                   \
   ASSERT_MEM_ALLOC(gBackupState->timerEvent);                          \
}

extern VmBackupOp *VmBackupOnFreezeScripts(void);
extern VmBackupOp *VmBackupOnThawScripts(void);

static DblLnkLst_Links *gEventQueue = NULL;
static VmBackupState *gBackupState = NULL;
static VmBackupSyncProvider *gSyncProvider = NULL;


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupKeepAliveCallback --
 *
 *    Sends a keep alive backup event to the VMX.
 *
 * Result
 *    TRUE.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

static Bool
VmBackupKeepAliveCallback(void *clientData)   // IN
{
   ASSERT(gBackupState != NULL);
   gBackupState->keepAlive = NULL;
   gBackupState->SendEvent(VMBACKUP_EVENT_KEEP_ALIVE, 0, "");
   return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * VmBackupSendEvent --
 *
 *    Sends a command to the VMX asking it to update VMDB about a new
 *    backup event.
 *
 * Result
 *    Whether sending the message succeeded.
 *
 * Side effects:
 *    Restarts the keep alive timer.
 *
 *-----------------------------------------------------------------------------
 */

static Bool
VmBackupSendEvent(const char *event,   // IN: event name
                  const uint32 code,   // IN: result code
                  const char *desc)    // IN: message related to the code
{
   Bool success;
   ASSERT(gBackupState != NULL);

   if (gBackupState->keepAlive != NULL) {
      EventManager_Remove(gBackupState->keepAlive);
   }

   success = RpcOut_sendOne(NULL, NULL,
                            VMBACKUP_PROTOCOL_EVENT_SET" %s %u %s",
                            event, code, desc);

   if (!success) {
      Debug("VmBackup: failed to send event to the VMX.\n");
   }

   gBackupState->keepAlive = EventManager_Add(gEventQueue,
                                              VMBACKUP_KEEP_ALIVE_PERIOD / 20,
                                              VmBackupKeepAliveCallback,
                                              NULL);
   ASSERT_MEM_ALLOC(gBackupState->keepAlive);
   return success;
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupFinalize --
 *
 *    Cleans up the backup state object and sends a "done" event to
 *    the VMX.
 *
 * Result
 *    None.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

static void
VmBackupFinalize(void)
{
   ASSERT(gBackupState != NULL);
   Debug("*** %s\n", __FUNCTION__);

   if (gBackupState->currentOp != NULL) {
      VmBackup_Cancel(gBackupState->currentOp);
      VmBackup_Release(gBackupState->currentOp);
   }

   gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_DONE, VMBACKUP_SUCCESS, "");

   if (gBackupState->timerEvent != NULL) {
      EventManager_Remove(gBackupState->timerEvent);
   }

   if (gBackupState->keepAlive != NULL) {
      EventManager_Remove(gBackupState->keepAlive);
   }

   free(gBackupState->volumes);
   free(gBackupState);
   gBackupState = NULL;
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupThaw --
 *
 *    Starts the execution of the "on thaw" scripts.
 *
 * Result
 *    TRUE, unless starting the scripts fails for some reason.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

static Bool
VmBackupThaw(void)
{
   Debug("*** %s\n", __FUNCTION__);
   if (!VmBackup_SetCurrentOp(gBackupState,
                              VmBackupOnThawScripts(),
                              NULL,
                              __FUNCTION__)) {
      gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                              VMBACKUP_SCRIPT_ERROR,
                              "Error when starting OnThaw scripts.");
      return FALSE;
   }

   return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupAsyncCallback --
 *
 *    Callback for the event manager. Checks the status of the current
 *    async operation being monitored, and calls the queued operations
 *    as needed.
 *
 * Result
 *    TRUE.
 *
 * Side effects:
 *    Several, depending on the backup state.
 *
 *-----------------------------------------------------------------------------
 */

static Bool
VmBackupAsyncCallback(void *clientData)   // IN
{
   Bool finalize = FALSE;

   ASSERT(gBackupState != NULL);
   Debug("*** %s\n", __FUNCTION__);

   gBackupState->timerEvent = NULL;

   if (gBackupState->currentOp != NULL) {
      VmBackupOpStatus status;

      Debug("VmBackupAsyncCallback: checking %s\n", gBackupState->currentOpName);
      status = VmBackup_QueryStatus(gBackupState->currentOp);

      switch (status) {
      case VMBACKUP_STATUS_PENDING:
         goto exit;

      case VMBACKUP_STATUS_FINISHED:
         Debug("Async request completed\n");
         VmBackup_Release(gBackupState->currentOp);
         gBackupState->currentOp = NULL;
         break;

      default:
         {
            char *errMsg = Str_Asprintf(NULL,
                                        "Asynchronous operation failed: %s\n",
                                        gBackupState->currentOpName);
            ASSERT_MEM_ALLOC(errMsg);
            gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                                    VMBACKUP_UNEXPECTED_ERROR,
                                    errMsg);
            free(errMsg);

            VmBackup_Release(gBackupState->currentOp);
            gBackupState->currentOp = NULL;
            finalize = TRUE;
            goto exit;
         }
      }
   }

   /*
    * Keep calling the registered callback until it's either NULL, or
    * an asynchronous operation is scheduled.
    */
   while (gBackupState->callback != NULL) {
      Bool cbRet;
      VmBackupCallback cb = gBackupState->callback;
      gBackupState->callback = NULL;

      cbRet = cb(gBackupState);
      if (cbRet) {
         if (gBackupState->currentOp != NULL || gBackupState->forceRequeue) {
            goto exit;
         }
      } else {
         finalize = TRUE;
      }
   }

   /*
    * If the sync provider is currently in execution and there's no
    * callback set, that means the sync provider is done executing,
    * so run the thaw scripts if we've received a "snapshot done"
    * event.
    */
   if (gBackupState->syncProviderRunning &&
       gBackupState->snapshotDone &&
       gBackupState->callback == NULL) {
      gBackupState->syncProviderRunning = FALSE;
      gBackupState->pollPeriod = 100;
      finalize = !VmBackupThaw();
      goto exit;
   }

   /*
    * If the sync provider is not running anymore, and we don't have
    * any callbacks to call anymore, it must mean we're finished.
    */
   finalize = (!gBackupState->syncProviderRunning &&
               gBackupState->callback == NULL);

exit:
   if (finalize) {
      VmBackupFinalize();
   } else {
      gBackupState->forceRequeue = FALSE;
      VMBACKUP_ENQUEUE_EVENT();
   }
   return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupEnableSync --
 *
 *    Calls the sync provider's start function.
 *
 * Result
 *    Whether the sync provider call succeeded.
 *
 * Side effects:
 *    Depends on the sync provider.
 *
 *-----------------------------------------------------------------------------
 */

static Bool
VmBackupEnableSync(VmBackupState *state)
{
   ASSERT(state != NULL);
   Debug("*** %s\n", __FUNCTION__);
   if (!gSyncProvider->start(state, gSyncProvider->clientData)) {
      state->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                       VMBACKUP_SYNC_ERROR,
                       "Error when enabling the sync provider.");
      return FALSE;
   }

   state->syncProviderRunning = TRUE;
   return TRUE;
}


/* RpcIn callbacks. */


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupStart --
 *
 *    Handler for the "vmbackup.start" message. Starts the "freeze" scripts
 *    unless there's another backup operation going on or some other
 *    unexpected error occurs.
 *
 * Result
 *    TRUE, unless an error occurs.
 *
 * Side effects:
 *    Depends on what the scripts do.
 *
 *-----------------------------------------------------------------------------
 */

Bool
VmBackupStart(char const **result,     // OUT
              size_t *resultLen,       // OUT
              const char *name,        // IN
              const char *args,        // IN
              size_t argsSize,         // IN
              void *clientData)        // IN
{
   Debug("*** %s\n", __FUNCTION__);
   if (gBackupState != NULL) {
      gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                              VMBACKUP_INVALID_STATE,
                              "Another backup operation is in progress.");
   }

   gBackupState = Util_SafeMalloc(sizeof *gBackupState);
   memset(gBackupState, 0, sizeof *gBackupState);

   gBackupState->SendEvent = VmBackupSendEvent;
   gBackupState->pollPeriod = 100;

   if (argsSize > 0) {
      gBackupState->volumes = Util_SafeStrdup(args);
   }

   gBackupState->SendEvent(VMBACKUP_EVENT_RESET, VMBACKUP_SUCCESS, "");

   if (!VmBackup_SetCurrentOp(gBackupState,
                              VmBackupOnFreezeScripts(),
                              VmBackupEnableSync,
                              "VmBackupOnFreeze")) {

      gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                              VMBACKUP_SCRIPT_ERROR,
                              "Error starting OnFreeze scripts.");
      VmBackupFinalize();
      return FALSE;
   }

   VMBACKUP_ENQUEUE_EVENT();
   return RpcIn_SetRetVals(result, resultLen, "", TRUE);
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupAbort --
 *
 *    Aborts the current operation if one is active, and stops the backup
 *    process. If the sync provider has been activated, tell it to abort
 *    the ongoing operation.
 *
 * Result
 *    TRUE
 *
 * Side effects:
 *    Possibly many.
 *
 *-----------------------------------------------------------------------------
 */

Bool
VmBackupAbort(char const **result,     // OUT
              size_t *resultLen,       // OUT
              const char *name,        // IN
              const char *args,        // IN
              size_t argsSize,         // IN
              void *clientData)        // IN
{
   if (gBackupState != NULL) {
      Debug("*** %s\n", __FUNCTION__);

      if (gBackupState->currentOp != NULL) {
         VmBackup_Cancel(gBackupState->currentOp);
         VmBackup_Release(gBackupState->currentOp);
         gBackupState->currentOp = NULL;
      }

      if (gBackupState->syncProviderRunning) {
         gSyncProvider->abort(gBackupState, gSyncProvider->clientData);
      }

      gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ABORT,
                              VMBACKUP_REMOTE_ABORT,
                              "Remote abort.");
      VmBackupFinalize();

      return RpcIn_SetRetVals(result, resultLen, "", TRUE);
   } else {
      return RpcIn_SetRetVals(result, resultLen,
                              "Error: no backup in progress", FALSE);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackupSnapshotDone --
 *
 *    Sets the flag that says it's OK to disable the sync driver.
 *
 * Result
 *    TRUE
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
VmBackupSnapshotDone(char const **result,    // OUT
                     size_t *resultLen,      // OUT
                     const char *name,       // IN
                     const char *args,       // IN
                     size_t argsSize,        // IN
                     void *clientData)       // IN
{
   if (gBackupState != NULL) {
      Debug("*** %s\n", __FUNCTION__);
      if (!gSyncProvider->snapshotDone(gBackupState, gSyncProvider->clientData)) {
         gBackupState->SendEvent(VMBACKUP_EVENT_REQUESTOR_ERROR,
                                 VMBACKUP_SYNC_ERROR,
                                 "Error when notifying the sync provider.");
         VmBackupFinalize();
      } else {
         gBackupState->snapshotDone = TRUE;
      }
      return RpcIn_SetRetVals(result, resultLen, "", TRUE);
   } else {
      return RpcIn_SetRetVals(result, resultLen,
                              "Error: no backup in progress", FALSE);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackup_Init --
 *
 *    Registers the RpcIn callbacks for the backup protocol.
 *
 * Result
 *    TRUE.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
VmBackup_Init(RpcIn *rpcin,                     // IN
              DblLnkLst_Links *eventQueue,      // IN
              VmBackupSyncProvider *provider)   // IN
{
   ASSERT(gEventQueue == NULL);
   ASSERT(eventQueue != NULL);
   ASSERT(provider != NULL);
   ASSERT(provider->start != NULL);
   ASSERT(provider->abort != NULL);
   ASSERT(provider->snapshotDone != NULL);
   ASSERT(provider->release != NULL);

   RpcIn_RegisterCallback(rpcin,
                          VMBACKUP_PROTOCOL_START,
                          VmBackupStart,
                          NULL);
   RpcIn_RegisterCallback(rpcin,
                          VMBACKUP_PROTOCOL_ABORT,
                          VmBackupAbort,
                          NULL);
   RpcIn_RegisterCallback(rpcin,
                          VMBACKUP_PROTOCOL_SNAPSHOT_DONE,
                          VmBackupSnapshotDone,
                          NULL);
   gEventQueue = eventQueue;
   gSyncProvider = provider;
   return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 *  VmBackup_Shutdown --
 *
 *    Unregisters the RpcIn callbacks.
 *
 * Result
 *    None.
 *
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

void
VmBackup_Shutdown(RpcIn *rpcin)
{
   if (gBackupState != NULL) {
      VmBackupFinalize();
   }

   gSyncProvider->release(gSyncProvider);
   gSyncProvider = NULL;

   RpcIn_UnregisterCallback(rpcin, VMBACKUP_PROTOCOL_START);
   RpcIn_UnregisterCallback(rpcin, VMBACKUP_PROTOCOL_ABORT);
   RpcIn_UnregisterCallback(rpcin, VMBACKUP_PROTOCOL_SNAPSHOT_DONE);
   gEventQueue = NULL;
}

