#ifndef PERSISTENCE_ADMIN_SERVICE_H
#define PERSISTENCE_ADMIN_SERVICE_H
/*******************************************************************************
 *
 * Copyright (C) 2012 Continental Automotive Systems, Inc.
 *
 * Author: guy.sagnes@continental-corporation.com
 *
 * Interface: public - Implementation of the Interface PersAdminService   
 *
 * For additional details see
 * https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  Date       Author    Version  Description (GSA- to check with doxy)
 *  2013.04.30 G.Sagnes  2.1.0.0  add restore capability over persAdminDataRestore()
 *  2013.01.24 G.Sagnes  2.0.0.0  adapt functions name to GENIVI coding rules
 *                                change user and seat type to unsigned int
 *                                extend the error codes
 *  2012.11.06 G.Sagnes  1.1.0.0  add definition of PERSIST_SELECT_ALL_USER, 
 *                                PERSIST_SELECT_ALL_SEAT correction of return 
 *                                value for resource_config_add()
 *  2012.07.14 G.Sagnes  1.0.0.0  Initial version of the interface
 *
 ******************************************************************************/

/** \defgroup GEN_PERS_ADMINSERVICE_INTERFACE API document
 *  \{
 */

/** Module version
The lower significant byte is equal 0 for released version only
*/
#define PERSIST_ADMINSERVICE_INTERFACE_VERSION  (0x02010000U)

/** \defgroup PAS_RETURNS persAdmin Return Values
 * ::PAS_SUCCESS, ::PAS_ERROR_CODE, ::PAS_FAILURE_INVALID_PARAMETER
 
 * These defines are used to define the return values of the given low level access functions
 *   - ::PAS_SUCCESS: the function call succeded
 *   - ::PAS_ERROR_CODE..::PAS_FAILURE: the function call failed
 *  \{
 */
/* Error code return by the SW Package, related to SW_PackageID. */
#define PAS_PACKAGEID                           0x013                      /*!< Software package identifier, use for return value base */
#define PAS_BASERETURN_CODE                    (PAS_PACKAGEID << 16)       /*!< Basis of the return value containing SW PackageID */

#define PAS_SUCCESS                             0x00000000 /*!< the function call succeded */

#define PAS_ERROR_CODE                       (-(PAS_BASERETURN_CODE))      /*!< basis of the error (negative values) */
#define PAS_FAILURE_INVALID_PARAMETER          (PAS_ERROR_CODE - 1)        /*!< Invalid parameter in the API call  */
#define PAS_FAILURE_BUFFER_TOO_SMALL           (PAS_ERROR_CODE - 2)        /*!< The provided buffer can not accommodate the available data size */
#define PAS_FAILURE_OUT_OF_MEMORY              (PAS_ERROR_CODE - 3)        /*!< not enough memory, malloc failed, no handler available */
#define PAS_FAILURE_INVALID_FORMAT             (PAS_ERROR_CODE - 4)        /*!< format of the import source is not as expected (internal layout, type, etc) */
#define PAS_FAILURE_NOT_FOUND                  (PAS_ERROR_CODE - 5)        /*!< one of the following resource file, folder or key not found */
#define PAS_FAILURE_INCOMPLETE_OPERATION       (PAS_ERROR_CODE - 6)        /*!< operation not completed due to shut-down notification */
#define PAS_FAILURE_ACCESS_DENIED              (PAS_ERROR_CODE - 7)        /*!< tried to access a file without having the right */
#define PAS_FAILURE_DBUS_ISSUE                 (PAS_ERROR_CODE - 8)        /*!< related to DBUS */
#define PAS_FAILURE_OS_RESOURCE_ACCESS         (PAS_ERROR_CODE - 9)        /*!< related to mutex, queues, threads, etc.*/ 
#define PAS_FAILURE_OPERATION_NOT_SUPPORTED    (PAS_ERROR_CODE - 10)       /*!< operation not (yet) supported */ 

#define PAS_FAILURE                            (PAS_ERROR_CODE - 0xFFFF)   /*!< should be the max. value for error */

#define PAS_WARNING_CODE                       (PAS_BASERETURN_CODE)       /*!< basis of the warning (positive values) */                                                                                                                  
/** \} */

