/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila[at]continental-corporation.com
*
* Implementation for Persistence Administration Service
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2016.06.22 Cosmin Cernat      Bugzilla Bug 437:  Remove timeout calculation dependency on the system time.
*                                                  Moved WDOG re-triggering to glib loop.
* 2016.06.22 Cosmin Cernat      Bugzilla Bug 437:  Open existing sync objects to support a PAS restart
* 2014.09.11 Petrica Manoila    OvipRbt#8870:      Fixed timeout calculation in WDOG retriggering mechanism.
* 2013.10.07 Petrica Manoila    CSP_WZ#5965:       Fixed timeout calculation for mq_timedreceive
* 2013.09.23 Ionut Ieremie      CSP_WZ#5781:       Watchdog timeout of pas-daemon
* 2013.09.17 Petrica Manoila    CSP_WZ#5633:       Ignore requests received before registration to NSM
* 2013.09.03 Petrica Manoila    CSP_WZ#4480:       Implement watchdog functionality
* 2013.05.23 Petrica Manoila    CSP_WZ#3831:       Adapt PAS to be started as a service by systemd
* 2013.04.19 Petrica Manoila    CSP_WZ#3424:       Add PAS IF extension for "restore to default"
* 2013.04.04 Petrica Manoila    CSP_WZ#2739:       Using PersCommonIPC for requests to PCL
* 2013.03.26 Petrica Manoila    CSP_WZ#3171:       Update PAS registration to NSM
* 2012.11.16 Ionut Ieremie      CSP_WZ#1280:       persadmin_response_msg_s: rename bResult to result and change type to long
* 2012.11.13 Petrica Manoila    CSP_WZ#1280:       Implemented DBus registration/notification mechanism
* 2012.11.12 Ionut Ieremie      CSP_WZ#1280:       Created (only stubs for access lib part introduced)
*
**********************************************************************************************************************/

/* ---------------------- include files  --------------------------------- */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <stddef.h>
#include <unistd.h>
#include <sys/file.h>
#include "fcntl.h"
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <mqueue.h>
#include <semaphore.h>
#include <dlt.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>                 /* Syslog messages */
#include <systemd/sd-daemon.h>      /* Systemd wdog    */

#include "persComTypes.h"
#include "persComErrors.h"

#include "persistence_admin_service.h"
#include "ssw_pers_admin_access_lib.h"
#include "ssw_pers_admin_pcl.h"
#include "ssw_pers_admin_dbus.h"
#include "ssw_pers_admin_pcl.h"
#include "ssw_pers_admin_service.h"


/* ---------- local defines, macros, constants and type definitions ------------ */

/* Definition of the process name for this application. Used for logging. */
#define        PERSISTENCE_ADMIN_PROC_NAME         "PersistenceAdminService"

#define        LT_HDR                              "PAS >>"

#define        INIT_PROC_PID                        1

#define        OS_SUCCESS_CODE                      0
#define        OS_ERROR_CODE                      (-1)
#define        OS_INVALID_HANDLE                  (-1)

#define        DAEMONIZE_FAIL_RET_VAL             (-1)
#define        DAEMONIZE_SUCCESS_RET_VAL            1
#define        DAEMONIZE_PARENT_RET_VAL             0


#define        PAS_PID_FILE_NAME                    "pers_admin_svc.pid"
#define        PAS_PID_FILE_PATH                    "/tmp/" PAS_PID_FILE_NAME
#define        PAS_SYSPROC_PATH                     "/proc"
#define        PAS_PID_MAX_SIZE                     10
#define        PAS_PID_PROC_PATH_MAX_SIZE           (PAS_PID_MAX_SIZE + 6)    /* PAS_PID_MAX_SIZE + strlen(PAS_SYSPROC_PATH) + 1 */

#define        ONE_MS_IN_US                         ((sint64_t)1000)                                /* one ms in microseconds */
#define        ONE_SEC_IN_US                        ((sint64_t)1000 * ONE_MS_IN_US)                    /* one sec in microseconds */
#define        PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT_US  ((sint64_t)((sint64_t)50 * ONE_MS_IN_US))       /* 50 ms in microseconds */


/* ----------global variables. initialization of global contexts ------------ */

DLT_DECLARE_CONTEXT(persAdminSvcDLTCtx);

static sint32_t          g_hPASPidFile                = OS_INVALID_HANDLE;

static bool_t            g_bOpInProgress              = false;
static bool_t            g_bShutdownInProgress        = false;
static volatile bool_t   g_bRunAccessLibThread        = true;

