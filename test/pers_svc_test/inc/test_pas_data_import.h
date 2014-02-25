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

#include "persComTypes.h"
#include "test_PAS.h"

bool_t ResetImportData(PersASSelectionType_e type);
bool_t Test_import_all_app(sint_t type, void* pv);
bool_t Test_import_all_all(sint_t type, void* pv);
bool_t Test_import_all_user(sint_t type, void* pv);

extern expected_key_data_localDB_s expected_key_data_after_import_app_all[22];
extern expected_file_data_s expected_file_data_after_import_app_all[12];

extern expected_key_data_localDB_s expected_key_data_after_import_all_all[57];
extern expected_file_data_s expected_file_data_after_import_all_all[30];

extern expected_key_data_localDB_s expected_key_data_after_import_all_user[57];
extern expected_file_data_s expected_file_data_after_import_all_user[30];
