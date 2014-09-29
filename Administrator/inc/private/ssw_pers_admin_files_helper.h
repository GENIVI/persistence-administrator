#ifndef SSW_PERS_ADMIN_FILES_HELPER_H
#define SSW_PERS_ADMIN_FILES_HELPER_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*         Petrica.Manoila@continental-corporation.com
*
* Interface: private - common functionality for files/folder manipulation
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2014.09.29 uidu0250  1.0.1.0  CSP_WZ#8463:  Added persadmin_get_file_size
* 2013.02.07 uidu0250  1.0.0.0  CSP_WZ#1280:  Added persadmin_check_for_same_file_content to check for identical file content
* 2012.11.16 uidv2833  1.0.0.0  CSP_WZ#1280:  persadmin_delete_folder and persadmin_delete_file return the number of bytes deleted
* 2012.11.15 uidl9757  1.0.0.0  CSP_WZ#1280:  Some extensions:
                                - persadmin_copy_folder and persadmin_copy_file return the number of bytes copied
                                - added persadmin_check_if_file_exist and persadmin_check_if_file_exist
* 2012.11.12 uidl9757  1.0.0.0  CSP_WZ#1280:  Created
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComTypes.h"
#include "persComDataOrg.h"

#define PERSADMIN_MAX_PATH_LENGHT (PERS_ORG_MAX_LENGTH_PATH_FILENAME + 1) //256
#define PERSADMIN_MAX_NESTING_DEPTH 12

typedef enum
{
    PersadminFilterAll = 0,                 /* all folders and files are selectes */
    PersadminFilterFilesAll,                /* only files are selected (folders are skipped )*/
    PersadminFilterFilesRegular,            /* only regular files are selected (folders and link files are skipped )*/
    PersadminFilterFilesLink,               /* only link files are selected (folders and regular files are skipped )*/
    PersadminFilterFoldersAndRegularFiles,  /* only folders and regular files are selected */
    PersadminFilterFoldersAndLinkFiles,     /* only folders and link files are selected */
    PersadminFilterFolders,                 /* only folders are selected */
}PersadminFilterMode_e ;

/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns in fileName_out the part of filePath after the last '/' (or the complete filePath if there is no '/') 
*/
sint_t persadmin_get_filename(pstr_t filePath, pstr_t fileName_out, sint_t size);

/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns the part of filePath after the last '.' (but must be after the last '/') 
*/
sint_t persadmin_get_file_extension(pstr_t filePath, pstr_t fileExtension_out, sint_t size);

/*
* get the size of the file
* returns size of the file, or a negative value in case of error
*/
sint_t persadmin_get_file_size(pconststr_t pathname) ;

/*
* filePath can be absolute or relative
* it is not checked if the file indicated by filePath actually exist
* returns the part of filePath before (and including) the last '/'
*/
sint_t persadmin_get_folder_path(pstr_t filePath, pstr_t folderPath_out, sint_t size);

/*
* check if file/folder indicated by pathname exist
* if (bIsFolder==true) it is checked the existence of the folder, otherway the existence of the file
* returns 0 if the file/folder exist, a negative value other way
*/
sint_t persadmin_check_if_file_exists(pstr_t pathname, bool_t bIsFolder) ;

/*
* create a link in the path indicated by pathLink which points to path indeicated by pathTarget
* there is no check if pathTarget actually exist (or is valid)
* the intermediate folders in pathLink are created if they do not exist
* returns 0 if the file/folder exist, a negative value other way
*/
sint_t persadmin_create_symbolic_link(pstr_t pathLink, pstr_t pathTarget) ;

/**
 * \brief copy the folder indicated by absPathSource into the folder indicated by absPathDestination
 * \usage : persadmin_copy_folder("/Data/mnt-c/App-1/", "/var/Pers/bkup/App_1_bkup/")
 *        : copy the content of /Data/mnt-c/App-1/ into /var/Pers/bkup/App_1_bkup/
 *
 * \param absPathSource         [in] absolute path of source folder
 * \param absPathDestination    [in] absolute path of destination folder
 * \param eFilter               [in]  specify what to list (see PersadminFilterMode_e)
 * \param bRecursive            [in]  if true, a recursive listing is performed
 * \return number of bytes copied, negative value for error
 * TODO: define error codes
 */
