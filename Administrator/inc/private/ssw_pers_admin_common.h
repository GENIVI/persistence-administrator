#ifndef SSW_PERS_ADMIN_COMMON_H
#define SSW_PERS_ADMIN_COMMON_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Interface: private - other common functionality
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013.03.21 uidu0250  1.0.0.0  CSP_WZ#2250:  Provide compress/uncompress functionality
* 2013.02.07 uidu0250  1.0.0.0  CSP_WZ#2220:  Removed persadmin_get_hash_for_file declaration
* 2013.01.22 uidn3591  1.0.0.0  CSP_WZ#2250:  Implemented wrappers over libarchive to compress/uncompress files into/from an archive
* 2013.01.04 uidu0250  1.0.0.0  CSP_WZ#2060:  Changed CRC32 to CRC16 provided by HelpLibs
* 2013.01.03 uidl9757  1.0.0.0  CSP_WZ#2060:  Common interface to be used by both PCL and PAS
* 2012.12.11 uidu0250  1.0.0.0	CSP_WZ#1280:  Added declaration for persadmin_get_hash_for_file
* 2012.11.16 uidn3565  1.0.0.0  CSP_WZ#1280:  Added declarations for persadmin_list_application_folders & persadmin_list_application_folders_get_size
* 2012.11.16 uidl9757  1.0.0.0  CSP_WZ#1280:  Created 
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"

#define BACKUP_LOCATION                 ("/tmp/backups/")
#define BACKUP_FORMAT                   (".arch.tar.gz")

extern const char StringTerminator;
extern const char PathSeparator;

/*
* export the application's information about links (to groups and public data) 
* returns the number of exported links, or a negative value in case of error
*/
sint_t persadmin_export_links(pstr_t absPathApplicationFolder, pstr_t absPathExportFolder) ;

/*
* install links (to groups and public data) into the application folder(indicated by absPathApplicationFolder)
* based on information available inside the import folder (indicated by absPathImportFolder)
* returns the number of installed links, or a negative value in case of error
*/
sint_t persadmin_import_links(pstr_t absPathImportFolder, pstr_t absPathApplicationFolder) ;

/**
 * @brief List all application folders under the given root data path.
 *
 * @param rootPath            [in]  Absolute path to the folder storing applications' data; the final path separator is not required - e.g. "/Data/mnt-c"
 * @param list                [out] Buffer receiving the list of application folders; entries are relative to rootPath, are separated by '\0' and are ended with a double '\0'
 * @param listSize            [in]  Size in bytes of of list; should be at least equal to the positive return value of persadmin_list_application_folders_get_size()
 * @return Positive value - the size of data written in list, negative - error.
 * TODO: define error codes
 */
sint_t persadmin_list_application_folders( pconststr_t rootPath, pstr_t list, sint_t listSize );

/**
 * @brief Compute the size of the buffer needed to accommodate the list of application folders
 *
 * @param rootPath            [in]  Absolute path to the folder storing applications' data; the final path separator is not required - e.g. "/Data/mnt-c"
 * @return Positive value - the size in bytes of application data folders listing, negative - error.
 * TODO: define error codes
 */
sint_t persadmin_list_application_folders_get_size( pconststr_t rootPath );

/**
 * @brief Saves files together into a single archive.
 *
 * @return 0 for success, negative value otherwise.
 */
sint_t persadmin_compress(pstr_t compressTo, pstr_t compressFrom);

/**
 * @brief Restore files from an archive.
 *
 * @return 0 for success, negative value otherwise.
 */
sint_t persadmin_uncompress(pstr_t extractFrom, pstr_t extractTo);


#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_PERS_ADMIN_COMMON_H */