static pthread_mutex_t   g_shutdownMtx;
static pthread_cond_t    g_shutdownCond;

/* Access lib communication objects */
static sem_t *           g_pSyncSem                   = NIL;
static mqd_t             g_mqdMsgQueueRequest         = OS_INVALID_HANDLE;
static mqd_t             g_mqdMsgQueueResponse        = OS_INVALID_HANDLE;


/* ---------------------- local functions declarations ---------------------------------- */

/**
 * \brief  Access lib thread. Thread responsible to perform (on behalf of the clients)
 * the requests available in persistence_admin_service.h
 *
 * \param  arg:    thread arguments
 *
 * \return NIL
 **/
static void* persadmin_AccessLibThread (void* arg);

/**
 * \brief  Initialize access lib communication objects. (to receive the requests available in persistence_admin_service.h)
 *
 * \return true if successful, false otherwise
 **/
static bool_t persadmin_InitAccessLibComm (void);

/**
 * \brief  Close access lib communication objects.
 *
 * \return void
 **/
static void persadmin_CloseAccessLibComm (void);

/**
 * \brief  Process the specified request (from the requests available in persistence_admin_service.h)
 *
 * \param  psRequest:    request details
 * \param  pResult_out: request result
 *
 * \return true if successful, false otherwise
 **/
static bool_t persadmin_ProcessRequest (persadmin_request_msg_s* psRequest, long* pResult_out);

/**
 * \brief  The function is called from main and turns the current process into a daemon.
 *
 * \param  pProcessName:    Specifies the name of the process to be daemonized.
 *
 * \return 0 : parent process returned; positive value: success; negative value: error
 **/
static sint_t persadmin_DaemonizeProcess (pstr_t pProcessName);

/**
 * \brief  Run main daemon actions.
 *
 * \return 0 : success; negative value: error
 **/
static sint_t persadmin_RunDaemon(void);

/**
 * \brief  Signal handler.
 *
 * \param  signum :    signal identifier
 *
 * \return void
 **/
static void persadmin_HandleTerm(int signum);

/**
 * \brief  The function is called from main. It checks if the  unlocks the PID file and closes the opened handles.
 *
 * \return true if the process is already running, false otherwise
 **/
static bool_t persadmin_IsAlreadyRunning(void);

/**
 * \brief  The function is called from main and writes the child PID to be used by systemd
 *
 * \return true if successful, false otherwise
 **/
static bool_t persadmin_LockPidFile(void);

/**
 * \brief  The function is called from main. It unlocks the PID file and closes the opened handles.
 *
 * \return void
 **/
static void persadmin_EndPASProcess(void);


/**
 * \brief Read a message from the specified message queue.
 *           If there is no message in the queue, the call will block for the specified timeout, or until a message is available.
 *
 * \param mqdes              [in]    message queue descriptor (obtained with mq_open)
 * \param msg_ptr            [out]   pointer to the buffer where the message obtained is placed
 * \param msg_len            [in]    the size of the buffer pointed to by msg_ptr, this must be greater than the mq_msgsize attribute of the queue

 *
 * \return on success, the number of bytes in the received message; on error, a negative value is returned; 0 is returned in case of timeout
 */
static sint_t persadmin_ReadMsgFromQueue( mqd_t        mqdes,
                                          uint8_t     *msg_ptr,
                                          size_t       msg_len);

/* ---------------------- public functions definition ---------------------------------- */

/**
 * \brief Set PAS shutdown state
 *
 * \param state the shutdown state : true if the shutdown is pending, false otherwise
 *
 * \return : void
 */
void persadmin_set_shutdown_state(bool_t state)
{
    if(OS_SUCCESS_CODE != pthread_mutex_lock(&g_shutdownMtx))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
                                                    DLT_STRING("Failed to lock the shutdown mutex. Error :");
                                                    DLT_STRING(strerror(errno)));
    }
    else
    {
        if(true == state)
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Shutdown request received. Waiting for active operations to complete..."));

            /* wait for any pending operation */
            while (true == g_bOpInProgress) {
                pthread_cond_wait(&g_shutdownCond, &g_shutdownMtx);
            }

            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Completed all running operations before shutdown."));
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Shutdown canceled notification received."));
        }

        g_bShutdownInProgress = state;

        pthread_mutex_unlock(&g_shutdownMtx);
    }
}


