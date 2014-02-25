#ifndef SSW_PERS_ADMIN_DATABASE_HELPER_H
#define SSW_PERS_ADMIN_DATABASE_HELPER_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ana.Chisca@continental-corporation.com
*
* Interface: private - common functionality for database manipulation
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013.01.19 uidn3591  1.0.0.0  CSP_WZ#2060:  Fix delete and copy keys by filter to conform to requirements
* 2013.01.03 uidl9757  1.0.0.0  CSP_WZ#2060:  Use pers_resource_config_table_if.h
* 2012.11.12 uidn3591  1.0.0.0  CSP_WZ#1280:  Created
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"

#include "persistence_admin_service.h"
#include "persComRct.h"

/**
 * @brief get the names of an application's databases based on the databases' type (local/shared)
 * @usage: 
 *
 * @param pchDBRootPath     [in]    path to the folder where to look for the databases
 * @param pchAppName        [in]    application for which the databases are requested
 * @param eStorage          [in]    the type of the requested databases
 * @param pchDBPaths_out    [out]   pointer to a buffer where to store the databases' names [the databases' names are separated by '\0']
 * @param bufSize           [in]    size of buffer
 *
 * @return positive value: size of data passed in pchDBPaths_out ; negative value - error
 * TODO: define error codes
 */
sint_t persadmin_get_all_db_paths_with_names(pconststr_t pchDBRootPath, pconststr_t pchAppName, PersistenceStorage_e eStorage, pstr_t pchDBPaths_out, sint_t bufSize);


/**
 * @brief deletes keys filtered by user name and seat number
 *
 * @param pchDBPath[in] path to where the DB can be located
 * @param user_no  [in] the user ID
 * @param seat_no  [in] the seat number (seat 1 to 4; 0 is for all seats)
 *
 * @return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
sint_t persadmin_delete_keys_by_filter(PersASSelectionType_e type, pstr_t pchDBPath, uint32_t user_no, uint32_t seat_no);


/**
 * @brief copies keys filtered by user name and seat number
 *
 * @param pchdestDBPath  [in] destination DB path
 * @param pchsourceDBPath[in] source DB path
 * @param user_no        [in] the user ID
 * @param seat_no        [in] the seat number (seat 1 to 4; 0 is for all seats)
 *
 * @return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
sint_t persadmin_copy_keys_by_filter(PersASSelectionType_e type, pstr_t pchdestDBPath, pstr_t pchsourceDBPath, uint32_t user_no, uint32_t seat_no);



#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_PERS_ADMIN_DATABASE_HELPER_H */
