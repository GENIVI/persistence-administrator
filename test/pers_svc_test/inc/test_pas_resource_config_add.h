/*********************************************************************************************************************** Copyright (C) 2012 Continental Automotive Systems, Inc.** Author: Petrica.Manoila@continental-corporation.com** Implementation of backup process** This Source Code Form is subject to the terms of the Mozilla Public* License, v. 2.0. If a copy of the MPL was not distributed with this* file, You can obtain one at http://mozilla.org/MPL/2.0/.** Date       Author             Reason* 2012.11.27 uidu0250           CSP_WZ#1280:  Initial version***********************************************************************************************************************/#ifndef SSW_TEST_PAS_RESOURCE_CONFIG_ADD_H
#define SSW_TEST_PAS_RESOURCE_CONFIG_ADD_H


#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"
#include "test_PAS.h"

extern expected_key_data_RCT_s expectedKeyData_RCT_resConfAdd_1[17] ;
extern expected_key_data_localDB_s expectedKeyData_localDB_resConfAdd_1[41] ;
expected_file_data_s expectedKeyData_files_resConfAdd_1[16] ;
//extern expected_file_data_s expectedFileData_shared_public_AfterReset[11] ;

extern expected_key_data_RCT_s      expected_RCT_public[13] ;
extern expected_key_data_localDB_s  expectedKeyData_public[29] ;
extern expected_file_data_s         expectedFileData_public[18] ;

extern expected_key_data_RCT_s expected_RCT_group_10[12] ;
extern expected_key_data_localDB_s expectedKeyData_group_10[24] ;
extern expected_file_data_s expectedFileData_group_10[18] ; 

extern expected_key_data_RCT_s expected_RCT_group_20[12] ;
extern expected_key_data_localDB_s expectedKeyData_group_20[16] ;
extern expected_file_data_s expectedFileData_group_20[10] ;

extern expected_key_data_RCT_s expected_RCT_group_30[6] ;
extern expected_key_data_localDB_s expectedKeyData_group_30[8] ;
extern expected_file_data_s expectedFileData_group_30[4] ;

extern expected_key_data_RCT_s expected_RCT_App30_Phase_1[6] ;
extern expected_key_data_localDB_s expectedKeyData_App30_Phase_1[8] ;
extern expected_file_data_s expectedFileData_App30_Phase_1[4] ;

extern expected_key_data_RCT_s      expected_RCT_public[13] ;
extern expected_key_data_localDB_s  expectedKeyData_public_phase2[29] ;
extern expected_file_data_s         expectedFileData_public_phase2[18] ;
    
bool_t Test_ResourceConfigAdd_1(int ceva, void* pAltceva) ;
bool_t Test_ResourceConfigAdd_2(int ceva, void* pAltceva) ;

#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_TEST_PAS_RESOURCE_CONFIG_ADD_H */