/**
 * \brief  Allow the modification of the resource properties from data (key-values and files)
 *
 * \param  resource_config_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified properties in the resource configuration; negative value: error code
 */
long persadmin_resource_config_change_properties(char* resource_config_file)
{
    if(NIL != resource_config_file)
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                    DLT_STRING("persadmin_resource_config_change_properties(");
                                                    DLT_STRING(resource_config_file);
                                                    DLT_STRING(")"));
    }

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                DLT_STRING("persadmin_resource_config_change_properties - NOT IMPLEMENTED"));

    return (long)PAS_FAILURE_OPERATION_NOT_SUPPORTED;
}


/**
 * \brief Allow the copy of user related data between different users
 *
 * \param src_user_no the user ID source
 * \param src_seat_no the seat number source (seat 0 to 3)
 * \param dest_user_no the user ID destination
 * \param dest_seat_no the seat number destination (seat 0 to 3)
 *
 * \return positive value: number of bytes copied; negative value: error code
 */
long persadmin_user_data_copy(unsigned int src_user_no, unsigned int src_seat_no, unsigned int dest_user_no, unsigned int dest_seat_no)
{
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                DLT_STRING("persadmin_user_data_copy(");
                                                DLT_UINT(src_user_no);
                                                DLT_UINT(src_seat_no);
                                                DLT_UINT(dest_user_no);
                                                DLT_UINT(dest_seat_no);
                                                DLT_STRING(")"));

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                DLT_STRING("persadmin_user_data_copy - NOT IMPLEMENTED"));

    return (long)PAS_FAILURE_OPERATION_NOT_SUPPORTED;
}


/* ---------------------- local functions definition ---------------------- */

static bool_t persadmin_InitAccessLibComm(void)
{
    bool_t     bEverythingOK     = true;
    sint_t    sErrnoVal;
    uint_t    uSemOpenFlag     = (uint_t)O_CREAT | (uint_t)O_EXCL;
    uint_t    uSemModeFlags    = (uint_t)S_IRWXU | (uint_t)S_IRWXG | (uint_t)S_IRWXO;
    uint_t    uQueueOpenFlag    = (uint_t)O_CREAT | (uint_t)O_EXCL | (uint_t)O_RDWR;
    uint_t    uQueueModeFlags    = (uint_t)S_IRWXU | (uint_t)S_IRWXG | (uint_t)S_IRWXO;

    struct mq_attr mq_attr_request;
    struct mq_attr mq_attr_response;
    mq_attr_request.mq_maxmsg = 1; /* max 1 message in the queue */
    mq_attr_request.mq_msgsize = sizeof(persadmin_request_msg_s);
    mq_attr_response.mq_maxmsg = 1; /* max 1 message in the queue */
    mq_attr_response.mq_msgsize = sizeof(persadmin_response_msg_s);

    /* create access lib sync semaphore */
    g_pSyncSem = sem_open(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE, (int)uSemOpenFlag, uSemModeFlags, 1);
    if(NIL == g_pSyncSem)
    {
        sErrnoVal = errno;

        /* ignore error in case the object already exists */
        if(EEXIST == sErrnoVal)
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                        DLT_STRING("Semaphore");
                                                        DLT_STRING(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
                                                        DLT_STRING("already created."));
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                        DLT_STRING("Failed to create semaphore");
                                                        DLT_STRING(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
                                                        DLT_STRING(". Error :");
                                                        DLT_STRING(strerror(errno)));
            bEverythingOK = false;
        }
    }
    else
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                    DLT_STRING("Semaphore");
                                                    DLT_STRING(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
                                                    DLT_STRING("created successfully."));
    }

    /* create access lib request queue*/
    if(bEverythingOK)
    {
        g_mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, (int)uQueueOpenFlag, uQueueModeFlags, &mq_attr_request);
        if(OS_INVALID_HANDLE == g_mqdMsgQueueRequest)
        {
            sErrnoVal = errno;

            /* open the existing request queue */
            if(EEXIST == sErrnoVal)
            {
                g_mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_RDWR, uQueueModeFlags, &mq_attr_request);
                if(OS_INVALID_HANDLE == g_mqdMsgQueueRequest)
                {
                    sErrnoVal = errno;

                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                                DLT_STRING("Failed to open request queue");
                                                                DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
                                                                DLT_STRING(". Error :");
                                                                DLT_STRING(strerror(sErrnoVal)));
                    bEverythingOK = false;
                }
                else
                {
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                                DLT_STRING("Request queue");
                                                                DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
                                                                DLT_STRING("opened successfully."));
                }
            }
            else
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                            DLT_STRING("Failed to open request queue");
                                                            DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
                                                            DLT_STRING(". Error :");
                                                            DLT_STRING(strerror(sErrnoVal)));
                bEverythingOK = false;
            }
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                        DLT_STRING("Request queue");
                                                        DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
                                                        DLT_STRING("created successfully."));
        }
    }

    /* create access lib response queue*/
    if(bEverythingOK)
    {
        g_mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, (int)uQueueOpenFlag, uQueueModeFlags, &mq_attr_response);
        if(OS_INVALID_HANDLE == g_mqdMsgQueueResponse)
        {
            sErrnoVal = errno;

            /* open the existing response queue */
            if(EEXIST == sErrnoVal)
            {
                g_mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDWR, uQueueModeFlags, &mq_attr_response);
                if(OS_INVALID_HANDLE == g_mqdMsgQueueResponse)
                {
                    sErrnoVal = errno;

                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                                DLT_STRING("Failed to open response queue");
                                                                DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
                                                                DLT_STRING(". Error :");
                                                                DLT_STRING(strerror(sErrnoVal)));
                    bEverythingOK = false;
                }
                else
                {
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                                DLT_STRING("Response queue");
                                                                DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
                                                                DLT_STRING("opened successfully."));
                }
            }
            else
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                            DLT_STRING("Failed to create response queue");
                                                            DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
                                                            DLT_STRING(". Error :");
                                                            DLT_STRING(strerror(errno)));
                bEverythingOK = false;
            }
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                        DLT_STRING("Response queue");
                                                        DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
                                                        DLT_STRING("created successfully."));
        }
    }

    return bEverythingOK;
}


