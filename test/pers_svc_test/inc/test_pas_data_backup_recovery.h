/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2012.11.27 uidu0250           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#ifndef SSW_TEST_PAS_DATA_BACKUP_RECOVERY_H
#define SSW_TEST_PAS_DATA_BACKUP_RECOVERY_H


#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"
#include "test_PAS.h"

#define	BACKUP_FOLDER			"/tmp/backups"
#define BACKUP_FOLDER_			BACKUP_FOLDER "/"
#define BACKUP_CONTENT_FOLDER	BACKUP_FOLDER PERS_ORG_ROOT_PATH

#define PATH_ABS_MAX_SIZE  		( 512)

#define BACKUP_FORMAT           (".arch.tar.gz")

#define USER_DONT_CARE                  0xFF
#define SEAT_DONT_CARE                  0xFF

//===================================================================================================================
// BACKUP CONTENT
//===================================================================================================================
bool_t ResetBackupContent(PersASSelectionType_e type, char* applicationID);

//===================================================================================================================
// RECOVER DATA - App1
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_App1[24];
extern expected_file_data_s expected_file_data_after_restore_App1[16];

bool_t Test_Recover_App1(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - User 1
//===================================================================================================================
extern expected_key_data_localDB_s expected_App1_key_data_after_restore_User1[24];
extern expected_file_data_s expected_App1_file_data_after_restore_User1[16];

bool_t Test_Recover_User1(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - All
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_All[23 + 24 + 48 + 44];
extern expected_file_data_s expected_file_data_after_restore_All[16 + 16 + 16];

bool_t Test_Recover_All(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - Users
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_Users[23 + 24 + 24 + 24 + 22];
extern expected_file_data_s expected_file_data_after_restore_Users[16 + 16 + 16];

bool_t Test_Recover_Users(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - All InitialContent
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_All_InitialContent[16 + 16 + 16 + 32 + 32];
extern expected_file_data_s expected_file_data_after_restore_All_InitialContent[10 + 10 + 10 + 10 + 10];

bool_t Test_Recover_All_InitialContent(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - App1 InitialContent
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_App1_InitialContent[16 + 16 + 16 + 32 + 32];
extern expected_file_data_s expected_file_data_after_restore_App1_InitialContent[10 + 10 + 10 + 10 + 10];

bool_t Test_Recover_App1_InitialContent_From_All(sint_t type, void* pv);
bool_t Test_Recover_App1_InitialContent_From_App1(sint_t type, void* pv);


//===================================================================================================================
// RECOVER DATA - User1 InitialContent
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_User1_InitialContent[16 + 16 + 16 + 32 + 32];
extern expected_file_data_s expected_file_data_after_restore_User1_InitialContent[10 + 10 + 10 + 10 + 10];

bool_t Test_Recover_User1_InitialContent_From_All(sint_t type, void* pv);
bool_t Test_Recover_User1_InitialContent_From_User1(sint_t type, void* pv);

//===================================================================================================================
// RECOVER DATA - User2 Seat1 InitialContent
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_User2_Seat1_InitialContent[16 + 16 + 16 + 32 + 32];
extern expected_file_data_s expected_file_data_after_restore_User2_Seat1_InitialContent[10 + 10 + 10 + 10 + 10];

bool_t Test_Recover_User2_Seat1_InitialContent_From_All(sint_t type, void* pv);
bool_t Test_Recover_User2_Seat1_InitialContent_From_User2_Seat1(sint_t type, void* pv);


#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_TEST_PAS_DATA_BACKUP_RECOVERY_H */