sint_t persadmin_copy_folder(pstr_t absPathSource, pstr_t absPathDestination, PersadminFilterMode_e eFilter, bool_t bRecursive);


/**
 * \brief create a folder as indicated by absPath
 * \usage : persadmin_create_folder("/var/Pers/bkup/App_1_bkup/")
 * \note : all the missing folders in the path are created (i.e. /var/Pers/, /var/Pers/bkup/)
 *
 * \param absPath               [in] absolute path to folder to be created
 * \return 0 for success, negative value otherwise;
 * TODO: define error codes
 */
sint_t persadmin_create_folder(pstr_t absPath);

/**
 * \brief delete a folder
 * \usage : persadmin_delete_folder("/var/Pers/bkup/App_1_bkup/")
 *
 * \param absPath               [in] absolute path to folder to be deleted
 * \return positive value: number of bytes deleted; negative value: error
 * TODO: define error codes
 */
sint_t persadmin_delete_folder(pstr_t absPath);

/**
 * \brief copy a file (with posibility to rename the copy)
 * \usage : persadmin_copy_file("/Data/mnt-c/shared/public/cached.itz", "/var/Pers/bkup/CACHED.itz")
 *        : content of /Data/mnt-c/shared/public/cached.itz is copied into /var/Pers/bkup/CACHED.itz
 *
 * \param absPathSource         [in] absolute path to the source file
 * \param absPathDestination    [in] absolute path to the destination file
 * \return number of bytes copied, negative value for error
 * TODO: define error codes
 */
sint_t persadmin_copy_file(pstr_t absPathSource, pstr_t absPathDestination);

/**
 * \brief delete a file
 * \usage : persadmin_delete_file("/Data/mnt-c/shared/public/cached.itz")
 *
 * \param absPath               [in] absolute path to the file to be deleted
 * \return positive value: number of bytes deleted; negative value: error
 * TODO: define error codes
 */
sint_t persadmin_delete_file(pstr_t absPath);

/**
 * \brief list the content of a folder
 *
 * \param folderPath            [in]  absolute path to the folder to be listed
 * \param buffer_out            [out] where the listing is save ; entries are relative to folderPath and separated by '\0'
 * \param bufSize               [in]  size of buffer_out
 * \param eFilter               [in]  specify what to list (see PersadminFilterMode_e)
 * \param bRecursive            [in]  if true, a recursive listing is performed
 * \return positive value - the size of data passed in buffer_out, negative - error;
 * TODO: define error codes
 */
sint_t persadmin_list_folder(pconststr_t folderPath, pstr_t buffer_out, sint_t bufSize, PersadminFilterMode_e eFilter, bool_t bRecursive) ;

/**
 * \brief find the buffer size needed to accommodate the output of a list folder request
 *
 * \param folderPath        [in]  absolute path to the folder to be listed
 * \param eListingMode      [in]  specify what to list (see PersadminFilterMode_e)
 * \param bRecursive        [in]  if true, a recursive listing is performed
 * \return positive value - the size needed, negative - error;
 * TODO: define error codes
 */
sint_t persadmin_list_folder_get_size(pconststr_t folderPath, PersadminFilterMode_e eListingMode, bool_t bRecursive) ;

/**
 * \brief Computes the size in bytes of all files located under the specified folder path. The file search is done recursively.
 *
 * \param folderPath        [in]  absolute path to the folder to be queried
 * \return positive value - the size in bytes of all files, negative - error;
 * TODO: define error codes
 */
sint_t persadmin_get_folder_size( pconststr_t folderPath );

/**
 * \brief Checks if the files specified have identical contents (without the usage of checksums)
 *
 * \param file1Path        [in]  absolute path to one of the files to be checked
 * \param file2Path		   [in]  absolute path to the other file to be checked
 * \return true - if the files have identical contents, false - otherwise;
 */
bool_t persadmin_check_for_same_file_content(pstr_t file1Path, pstr_t file2Path);

#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /*SSW_PERS_ADMIN_FILES_HELPER_H */