static void persadmin_CloseAccessLibComm(void)
{
    if(NIL != g_pSyncSem)
    {
        sem_close(g_pSyncSem);
        sem_unlink(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
    }

    if(OS_INVALID_HANDLE != g_mqdMsgQueueRequest)
    {
        mq_close(g_mqdMsgQueueRequest);
        mq_unlink(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
    }

    if(OS_INVALID_HANDLE != g_mqdMsgQueueResponse)
    {
        mq_close(g_mqdMsgQueueResponse);
        mq_unlink(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
    }
}


static bool_t persadmin_ProcessRequest(persadmin_request_msg_s* psRequest, long* pResult_out)
{
    bool_t     bEverythingOK     = true;
    bool_t    bLockedMtx        = false;
    long result             = -1 ;

    /* check if PAS is registered to NSM */
    if(false == persadmin_IsRegisteredToNSM())
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, DLT_STRING("Cannot process request. Not registered to NSM yet..."));

        bEverythingOK = false;
    }


    /* check if a shutdown is in progress */
    if(true == bEverythingOK)
    {
        if(OS_SUCCESS_CODE != pthread_mutex_lock(&g_shutdownMtx))
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
                                                        DLT_STRING("Failed to lock the shutdown mutex. Error :");
                                                        DLT_STRING(strerror(errno)));
            bEverythingOK = false;
        }
        else
        {
            bLockedMtx = true;
        }
    }

    if(true == bEverythingOK)
    {
        if(true == g_bShutdownInProgress)
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, DLT_STRING("Cannot process request. Shutdown is in progress..."));

            *pResult_out = PAS_FAILURE_OS_RESOURCE_ACCESS;

            bEverythingOK = false;
        }
    }

    if(true == bEverythingOK)
    {
        g_bOpInProgress = true;

        pthread_mutex_unlock(&g_shutdownMtx);
        bLockedMtx = false;

        switch(psRequest->eRequest)
        {
            case PAS_Req_DataBackupCreate:
            {
                result = persadmin_data_backup_create(psRequest->type, psRequest->path1, psRequest->path2, psRequest->user_no, psRequest->seat_no) ;
                break ;
            }
            case PAS_Req_DataBackupRecovery:
            {
                result = persadmin_data_backup_recovery(psRequest->type, psRequest->path1, psRequest->path2, psRequest->user_no, psRequest->seat_no) ;
                break ;
            }
            case PAS_Req_DataRestoreToDefault:
            {
                result = persadmin_data_restore_to_default(psRequest->type, psRequest->defaultSource, psRequest->path2, psRequest->user_no, psRequest->seat_no);
                break;
            }
            case PAS_Req_DataFolderExport:
            {
                result = persadmin_data_folder_export(psRequest->type, psRequest->path1) ;
                break ;
            }
            case PAS_Req_DataFolderImport:
            {
                result = persadmin_data_folder_import(psRequest->type, psRequest->path1) ;
                break ;
            }
            case PAS_Req_ResourceConfigAdd:
            {
                result = persadmin_resource_config_add(psRequest->path1) ;
                break ;
            }
            case PAS_Req_ResourceConfigChangeProperties:
            {
                result = persadmin_resource_config_change_properties(psRequest->path1) ;
                break ;
            }
            case PAS_Req_UserDataCopy:
            {
                result = persadmin_user_data_copy(psRequest->src_user_no, psRequest->src_seat_no, psRequest->dest_user_no, psRequest->dest_seat_no) ;
                break ;
            }
            case PAS_Req_UserDataDelete:
            {
                result = persadmin_user_data_delete(psRequest->user_no, psRequest->seat_no) ;
                break ;
            }
            default:
            {
                /* shall never happen */
                bEverythingOK = false ;
                break ;
            }
        }

        /* operation finished */
        if(OS_SUCCESS_CODE != pthread_mutex_lock(&g_shutdownMtx))
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
                                                        DLT_STRING("Failed to lock the shutdown mutex. Error :");
                                                        DLT_STRING(strerror(errno)));
            bEverythingOK = false;
        }
        else
        {
            bLockedMtx = true;

            g_bOpInProgress = false;

            /* notify any waiting thread */
            pthread_cond_broadcast(&g_shutdownCond);
        }
    }

    if(true == bLockedMtx)
    {
        pthread_mutex_unlock(&g_shutdownMtx);
        bLockedMtx = false;
    }

    *pResult_out = bEverythingOK ? result : PAS_FAILURE ;

    return bEverythingOK;

}


