/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of funtions declaredin ssw_pers_admin_common.h
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.03.20 uidu0250           CSP_WZ#2250:  Provide compress/uncompress functionality
* 2013.02.07 uidu0250           CSP_WZ#2220:  Removed Helplibs dependency (CRC16 checksum calculation)
* 2013.01.22 uidn3591           CSP_WZ#2060:  Implemented wrappers over libarchive to compress/uncompress files into/from an archive
* 2013.01.04 uidu0250           CSP_WZ#2060:  Switched get_hash_for_file implementation from using CRC32 to using CRC16 provided by HelpLibs
* 2012.11.15 uidl9757           CSP_WZ#1280:  Use protected interface pers_data_organization_if.h
* 2012.11.16 uidn3565           CSP_WZ#1280:  Added implementation for
                                              - persadmin_list_application_folders
                                              - persadmin_list_application_folders_get_size
* 2012.11.15 uidl9757           CSP_WZ#1280:  Created
*
**********************************************************************************************************************/
/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* compress/uncompress */
#include <archive.h>
#include <archive_entry.h>

#include "malloc.h"

#include "ssw_pers_admin_common.h"
#include "ssw_pers_admin_files_helper.h"
#include "persComDataOrg.h"
#include "persistence_admin_service.h"

/* ---------------------- local definitions -------------------------------- */
#define READ_BUFFER_LENGTH (16384)
#define COPY_BUFFER_LENGTH (128)

#define PATH_ABS_MAX_SIZE  ( 512)

#define PERSADMIN_POLICY_MARKER_CACHED PERS_ORG_CACHE_FOLDER_NAME
#define PERSADMIN_POLICY_MARKER_WT PERS_ORG_WT_FOLDER_NAME
#define CRC16_FILE_CHUNK_SIZE         (10 * 1024)
/* TO DO : This should be published in a protected interface */
#define PERSADMIN_LINKS_INFO_FILENAME "linksInfo.lnk"
#define GetApplicationRootPath( RootPath, FullPath )                                            \
  int pathLength = strlen( RootPath );                                                          \
  char FullPath[ strlen( gLocalCachePath ) + pathLength + sizeof( StringTerminator ) ];         \
  snprintf( FullPath, sizeof( FullPath ), "%s", RootPath );                                     \
  snprintf( FullPath + pathLength, sizeof( FullPath ) - pathLength, gLocalCachePath, "", "" )
/* ---------------------- local types -------------------------------------- */

/* ---------------------- local functions ---------------------------------- */
static sint_t persadmin_common_extract_group_id(pstr_t linkName) ;
static void   persadmin_common_remove_endofline(pstr_t line) ;
static bool_t persadmin_is_shared_folder( pconststr_t name, int length );
static sint_t persadmin_copy_data(struct archive *ar, struct archive *aw);

/* ---------------------- local variables ---------------------------------- */
//moved to "ssw_pers_admin_common.h"
//extern const char StringTerminator;

