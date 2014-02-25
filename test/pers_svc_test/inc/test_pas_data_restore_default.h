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

#ifndef SSW_TEST_PAS_DATA_RESTORE_DEFAULT_H
#define SSW_TEST_PAS_DATA_RESTORE_DEFAULT_H


#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"
#include "test_PAS.h"

typedef struct
{
    pstr_t              filename;       	/* contains the full path (with node, user,... prefix) */
    bool_t              bIsFolder;   		/* if true, the filename is a folder */
    bool_t              bExpectedToExist;   /* if true, the key is expected to be found in the indicated DB */
}expected_default_file_data_s;

//===================================================================================================================
// RESTORE DEFAULT - App1
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_default_App1[16 + 16 + 16 + 16 + 16];
extern expected_file_data_s expected_file_data_after_restore_default_App1[10 + 10 + 10 + 10 + 10];

bool_t Test_Restore_Factory_Default_App1(sint_t type, void* pv);
bool_t Test_Restore_Configurable_Default_App1(sint_t type, void* pv);


//===================================================================================================================
// RESTORE DEFAULT - User 1
//===================================================================================================================
extern expected_key_data_localDB_s expected_key_data_after_restore_default_User1[16 + 16 + 16 + 16 + 16];
extern expected_file_data_s expected_file_data_after_restore_default_User1[10 + 10 + 10 + 10 + 10];

bool_t Test_Restore_Configurable_Default_User1(sint_t type, void* pv);


//===================================================================================================================
// RESTORE DEFAULT - All
//===================================================================================================================
extern expected_key_data_localDB_s expected_key_data_after_restore_default_All[16 + 16 + 16 + 16 + 16];
extern expected_file_data_s expected_file_data_after_restore_default_All[10 + 10 + 10 + 10 + 10];

bool_t Test_Restore_Factory_Default_All(sint_t type, void* pv);
bool_t Test_Restore_Configurable_Default_All(sint_t type, void* pv);


//===================================================================================================================
// RESTORE DEFAULT - User2 Seat1
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_default_User2Seat1[16 + 16 + 16 + 16 + 16];
extern expected_file_data_s expected_file_data_after_restore_default_User2Seat1[10 + 10 + 10 + 10 + 10];

bool_t Test_Restore_Configurable_Default_User2Seat1(sint_t type, void* pv);

//===================================================================================================================
// RESTORE DEFAULT - User2 App1
//===================================================================================================================

extern expected_key_data_localDB_s expected_key_data_after_restore_default_User2App1[16 + 16 + 16 + 16 + 16];
extern expected_file_data_s expected_file_data_after_restore_default_User2App1[10 + 10 + 10 + 10 + 10];

bool_t Test_Restore_Configurable_Default_User2App1(sint_t type, void* pv);

#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_TEST_PAS_DATA_RESTORE_DEFAULT_H*/