static void* persadmin_AccessLibThread(void* arg)
{
    bool_t             bEverythingOK         = true;
    sint_t            sFctRetVal;

    persadmin_request_msg_s     sRequest;
    persadmin_response_msg_s     sResponse;

    /* initialize the communication channel */
    bEverythingOK = persadmin_InitAccessLibComm();

    if(true == bEverythingOK)
    {

        /* everything is initialized now, so we can process requests */
        while(true == g_bRunAccessLibThread)
        {
            /* wait for request */
            sFctRetVal = persadmin_ReadMsgFromQueue(g_mqdMsgQueueRequest, (uint8_t*)&sRequest, sizeof(sRequest));
            if(sFctRetVal < PERS_COM_SUCCESS)
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                            DLT_STRING("Failed waiting for access lib requests. Error :");
                                                            DLT_INT(sFctRetVal));
                g_bRunAccessLibThread = false;
            }
            else
            {
                switch(sFctRetVal)
                {

                    case PERS_COM_SUCCESS:
                    {
                        /* timeout - repeat the call */
                    }
                    break;

                    case sizeof(sRequest):
                    {
                        long     requestResult = -1;
                        sint_t    sFctRetVal;

                        /* request received in the right format */
                        sFctRetVal = persadmin_SendLockAndSyncRequestToPCL();
                        if(PERS_COM_SUCCESS != sFctRetVal)
                        {
                            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                                        DLT_STRING("persadmin_SendLockAndSyncRequestToPCL call failed. Error :");
                                                                        DLT_INT(sFctRetVal));
                            requestResult = PAS_FAILURE;
                        }
                        else
                        {
                            if(false == persadmin_ProcessRequest(&sRequest, &requestResult))
                            {
                                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                                            DLT_STRING("persadmin_ProcessRequest call failed."));
                            }

                            persadmin_SendUnlockRequestToPCL();
                        }

                        sResponse.result = requestResult;

                        if(OS_ERROR_CODE == mq_send(g_mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), 0))
                        {
                            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,    DLT_STRING(LT_HDR);
                                                                        DLT_STRING("Failed to send response. Error :");
                                                                        DLT_STRING(strerror(errno)));

                            g_bRunAccessLibThread = false;
                        }
                    }
                    break;

                    default:
                    {
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,    DLT_STRING(LT_HDR);
                                                                    DLT_STRING("Invalid request received. Returned");
                                                                    DLT_INT(sFctRetVal);
                                                                    DLT_STRING("bytes. Expected");
                                                                    DLT_INT(sizeof(sRequest));
                                                                    DLT_STRING("bytes"));
                        /* ignore request and continue */
                    }
                    break;
                }
            }
        }
    }

    /* close the communication channel */
    persadmin_CloseAccessLibComm();

    return NIL;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static sint_t persadmin_DaemonizeProcess(pstr_t pProcessName)
{
    pid_t    pid, sid;
    sint_t    sRetVal         = DAEMONIZE_SUCCESS_RET_VAL;

    if(NIL == pProcessName)
    {
        syslog(LOG_ERR, "PAS >> Invalid argument in persadmin_DaemonizeProcess call.");

        sRetVal = DAEMONIZE_FAIL_RET_VAL;
    }

    if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
    {
        syslog(LOG_INFO, "PAS >> Daemon process %s starting.", pProcessName);

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "PAS >> Unable to fork, code=%d (%s).", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

            sRetVal = DAEMONIZE_FAIL_RET_VAL;
        }
        else
        {
            if(pid > 0)
            {
                syslog(LOG_ERR, "PAS >> Child(daemon) process created successfully with PID=%d.", pid);

                sRetVal = DAEMONIZE_PARENT_RET_VAL;
            }
        }

        if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
        {
            /* Ignore or perform the default action for some of the signals in the child process */
            (void)signal(SIGCHLD,SIG_DFL); /* A child process dies */
            (void)signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
            (void)signal(SIGTTOU,SIG_IGN);
            (void)signal(SIGTTIN,SIG_IGN);
            (void)signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
            (void)signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

            syslog(LOG_INFO, "PAS >> Child signals ignored.");

            /* Change the file mode mask */
            (void)umask(0);

            syslog(LOG_INFO, "PAS >> File mode mask changed.");

            /* Get a unique Session ID from the kernel */
            sid = setsid();
            if (OS_ERROR_CODE == sid)
            {
                syslog(LOG_ERR, "PAS >> Unable to get a unique session id. Error code %d (%s).", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

                sRetVal = DAEMONIZE_FAIL_RET_VAL;
            }
            else
            {
                syslog(LOG_INFO, "PAS >> New SID for child process: %d.", sid);
            }
        }

        if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
        {
            /* Change the current working directory.
             * This prevents the current directory from being locked; hence not being able to remove it.
             * The root directory cannot be unmounted.
             */
            if ((chdir("/")) < 0)
            {
                syslog(LOG_ERR, "PAS >> Unable to change directory to root path. Error code %d (%s)", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

                sRetVal = DAEMONIZE_FAIL_RET_VAL;
            }
            else
            {
                syslog(LOG_INFO, "PAS >> Current working directory changed for child process.");
            }
        }

        if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
        {
            /* Redirect standard files to /dev/null */
            (void)freopen( "/dev/null", "r", stdin);
            (void)freopen( "/dev/null", "w", stdout);
            (void)freopen( "/dev/null", "w", stderr);

            syslog(LOG_INFO, "PAS >> Standard file descriptors redirected to /dev/null.");
        }
    }

    return sRetVal;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static sint_t persadmin_RunDaemon(void)
{
    sint_t                    sRetVal                = EXIT_SUCCESS;
    sint_t                    retVal                 = PERS_COM_SUCCESS;
    bool_t                    bSyncObjInitialized    = false;
    bool_t                    bLockedPIDFile         = false;
    struct     sigaction      action;

    /* Initialize the logging interface */
    DLT_REGISTER_APP("PERS", "Persistence Administration Service");
    DLT_REGISTER_CONTEXT(persAdminSvcDLTCtx, "PADM", "Persistence Administration Service Context");

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING(PERSISTENCE_ADMIN_PROC_NAME);
                                                DLT_STRING(" starting..."));

    /* Set SIGTERM handler */
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = &persadmin_HandleTerm;
    sigaction(SIGTERM, &action, NULL);

    /* Lock the PID file */
    if(false == persadmin_LockPidFile())
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to lock the PID file."));

        sRetVal = EXIT_FAILURE;
    }
    else
    {
        bLockedPIDFile = true;
    }

    if(EXIT_FAILURE != sRetVal)
    {
        /* Initialize synchronization objects */
        if(OS_SUCCESS_CODE == pthread_mutex_init (&g_shutdownMtx, NULL))
        {
            if(OS_SUCCESS_CODE == pthread_cond_init (&g_shutdownCond, NULL))
            {
                bSyncObjInitialized = true;
            }
            else
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init shutdown sync. obj. (cond)"));

                pthread_mutex_destroy(&g_shutdownMtx);

                sRetVal = EXIT_FAILURE;
            }
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init shutdown sync. obj. (mtx)"));

            sRetVal = EXIT_FAILURE;
        }

        if(true == bSyncObjInitialized)
        {
            pthread_t        g_hAccessLibThread    = 0;

            /* Create the AccessLib thread */
            if(OS_SUCCESS_CODE != pthread_create(&g_hAccessLibThread, NIL, &persadmin_AccessLibThread, NIL))
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to create AccessLib thread."));
            }
            else
            {
                /* Init PAS IPC protocol */
                retVal = persadmin_InitIpc();
                if(PERS_COM_SUCCESS != retVal)
                {
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init PAS IPC component."));

                    sRetVal = EXIT_FAILURE;
                }
                else
                {
                    /* Notify systemd that PAS is ready */
                    (void)sd_notify(0, "READY=1");

                    /* Init DBus connection and run the PAS main loop */
                    persadmin_RunDBusMainLoop();
                }

                /* signal the AccessLib thread to stop */
                g_bRunAccessLibThread = false;

                /* Wait for the AccessLib thread */
                pthread_join(g_hAccessLibThread, NIL);

                /* Close synchronization objects */
                pthread_mutex_destroy(&g_shutdownMtx);
                pthread_cond_destroy(&g_shutdownCond);
            }
        }
    }

    if(true == bLockedPIDFile)
    {
        /* Unlock PID file and close handles */
        persadmin_EndPASProcess();
    }

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING(PERSISTENCE_ADMIN_PROC_NAME);
                                                DLT_STRING(" stopped."));
    DLT_UNREGISTER_CONTEXT(persAdminSvcDLTCtx);
    DLT_UNREGISTER_APP();

    return sRetVal;

}


