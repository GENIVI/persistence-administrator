/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.04.15 uidu0250           CSP_WZ#3424:  Add IF extension for "restore to default"
* 2012.11.29 uidv2833           CSP_WZ#1280:  Adapted implementation for the new test framework
* 2012.11.28 uidl9757           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#ifndef SSW_TEST_PAS_H
#define SSW_TEST_PAS_H


#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"
#include "persComRct.h"
#include "persComDataOrg.h"

/* needed because of change in PCL (should be removed later) */
#define TST_DEFAULT_CONTEXT {0xFF, 0xFF, 0xFF}

#define MAX_PATH_SIZE                   ( 256 )
#define MAX_APPLICATION_NAME_SIZE       (  64 )

/* backup */
#define NO_BACKUP                       ""
#define BACKUP_NAME                     "/tmp/backup"

#define	BACKUP_FOLDER					"/tmp/backups"
#define BACKUP_FOLDER_					BACKUP_FOLDER "/"
#define BACKUP_CONTENT_FOLDER			BACKUP_FOLDER PERS_ORG_ROOT_PATH
#define BACKUP_FORMAT      				(".arch.tar.gz")

/* application */
#define NO_APPLICATION                  ""
#define APPLICATION_NAME                "App1"

/* user */
//#define USER_DONT_CARE                  0xFF
//#define SEAT_DONT_CARE                  0xFF

typedef struct
{
    pstr_t resourceID ;
    bool_t bIsKey ; 
    PersistenceConfigurationKey_s sRctEntry ;
}entryRctInit_s ;

typedef struct
{
    str_t LDBID;
    pstr_t resourceID ;
    PersistencePolicy_e policy ;
    str_t userID ;
    str_t seatID ;
    pstr_t data ;
}entryDataInit_s ;

typedef struct
{
    pstr_t  pResourceID ;
    pstr_t  data ;
}defaultDataInit_s ;

typedef enum
{
    dbType_local = 0,
    dbType_RCT
}dbType_e ;

typedef struct
{
    pstr_t                  installFolderPath ;
    pstr_t                  RCT_pathname ;
    dbType_e                RctDBtype ;
    pstr_t                  wtDBpathname ;
    dbType_e                wtDBtype ;
    pstr_t                  wcDBpathname ;
    dbType_e                wcDBtype ;
    entryRctInit_s*         RctInitTab ;
    sint_t                  noEntriesRctInitTab ;
    entryDataInit_s*        dataKeysInitTab ;
    sint_t                  noEntriesDataKeysInitTab ;
    entryDataInit_s*        dataFilesInitTab ;
    sint_t                  noEntriesDataFilesInitTab ;

    pstr_t                  factoryDefaultDBpathname ;
    defaultDataInit_s*      factoryDefaultInitTab ;
    sint_t                  noEntriesFactoryDefaultInitTab ;
    pstr_t                  configurableDefaultDBpathname ;
    defaultDataInit_s*      configurableDefaultInitTab ;
    sint_t                  noEntriesConfigurableDefaultInitTab ;
}dataInit_s ;

/**********************************************************************************************************************************************
 ***************************************** Structures used for test cases ********************************************************************
 *********************************************************************************************************************************************/
typedef struct
{
    pstr_t              key ;               /* contains the complete name (with node, user,... prefix) */
    pstr_t              dbPath ;            /* abs path to DB */  
    bool_t              bExpectedToExist;   /* if true, the key is expected to be found in the indicated DB */
    pstr_t              expectedData ;
    sint_t              expectedDataSize ;
}expected_key_data_localDB_s ;

typedef struct
{
    pstr_t                          key ;               /* contains the complete name (with node, user,... prefix) */
    pstr_t                          dbPath ;            /* abs path to DB */  
    bool_t                          bExpectedToExist;   /* if true, the key is expected to be found in the indicated DB */
    PersistenceConfigurationKey_s   sExpectedConfig ;
}expected_key_data_RCT_s ;

typedef struct
{
    pstr_t              filename ;          /* contains the complete name (with node, user,... prefix) */
    bool_t              bExpectedToExist;   /* if true, the key is expected to be found in the indicated DB */
    pstr_t              expectedData ;
    sint_t              expectedDataSize ;
}expected_file_data_s ;

/* test case prototype */
typedef bool_t (*pfTestCase) (sint_t, void*) ;

typedef struct
{
    pfTestCase                      TestCaseFunction ;
    sint_t                          iParam ;
    void*                           pvoidParam ;
    pstr_t                          testCaseDescription ;
    expected_key_data_RCT_s*        pExpectedKeyDataRCT ;
    sint_t                          noItemsInExpectedKeyDataRCT ;
    expected_key_data_localDB_s*    pExpectedKeyDataLocalDB ;
    sint_t                          noItemsInExpectedKeyDataLocalDB ;
    expected_file_data_s*           pExpectedFileData ;
    sint_t                          noItemsInExpectedFileData ;
    bool_t                          bSkipDataReset ;
}testcase_s ;

bool_t InitDataFolder(dataInit_s* psDataInit) ;
sint_t DeleteFolderContent(pstr_t folderPath);
sint_t DeleteFolder(pstr_t folderPath);
sint_t CheckIfFileExists(pstr_t pathname, bool_t bIsFolder);
bool_t ExecuteTestCases(sint_t		*pNoOfTestCases,
						sint_t		*pNoOfTestCasesSuccessful,
						sint_t		*pNoOfTestCasesFailed );

#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_TEST_PAS_H */