/*
* export the application's information about links (to groups and public data)
* returns the number of exported links, or a negative value in case of error
*/
sint_t persadmin_export_links(pstr_t absPathApplicationFolder, pstr_t absPathExportFolder)
{
    pstr_t FuncName = "persadmin_export_links:" ;
    bool_t bEverythingOK = true ;
    pstr_t buffer = NIL ;
    bool_t bNothingToExport = false ;
    sint_t exportedLinks = 0 ;
    sint_t neededBufferSize = 0 ;
    FILE * fileExport = NIL ;
    if( (NIL == absPathApplicationFolder) || (NIL == absPathExportFolder))
    {
        bEverythingOK = false ;
        printf("\n%s NIL param \n", FuncName) ;
    }
    else
    {
        if(0 != persadmin_check_if_file_exists(absPathApplicationFolder, true))
        {
            bEverythingOK = false ;
            printf("\n%s folder does not exist (%s) \n", FuncName, absPathApplicationFolder) ;
        }
        else
        {
            if(0 != persadmin_check_if_file_exists(absPathExportFolder, true))
            {
                bEverythingOK = false ;
                printf("\n%s folder does not exist (%s) \n", FuncName, absPathExportFolder) ;
            }
        }
    }
    if(bEverythingOK)
    {
        neededBufferSize = persadmin_list_folder_get_size(absPathApplicationFolder, PersadminFilterFilesLink, false) ;
        if(neededBufferSize > 0)
        {
            buffer = (pstr_t)malloc(neededBufferSize*sizeof(str_t)) ;
            if(NIL == buffer)
            {
                bEverythingOK = false ;
            }
            else
            {
                sint_t listingSize = persadmin_list_folder(absPathApplicationFolder, buffer, neededBufferSize, PersadminFilterFilesLink, false) ;
                if(neededBufferSize != listingSize)
                {
                    bEverythingOK = false ;
                    printf("\n%s persadmin_list_folder(%s) returned %d (expected %d) \n", FuncName, absPathApplicationFolder, listingSize, neededBufferSize) ;
                }
            }
        }
        else
        {
            if(0 == neededBufferSize)
            {
                /* no links to export */
                bNothingToExport = true ;
            }
            else
            {
                bEverythingOK = false ;
                printf("\n%s persadmin_list_folder_get_size(<%s>, FilterFilesLink, false) failed\n", FuncName, absPathApplicationFolder) ;
            }
        }
    }
    if(bEverythingOK && (! bNothingToExport))
    {
        /* create the export file */
        str_t completePath[PERSADMIN_MAX_PATH_LENGHT] ;
        sint_t lenPathExport = strlen(absPathExportFolder) ;
        sint_t lenLinkFilename = strlen(PERSADMIN_LINKS_INFO_FILENAME) ;
        if( (lenPathExport + 1 + lenLinkFilename) < PERSADMIN_MAX_PATH_LENGHT )
        {
            strncpy(completePath, absPathExportFolder, sizeof(completePath)) ;
            if('/' != completePath[lenPathExport -1])
            {
                strncat(completePath, "/", sizeof(completePath)) ;
            }
            strncat(completePath, PERSADMIN_LINKS_INFO_FILENAME, sizeof(completePath)) ;
            fileExport = fopen(completePath, "w") ;
            if(NIL == fileExport)
            {
                bEverythingOK = false ;
                printf("\n%s fopen(<%s>, w) errno = %s\n", FuncName, completePath, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            printf("\n%s path too long (%s/%s)\n", FuncName, absPathExportFolder, PERSADMIN_LINKS_INFO_FILENAME) ;
        }
    }
    if(bEverythingOK && (! bNothingToExport))
    {
        /* for each link file in buffer, create an entry (line) in the export file
        *  for links to group - only the number of the group (i.e. a hex value in 0x00 - 0xFF domain) is stored
        *  for links to public data - the word "public" is stored
        **/
        sint_t posInBuffer = 0 ;
        while((posInBuffer < neededBufferSize) && bEverythingOK)
        {
            sint_t lenCurrentLink = strlen(buffer+posInBuffer) ;
            if(0 == strcmp(buffer+posInBuffer, PERS_ORG_SHARED_PUBLIC_SYMLINK_NAME))
            {
                /* it is the link to the public data */
                if(0 > fputs("public\n", fileExport))
                {
                    bEverythingOK = false ;
                    printf("\n%s fputs(public) errno=%s\n", FuncName, strerror(errno)) ;
                }
                else
                {
                    exportedLinks++ ;
                }
            }
            else
            {
                sint_t groupID = persadmin_common_extract_group_id(buffer+posInBuffer) ;
                if(groupID >= 0)
                {
                    str_t line[PERSADMIN_MAX_PATH_LENGHT] ;
                    printf("\n%s groupID(%s)=0x%02X\n", FuncName, buffer+posInBuffer, groupID) ;
                    snprintf(line, sizeof(line), "%02X\n", groupID) ;
                    if(0 > fputs(line, fileExport))
                    {
                        bEverythingOK = false ;
                        printf("\n%s fputs(%02X) errno=%s\n", FuncName, groupID, strerror(errno)) ;
                    }
                    else
                    {
                        exportedLinks++ ;
                    }
                }
                else
                {
                    printf("\n%s unable to extract group ID from (%s). Ignore it\n", FuncName, buffer+posInBuffer) ;
                }
            }
            posInBuffer += (lenCurrentLink + 1) ;
        }
    }
    if(NIL != buffer)
    {
        free(buffer) ;
    }
    if(NIL != fileExport)
    {
        fclose(fileExport) ;
    }
    return bEverythingOK ? exportedLinks : PAS_FAILURE;
}
/*
* install links (to groups and public data) into the application folder(indicated by absPathApplicationFolder)
* based on information available inside the import folder (indicated by absPathImportFolder)
* returns the number of installed links, or a negative value in case of error
*/
sint_t persadmin_import_links(pstr_t absPathImportFolder, pstr_t absPathApplicationFolder)
{
    pstr_t FuncName = "persadmin_import_links:" ;
    bool_t bEverythingOK = true ;
    bool_t bNothingToImport = false ;
    sint_t importedLinks = 0 ;
    FILE * fileImport = NIL ;
    bool_t bImportInCachedPath = true ;
    if( (NIL == absPathImportFolder) || (NIL == absPathApplicationFolder))
    {
        bEverythingOK = false ;
        printf("\n%s NIL param \n", FuncName) ;
    }
    else
    {
        if(0 != persadmin_check_if_file_exists(absPathImportFolder, true))
        {
            bEverythingOK = false ;
            printf("\n%s folder does not exist (%s) \n", FuncName, absPathImportFolder) ;
        }
        else
        {
            if(0 != persadmin_check_if_file_exists(absPathApplicationFolder, true))
            {
                bEverythingOK = false ;
                printf("\n%s folder does not exist (%s) \n", FuncName, absPathApplicationFolder) ;
            }
        }
    }
    if(bEverythingOK)
    {
        /* check if the absPathApplicationFolder is a cached or write-through path */
        if(NIL != strstr(absPathApplicationFolder, PERSADMIN_POLICY_MARKER_CACHED))
        {
            /* cached path */
            bImportInCachedPath = true ;
        }
        else
        {
            /* not a cached path, so it ahould be a write-through path */
            if(NIL != strstr(absPathApplicationFolder, PERSADMIN_POLICY_MARKER_WT))
            {
                /* write-through */
                bImportInCachedPath = false ;
            }
            else
            {
                bEverythingOK = false ;
                printf("\n%s no cached or wt path (%s) \n", FuncName, absPathApplicationFolder) ;
            }
        }
    }
    if(bEverythingOK)
    {
        /* open the source file */
        str_t completePath[PERSADMIN_MAX_PATH_LENGHT] ;
        sint_t lenPathImport = strlen(absPathImportFolder) ;
        sint_t lenLinkFilename = strlen(PERSADMIN_LINKS_INFO_FILENAME) ;
        if( (lenPathImport + 1 + lenLinkFilename) < PERSADMIN_MAX_PATH_LENGHT )
        {
            strncpy(completePath, absPathImportFolder, sizeof(completePath)) ;
            if('/' != completePath[lenPathImport -1])
            {
                strncat(completePath, "/", sizeof(completePath)) ;
            }
            strncat(completePath, PERSADMIN_LINKS_INFO_FILENAME, sizeof(completePath)) ;
            fileImport = fopen(completePath, "r") ;
            if(NIL == fileImport)
            {
                /* nothing to import */
                bNothingToImport = true ;
                printf("\n%s fopen(<%s>, r) errno = %s\n", FuncName, completePath, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            printf("\n%s path too long (%s/%s)\n", FuncName, absPathImportFolder, PERSADMIN_LINKS_INFO_FILENAME) ;
        }
    }
    if(bEverythingOK && (! bNothingToImport))
    {
        bool_t bEndOfFileReached = false ;
        str_t appFolderPath[PERSADMIN_MAX_PATH_LENGHT] ;
        sint_t lenAppFolder = strlen(absPathApplicationFolder) ;
        if(lenAppFolder + 1 < PERSADMIN_MAX_PATH_LENGHT)
        {
            strncpy(appFolderPath, absPathApplicationFolder, sizeof(appFolderPath)) ;
            if('/' != appFolderPath[lenAppFolder-1])
            {
                strncat(appFolderPath, "/", sizeof(appFolderPath)) ;
                lenAppFolder += 1 ;
            }
        }
        else
        {
            bEverythingOK = false ;
            printf("\n%s path too long (%s)\n", FuncName, absPathImportFolder) ;
        }

        while((! bEndOfFileReached) && bEverythingOK)
        {
            str_t line[256] ;
            pstr_t pResult = fgets(line, sizeof(line), fileImport) ;
            if(NIL == pResult)
            {
                /* end of file */
                bEndOfFileReached = true ;
            }
            else
            {
                if(strlen(line) > (sizeof(line) - 3)) /* 3 <=> \n \r \0 */
                {
                    bEverythingOK = false ;
                    printf("%s - unexpected line too long \n", FuncName) ;
                }
                else
                {
                    str_t linkTarget[256] ;
                    str_t linkPathname[256] ;
                    bool_t bIgnoreLine = false ;

                    persadmin_common_remove_endofline(line) ;
                    if(0 == strcmp(line, "public"))
                    {
                        snprintf(linkTarget, sizeof(linkTarget), "%s",
                                bImportInCachedPath ? PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ : PERS_ORG_SHARED_PUBLIC_WT_PATH_) ;
                        if((sizeof(linkPathname)-1) < snprintf(linkPathname, sizeof(linkPathname), "%s%s",
                                                            appFolderPath, PERS_ORG_SHARED_PUBLIC_SYMLINK_NAME))
                        {
                            /* hard to believe, but anyway */
                            bIgnoreLine = true ;
                            printf("%s - unexpected linkPathname too long (%s%s) \n", FuncName, appFolderPath, PERS_ORG_SHARED_PUBLIC_SYMLINK_NAME) ;
                        }
                    }
                    else
                    {
                        sint_t groupID ;
                        sint_t result = sscanf(line, "%X", &groupID) ;
                        if(1 == result)
                        {
                            snprintf(linkTarget, sizeof(linkTarget), "%s%02X",
                                (bImportInCachedPath ? PERS_ORG_SHARED_GROUP_CACHE_PATH_ : PERS_ORG_SHARED_GROUP_WT_PATH_), groupID) ;
                            if((sizeof(linkPathname)-1) < snprintf(linkPathname, sizeof(linkPathname), "%s%s%02X", appFolderPath, PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX, groupID))
                            {
                                /* hard to believe, but anyway */
                                bIgnoreLine = true ;
                                printf("%s - unexpected linkPathname too long (%s%s%02X) \n", FuncName, appFolderPath, PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX, groupID) ;
                            }
                        }
                        else
                        {
                            bIgnoreLine = true ;
                            printf("%s - unable to extract group ID from (%s) - ignore it \n", FuncName, line) ;
                        }
                    }
                    if( ! bIgnoreLine)
                    {
                        if(0 <= persadmin_check_if_file_exists(linkPathname, false))
                        {
                            if(0 > persadmin_delete_file(linkPathname))
                            {
                                bEverythingOK = false ;
                                printf("%s - unable to delete existing link (%s) \n", FuncName, linkPathname) ;
                            }
                        }
                        if(bEverythingOK)
                        {
                            if(0 == persadmin_create_symbolic_link(linkPathname, linkTarget))
                            {
                                importedLinks++ ;
                            }
                            else
                            {
                                bEverythingOK = false ;
                                printf("%s - persadmin_create_symbolic_link(<%s>, <%s>) failed \n", FuncName, linkPathname, linkTarget) ;
                            }
                        }
                    }
                }
            }
        }
    }
    if(NIL != fileImport)
    {
        fclose(fileImport) ;
    }
    return bEverythingOK ? importedLinks : PAS_FAILURE;
}

/*
* linkName is not checked against NIL
* return group ID, or negative value for error
* it is assumed that lenght of linkName < PERSADMIN_MAX_PATH_LENGHT
**/
static sint_t persadmin_common_extract_group_id(pstr_t linkName)
{
    sint_t groupID = -1 ;
    sint_t lenPrefix = strlen(PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX) ;
    if(strlen(linkName) > lenPrefix)
    {
        if(0 == strncmp(linkName, PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX, lenPrefix))
        {
            if(1 != sscanf(linkName+lenPrefix, "%X", &groupID))
            {
                groupID = -1 ;
            }
        }
    }
    return groupID ;
}

/*
* line is not checked against NIL
* the content of line is changed
**/
static void persadmin_common_remove_endofline(pstr_t line)
{
    sint_t len = strlen(line) ;
    if((len > 0) && (('\r' == line[len-1]) || ('\n' == line[len-1])))
    {
        line[len-1] = '\0' ;
    }
    if((len > 1) && (('\r' == line[len-2]) || ('\n' == line[len-2])))
    {
        line[len-2] = '\0' ;
    }
}

sint_t persadmin_list_application_folders( pconststr_t rootPath, pstr_t list, sint_t listSize ) {
  sint_t result = PAS_FAILURE;
  if ( ( rootPath != 0 ) && ( list != 0 ) && ( listSize > 0 ) ) {
    GetApplicationRootPath( rootPath, completeRootPath );
    // Clear the output buffer before retrieving the actual list
    memset( list, 0, listSize );
    result = persadmin_list_folder( completeRootPath, list, listSize, PersadminFilterFolders, false );
    if ( result >= 0 ) {
      char * crtName = list;
      bool_t done = false;
      result += sizeof( StringTerminator );
      while ( ( ( * crtName ) != 0 ) && ( done == false ) ) {
        int length = strlen( crtName );
        if ( persadmin_is_shared_folder( crtName, length ) == true ) {
          result -= length + sizeof( StringTerminator );
          memmove( crtName, crtName + length + sizeof( StringTerminator ), listSize - ( crtName - list ) - length - sizeof( StringTerminator ) );
          done = true;
        }
        else {
          crtName += length + sizeof( StringTerminator );
        }
      }
    }
  }
  return result;
}

sint_t persadmin_list_application_folders_get_size( pconststr_t rootPath ) {
  sint_t result = PAS_FAILURE;
  GetApplicationRootPath( rootPath, completeRootPath );
  result = persadmin_list_folder_get_size( completeRootPath, PersadminFilterFolders, false );
  if ( result > 0 ) {
    // Add space for doubling the final '\0' - this is not done inside persadmin_list_folder_get_size()
    result += sizeof( StringTerminator );
  }
  return result;
}

bool_t persadmin_is_shared_folder( pconststr_t name, int length ) {
  bool_t result = false;
  int compLen = strlen( gSharedPathName );
  if ( ( name != 0 ) && ( length == compLen ) ) {
    int i = 0;
    result = true;
    for ( ; ( ( i < compLen ) && ( result == true ) ); ++i ) {
      if ( gSharedPathName[ i ] != name[ i ] ) {
        result = false;
      }
    }
  }
  return result;
}


/**
 * @brief Saves files together into a single archive.
 * @usage persadmin_compress("/path/to/compress/to/archive_name.tgz", "/path/from/where/to/compress")
 * @return 0 for success, negative value otherwise.
 */
sint_t persadmin_compress(pstr_t compressTo, pstr_t compressFrom)
{
    uint8_t              buffer         [READ_BUFFER_LENGTH];
    str_t                pchParentPath  [PATH_ABS_MAX_SIZE];
    pstr_t               pchStrPos      = NIL;
    struct archive       *psArchive     = NIL;
    struct archive       *psDisk        = NIL;
    struct archive_entry *psEntry       = NIL;
    sint_t               s32Result      = ARCHIVE_OK;
    sint_t               s32Length      = 0;
    sint_t               fd;
    sint_t               s32ParentPathLength = 0;


    if( (NIL == compressTo) ||
        (NIL == compressFrom) )
    {
        s32Result = ARCHIVE_FAILED;
        printf("persadmin_compress - invalid parameters \n");
    }

    if( ARCHIVE_OK == s32Result )
    {
        printf("persadmin_compress - create <%s> from <%s>\n", compressTo, compressFrom);
        psArchive = archive_write_new();
        if( NIL == psArchive )
        {
            s32Result = ARCHIVE_FAILED;
            printf("persadmin_compress - archive_write_new ERR\n");
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        /* this in turn calls archive_write_add_filter_gzip; */
        s32Result = archive_write_set_compression_gzip(psArchive);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_write_set_compression_gzip ERR %d\n", s32Result);
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        /* portable archive exchange; */
        archive_write_set_format_pax(psArchive);
        compressTo = (strcmp(compressTo, "-") == 0) ? NIL : compressTo;
        s32Result = archive_write_open_filename(psArchive, compressTo);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_write_open_filename ERR %d\n", s32Result);
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        psDisk = archive_read_disk_new();
        if( NIL == psDisk )
        {
            s32Result = ARCHIVE_FAILED;
            printf("persadmin_compress - archive_read_disk_new ERR\n");
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        archive_read_disk_set_standard_lookup(psDisk);
        s32Result = archive_read_disk_open(psDisk, compressFrom);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_read_disk_new ERR %s\n", archive_error_string(psDisk));
        }
    }

    memset(pchParentPath, 0, sizeof(pchParentPath));
    snprintf(pchParentPath, sizeof(pchParentPath), "%s", compressFrom);
    pchStrPos = strrchr(pchParentPath, '/');
    if(NIL != pchStrPos)
    {
        *pchStrPos = '\0';
    }
    s32ParentPathLength = strlen(pchParentPath);


    while( ARCHIVE_OK == s32Result )
    {
        psEntry = archive_entry_new();
        s32Result = archive_read_next_header2(psDisk, psEntry);

        switch( s32Result )
        {
            case ARCHIVE_EOF:
            {
                /* nothing else to do; */
                break;
            }
            case ARCHIVE_OK:
            {
                str_t   pstrTemp[PATH_ABS_MAX_SIZE];
                pstr_t  p = (pstr_t)archive_entry_pathname(psEntry);
                if(NIL != p)
                {
                    /* remove parent section and save relative pathnames */
                    memset(pstrTemp, 0, sizeof(pstrTemp));
                    snprintf(pstrTemp, sizeof(pstrTemp), "%s", p + (s32ParentPathLength + 1));
                    archive_entry_copy_pathname(psEntry, pstrTemp);
                }

                archive_read_disk_descend(psDisk);
                s32Result = archive_write_header(psArchive, psEntry);
                if( ARCHIVE_OK > s32Result)
                {
                    printf("persadmin_compress - archive_write_header ERR %s\n", archive_error_string(psArchive));
                }
                if( ARCHIVE_FATAL == s32Result )
                {
                    /* exit; */
                    printf("persadmin_compress - archive_write_header ERR FATAL\n");
                }
                if( ARCHIVE_FAILED < s32Result )
                {
#if 0
                    /* Ideally, we would be able to use
                     * the same code to copy a body from
                     * an archive_read_disk to an
                     * archive_write that we use for
                     * copying data from an archive_read
                     * to an archive_write_disk.
                     * Unfortunately, this doesn't quite
                     * work yet. */
                    persadmin_copy_data(psDisk, psArchive);
#else

                    /* For now, we use a simpler loop to copy data
                     * into the target archive. */
                    fd = open(archive_entry_sourcepath(psEntry), O_RDONLY);
                    s32Length = read(fd, buffer, READ_BUFFER_LENGTH);
                    while( s32Length > 0 )
                    {
                        archive_write_data(psArchive, buffer, s32Length);
                        s32Length = read(fd, buffer, READ_BUFFER_LENGTH);
                    }
                    close(fd);
#endif
                }

                break;
            }
            default:
            {
                printf("persadmin_compress - archive_read_next_header2 ERR %s\n", archive_error_string(psDisk));
                /* exit; */
                break;
            }
        }

        if( NIL != psEntry )
        {
            archive_entry_free(psEntry);
        }
    }

    /* perform cleaning operations; */
    if( NIL != psDisk )
    {
        archive_read_close(psDisk);
        archive_read_free(psDisk);
    }

    if( NIL != psArchive )
    {
        archive_write_close(psArchive);
        archive_write_free(psArchive);
    }

    /* overwrite result; */
    s32Result = (s32Result == ARCHIVE_EOF) ? ARCHIVE_OK : s32Result;
    /* return result; */
    return s32Result;

} /* persadmin_compress() */

/**
 * @brief Restore files from an archive.
 * @usage persadmin_uncompress("/path/from/where/to/extract/archive_name.tgz", "/path/where/to/extract/") - extract to specified folder
 * @usage persadmin_uncompress("/path/from/where/to/extract/archive_name.tgz", "") - extract to local folder
 * @return 0 for success, negative value otherwise.
 */
sint_t persadmin_uncompress(pstr_t extractFrom, pstr_t extractTo)
{
    struct archive          *psArchive  = NIL;
    struct archive          *psExtract  = NIL;
    struct archive_entry    *psEntry    = NIL;
    sint_t                  s32Result   = ARCHIVE_OK;
    sint_t                  s32Flags    = ARCHIVE_EXTRACT_TIME;

    /* select which attributes to restore; */
    s32Flags |= ARCHIVE_EXTRACT_PERM;
    s32Flags |= ARCHIVE_EXTRACT_ACL;
    s32Flags |= ARCHIVE_EXTRACT_FFLAGS;

    if( (NIL == extractFrom) ||
        (NIL == extractTo) )
    {
        s32Result = ARCHIVE_FAILED;
        printf("persadmin_uncompress - invalid parameters\n");
    }

    if( ARCHIVE_OK == s32Result )
    {
        psArchive = archive_read_new();
        if( NIL == psArchive )
        {
            s32Result = ARCHIVE_FAILED;
            printf("persadmin_uncompress - archive_read_new ERR\n");
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        archive_read_support_format_all(psArchive);
        archive_read_support_compression_all(psArchive);
        psExtract = archive_write_disk_new();
        s32Result = ((NIL == psExtract) ? ARCHIVE_FAILED : s32Result);
        archive_write_disk_set_options(psExtract, s32Flags);
        archive_write_disk_set_standard_lookup(psExtract);
    }

    if( ARCHIVE_OK == s32Result )
    {
        s32Result = archive_read_open_filename(psArchive, extractFrom, 10240);
        /* exit if s32Result != ARCHIVE_OK; */
    }

    while( ARCHIVE_OK == s32Result )
    {
        s32Result = archive_read_next_header(psArchive, &psEntry);
        switch( s32Result )
        {
            case ARCHIVE_EOF:
            {
                /* nothing else to do; */
                break;
            }
            case ARCHIVE_OK:
            {
                /* modify entry here to extract to the needed location; */
                str_t pstrTemp[PATH_ABS_MAX_SIZE];
                memset(pstrTemp, 0, sizeof(pstrTemp));
                snprintf(pstrTemp, sizeof(pstrTemp), "%s%s", extractTo, archive_entry_pathname(psEntry));
                printf("persadmin_uncompress - archive_entry_pathname %s\n", pstrTemp);
                archive_entry_copy_pathname(psEntry, pstrTemp);

                s32Result = archive_write_header(psExtract, psEntry);
                if( ARCHIVE_OK == s32Result )
                {
                    if( archive_entry_size(psEntry) > 0 )
                    {
                        s32Result = persadmin_copy_data(psArchive, psExtract);
                        if( ARCHIVE_OK != s32Result )
                        {
                            printf("persadmin_uncompress - copy_data ERR %s\n", archive_error_string(psExtract));
                        }
                        /* if( ARCHIVE_WARN > s32Result ) exit; */
                    }

                    if( ARCHIVE_OK == s32Result )
                    {
                        s32Result = archive_write_finish_entry(psExtract);
                        if( ARCHIVE_OK != s32Result )
                        {
                            printf("persadmin_uncompress - archive_write_finish_entry ERR %s\n", archive_error_string(psExtract));
                        }
                        /* if( ARCHIVE_WARN > s32Result ) exit; */
                    }
                }
                else
                {
                    printf("persadmin_uncompress - archive_write_header ERR %s\n", archive_error_string(psExtract));
                }

                break;
            }
            default:
            {
                printf("persadmin_uncompress - archive_read_next_header ERR %d\n", s32Result);
                break;
            }
        }
    }

    /* perform cleaning operations; */
    if( NIL != psArchive )
    {
        archive_read_close(psArchive);
        archive_read_free(psArchive);
    }
    if( NIL != psExtract )
    {
        archive_write_close(psExtract);
        archive_write_free(psExtract);
    }

    /* overwrite result; */
    s32Result = (s32Result == ARCHIVE_EOF) ? ARCHIVE_OK : s32Result;

    /* return result; */
    return s32Result;

} /* persadmin_uncompress() */


static sint_t persadmin_copy_data(struct archive *ar, struct archive *aw)
{
    sint_t  s32Result = ARCHIVE_OK;
    sint_t  s32Size   = 0;
    uint8_t buffer[COPY_BUFFER_LENGTH];

    while( ARCHIVE_OK == s32Result )
    {
        s32Size = archive_read_data(ar, buffer, COPY_BUFFER_LENGTH);
        if( 0 > s32Size )
        {
            printf("persadmin_copy_data - archive_read_data ERR\n");
            s32Result = ARCHIVE_FAILED;
        }
        else
        {
            if( 0 < s32Size )
            {
                s32Size = archive_write_data(aw, buffer, COPY_BUFFER_LENGTH);
                if( 0 > s32Size )
                {
                    printf("persadmin_copy_data - archive_write_data ERR\n");
                    s32Result = ARCHIVE_FAILED;
                }
            }
            else
            {
                /* nothing to copy; */
                break;
            }
        }
    }

    /* return result; */
    return s32Result;

} /* persadmin_copy_data() */