static bool_t persadmin_LockPidFile(void)
{
    str_t     strPid[PAS_PID_MAX_SIZE];
    bool_t    bRetVal = true;

    g_hPASPidFile = open(PAS_PID_FILE_PATH, (uint_t)O_RDWR | (uint_t)O_CREAT, 0640);
    if (OS_INVALID_HANDLE == g_hPASPidFile)
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to create PID file:");
                                                    DLT_STRING(PAS_PID_FILE_PATH));
        bRetVal = false;
    }
    else
    {
        if (OS_INVALID_HANDLE == lockf(g_hPASPidFile, F_TLOCK, 0))
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to lock PID file:");
                                                        DLT_STRING(PAS_PID_FILE_PATH));
            bRetVal = false;
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING("Locked PID file:");
                                                        DLT_STRING(PAS_PID_FILE_PATH));

            /* write PID to be used by systemd */
            snprintf(strPid, sizeof(strPid), "%d", getpid());
            write(g_hPASPidFile, strPid, strlen(strPid));
        }
    }

    return bRetVal;
}


static void persadmin_EndPASProcess(void)
{
    /* unlock PID file */
    if(OS_INVALID_HANDLE != g_hPASPidFile)
    {
        if(OS_INVALID_HANDLE == lockf(g_hPASPidFile, F_ULOCK, 0))
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to unlock PID file:");
                                                        DLT_STRING(PAS_PID_FILE_PATH));
        }
        else
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING("Unlocked PID file:");
                                                        DLT_STRING(PAS_PID_FILE_PATH));
        }
    }

    /* close PID file */
    if(OS_INVALID_HANDLE == close(g_hPASPidFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to close PID file:");
                                                    DLT_STRING(PAS_PID_FILE_PATH));
    }
    
    g_hPASPidFile = OS_INVALID_HANDLE;

    /* remove PID file */
    if(OS_INVALID_HANDLE == remove(PAS_PID_FILE_PATH))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to remove PID file:");
                                                    DLT_STRING(PAS_PID_FILE_PATH));
    }
}


