#ifndef PERSADMIN_ACCESS_LIB_H
#define PERSADMIN_ACCESS_LIB_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Interface: private - persistence admin service access lib
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013.04.15 uidu0250  2.0.0.0	CSP_WZ#3424:  Add IF extension for "restore to default"
* 2012.11.16 uidl9757  1.0.0.0  CSP_WZ#1280:  persadmin_response_msg_s: rename bResult to result and change type to long
* 2012.11.12 uidl9757  1.0.0.0  CSP_WZ#1280:  Member "type" in persadmin_request_msg_s must be PersASSelectionType_e
* 2012.11.13 uidl9757  1.0.0.0  CSP_WZ#1280:  Created 
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include "persComTypes.h"
#include "persistence_admin_service.h"
#include "persComDataOrg.h"

#define PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH (PERS_ORG_MAX_LENGTH_PATH_FILENAME + 1)//256

/* named synchronization objects */
#define PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST "/PersadminAccessLibRequest"
#define PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE "/PersadminAccessLibResponse"
#define PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE "/PersadminAccessLibSync"

/* supported opperations */
typedef enum
{
    PAS_Req_DataBackupCreate = 0,
    PAS_Req_DataBackupRecovery,
    PAS_Req_DataFolderExport,
    PAS_Req_DataFolderImport,
    PAS_Req_ResourceConfigAdd,
    PAS_Req_ResourceConfigChangeProperties,
    PAS_Req_UserDataCopy,
    PAS_Req_UserDataDelete,
    PAS_Req_DataRestoreToDefault,
    /* add new definitions here*/
    PAS_Req_LastEntry
}persadmin_request_e ;

/* used to pass client's request to the processing thread */
typedef struct
{
    persadmin_request_e eRequest ;
    PersASSelectionType_e type ;
    PersASDefaultSource_e defaultSource ;
    char path1[PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH] ; /* for backup_name, dst_folder, src_folder, resource_file */
    char path2[PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH] ; /* applicationID */
    unsigned int user_no ;
    unsigned int seat_no ;
    unsigned int src_user_no ;
    unsigned int src_seat_no ;
    unsigned int dest_user_no ;
    unsigned int dest_seat_no ;
}persadmin_request_msg_s ;

/* used to pass response from processing thread to the client who made the request */
typedef struct
{
    long result ;
}persadmin_response_msg_s ;

#ifdef __cplusplus
}
#endif

#endif /* PERSADMIN_ACCESS_LIB_H */