/** \defgroup PERS_ADMIN_HELPER Configuration parameter
 *  \{
 */

#define PERSIST_SELECT_ALL_USERS                (0xFFFFFFFF) /**< 32bit value used to allow access to all users */
#define PERSIST_SELECT_ALL_SEATS                (0xFFFFFFFF) /**< 32bit value used to allow access to all seats */

/** enumerator used to identify the type of selected data for backup, import, export */
typedef enum _PersASSelectionType_e
{
  PersASSelectionType_All          = 0,  /**< select all data/files: (node+user)->(application+shared) */
  PersASSelectionType_User         = 1,  /**< select user data/files: (user)->(application+shared) */
  PersASSelectionType_Application  = 2,  /**< select application data/files: (application)->(node+user) */
    /* insert new entries here ... */
  PersASSelectionType_LastEntry        /**< last entry */

} PersASSelectionType_e;


/** enumerator used to identify the type of selected data for backup, import, export 
* \since V2.1.0
*/
typedef enum _PersASDefaultSource_e
{
PersASDefaultSource_Factory 		= 0, /**< select from factory definition */
PersASDefaultSource_Configurable	= 1, /**< select from user factory or configurable default if exist */
/* insert new entries here ... */
PersASDefaultSource_LastEntry /**< last entry */
} PersASDefaultSource_e;
/** \} */ 

/**
 * \brief Allow creation of a backup on different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param backup_name name of the backup to allow identification
 * \param applicationID the application identifier
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes written; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataBackupCreate(PersASSelectionType_e type, const char* backup_name, const char* applicationID, unsigned int user_no, unsigned int seat_no);
 
/**
 * \brief Allow recovery of from backup on different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param backup_name name of the backup to allow identification
 * \param applicationID the application identifier
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes restored; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataBackupRecovery(PersASSelectionType_e type, const char* backup_name, const char* applicationID, unsigned int user_no, unsigned int seat_no);
 
/**
 * \brief Allow to identify and prepare the data to allow an export from system
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param dst_folder name of the destination folder for the data
 *
 * \return positive value: number of bytes written; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataFolderExport(PersASSelectionType_e type, const char* dst_folder);
 
/**
 * \brief Allow the import of data from specified folder to the system respecting different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param src_folder name of the source folder of the data
 *
 * \return positive value: number of bytes imported; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataFolderImport(PersASSelectionType_e type, const char* src_folder);
 
/**
 * \brief Allow to extend the configuration for persistency of data from specified level (application, user).
 *        Used during new persistency entry installation
 *
 * \param resource_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified entries in the resource configuration; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminResourceConfigAdd(const char* resource_file);
 
/**
 * \brief Allow the modification of the resource properties from data (key-values and files)
 *
 * \param resource_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified properties in the resource configuration; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminResourceConfigChangeProperties(const char* resource_file);
 
/**
 * \brief Allow the copy of user related data between different users
 *
 * \param src_user_no the user ID source
 * \param src_seat_no the seat number source (seat 0 to 3)
 * \param dest_user_no the user ID destination
 * \param dest_seat_no the seat number destination (seat 0 to 3)
 *
 * \return positive value: number of bytes copied; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminUserDataCopy(unsigned int src_user_no, unsigned int src_seat_no, unsigned int dest_user_no, unsigned int dest_seat_no);
 
/**
 * \brief Delete the user related data from persistency containers
 *
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes deleted; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminUserDataDelete(unsigned int user_no, unsigned int seat_no);

/**
* \brief Allow restore of values from default on different level (application, user or complete)
*
* \param type represents the data to restore: all, application, user
* \param defaultSource source of the default to allow reset
* \param applicationID the application identifier
* \param user_no the user ID
* \param seat_no the seat number (seat 0 to 3)
*
* \return positive value: number of bytes restored; negative value: error code (\ref PAS_RETURNS)
*
* \since V2.1.0
*/

long persAdminDataRestore(PersASSelectionType_e type, PersASDefaultSource_e defaultSource, 
                                          const char* applicationID, unsigned int user_no, unsigned int seat_no);


/** \} */ /* End of API */
#endif /* PERSISTENCE_ADMIN_SERVICE_H */