static bool_t persadmin_IsAlreadyRunning(void)
{
    sint32_t         hFile         = OS_INVALID_HANDLE;
    struct stat      sStat;
    str_t            strPid[PAS_PID_MAX_SIZE];
    str_t            strProcPath[PAS_PID_PROC_PATH_MAX_SIZE];
    uint32_t         u32PidVal     = 0;
    bool_t           bRetVal     = true;

    /* check PID file */
    hFile = stat(PAS_PID_FILE_PATH, &sStat);
    if (OS_INVALID_HANDLE == hFile)
    {
        /* PAS process not running. The PID file does not exists */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING("Failed to open PAS PID file. Process is not running already. Error :");
                                                    DLT_STRING(strerror(errno)));
        bRetVal =  false;
    }
    else
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING("Found PAS PID file");
                                                    DLT_STRING(PAS_PID_FILE_PATH));

        /* PID file exists, check if the process with the specified PID is running */
        hFile = open(PAS_PID_FILE_PATH, O_RDONLY);
        if(OS_INVALID_HANDLE == hFile)
        {
            /* the process might be running */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("Failed to open file");
                                                        DLT_STRING(PAS_PID_FILE_PATH));
        }
    }

    if(OS_INVALID_HANDLE != hFile)
    {
        /* read PID and check process status */
        memset(strPid, 0, sizeof(strPid));
        read(hFile, strPid, (PAS_PID_MAX_SIZE - 1));
        close(hFile);

        if(strlen(strPid) > 0)
        {
            sscanf(strPid, "%d", &u32PidVal);

            snprintf(strProcPath, sizeof(strProcPath), "%s/%d", PAS_SYSPROC_PATH, u32PidVal);

            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,     DLT_STRING("Checking status for process");
                                                        DLT_STRING(strProcPath));

            hFile = stat(strProcPath, &sStat);
            if (OS_INVALID_HANDLE == hFile)
            {
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("File '");
                                            DLT_STRING(strProcPath);
                                            DLT_STRING("' exists, but process with PID:");
                                            DLT_STRING(strPid);
                                            DLT_STRING(" is not running."));
                bRetVal = false;
            }
        }
    }

    return bRetVal;
}


static void persadmin_HandleTerm(int signum)
{
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,    DLT_STRING(LT_HDR);
                                                DLT_STRING("SIGTERM received..."));

    /* quit DBus main loop */
    persadmin_QuitDBusMainLoop();
}


static sint_t persadmin_ReadMsgFromQueue(mqd_t        mqdes,
                                         uint8_t     *msg_ptr,
                                         size_t       msg_len)
{
    sint_t    sRetVal    = PERS_COM_SUCCESS;
    fd_set    readFds;

    struct timeval     tvTimeout = {PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT_US / ONE_SEC_IN_US, PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT_US % ONE_SEC_IN_US};

    /* build the read fd list */
    FD_ZERO(&readFds);
    FD_SET(mqdes, &readFds);

    /* wait (at most until the timeout occurs) for a message to be available */
    sRetVal = select(mqdes + (sint_t)1, &readFds, NIL, NIL, &tvTimeout);
    if(sRetVal > 0)
    {
        /* message available in the queue */
        if(!FD_ISSET(mqdes, &readFds))
        {
            sRetVal = PERS_COM_FAILURE;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("ERROR - FD_ISSET"));
        }
        else
        {
            /* extract the message from the queue */
            sRetVal = mq_receive(mqdes, (char *)msg_ptr, msg_len, NIL);
            if(sRetVal < 0)
            {
                sRetVal = PERS_COM_FAILURE;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("mq_receive() call failed with error :");
                                                            DLT_STRING(strerror(errno)));
            }
        }
    }
    else
    {
        if(sRetVal < 0)
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,     DLT_STRING("select() call failed with error:");
                                                        DLT_STRING(strerror(errno)));
            sRetVal = PERS_COM_FAILURE;
        }
    }

    return sRetVal;
}


int main(void)
{
    sint_t    sRetVal    = EXIT_SUCCESS;
    sint_t    sTmpRetVal;

    /* Initialize syslog */
    (void)setlogmask (LOG_UPTO (LOG_DEBUG));
    openlog("PAS", LOG_PID, LOG_USER);

    /* Check if the process is already running */
    if(true == persadmin_IsAlreadyRunning())
    {
        syslog(LOG_INFO, "PAS >> PAS process is already running! Only one instance allowed!");

        sRetVal = EXIT_FAILURE;
    }

    if(EXIT_FAILURE != sRetVal)
    {
        /* Daemonize the current process */
        sTmpRetVal = persadmin_DaemonizeProcess((str_t*)PERSISTENCE_ADMIN_PROC_NAME);
        if(DAEMONIZE_FAIL_RET_VAL == sTmpRetVal)
        {
            syslog(LOG_INFO, "PAS >> persadmin_DaemonizeProcess call failed.");

            sRetVal = EXIT_FAILURE;

        }else
        {
            if(DAEMONIZE_SUCCESS_RET_VAL == sTmpRetVal)
            {
                /* Run daemon actions */
                sRetVal = persadmin_RunDaemon();

            }/* exit with success for parent process */
        }
    }

    /* De-initialize syslog */
    closelog();

    return sRetVal;
}
