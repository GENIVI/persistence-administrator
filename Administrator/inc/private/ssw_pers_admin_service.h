#ifndef SSW_PERS_ADMIN_SERVICE_H_
#define SSW_PERS_ADMIN_SERVICE_H_

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Interface: private - persistence admin service
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013.09.23 uidl9757  2.1.0.0  CSP_WZ#5781:  Watchdog timeout of pas-daemon
* 2013.04.19 uidu0250  2.0.0.0	CSP_WZ#3424:  Add IF extension for "restore to default"
* 2013.03.26 uidu0250  1.0.0.0  CSP_WZ#3171:  Update PAS registration to NSM
* 2012.11.12 uidl9757  1.0.0.0  CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#include "persistence_admin_service.h"

/**
 * \brief Re-trigger systemd watchdog
 *
 * \return none
 **/
void persadmin_RetriggerWatchdog(void);

/**
 * @brief Allow the export of data from specified folder to the system following different levels (application, user or complete)
 *
 * @param type represents the quality of the data to backup: all, application, user
 * @param dst_folder name of the destination folder of the data
 *
 * @return positive value: number of bytes imported; negative value: error code
 */
long persadmin_data_folder_export(PersASSelectionType_e type, pconststr_t dst_folder);

/**
 * @brief Allow recovery of from backup on different level (application, user or complete)
 *
 * @param type represents the quality of the data to backup: all, application, user
 * @param backup_name name of the backup to allow identification
 * @param applicationID the application identifier
 * @param user_no the user ID
 * @param seat_no the seat number (seat 1 to 4, 0xFF for all)
 *
 * @return positive value: number of written bytes; negative value: error code
 */
long persadmin_data_backup_create(PersASSelectionType_e type, char* backup_name, char* applicationID, unsigned int user_no, unsigned int seat_no);

/**
 * @brief Allow the import of data from specified folder to the system respecting different level (application, user or complete)
 *
 * @param type represent the quality of the data to backup: all, application, user
 * @param src_folder name of the source folder of the data
 *
 * @return positive value: number of bytes imported; negative value: error code
 */
long persadmin_data_folder_import(PersASSelectionType_e type, pconststr_t src_folder);

/**
 * @brief Allow recovery of from backup on different level (application, user or complete)
 *
 * @param type represent the quality of the data to backup: all, application, user
 * @param backup_name name of the backup to allow identification
 * @param applicationID the application identifier
 * @param user_no the user ID
 * @param seat_no the seat number (seat 1 to 4)
 *
 * @return positive value: number of bytes restored; negative value: error code
 */
long persadmin_data_backup_recovery(PersASSelectionType_e type, char* backup_name, char* applicationID, unsigned int user_no, unsigned int seat_no);

/**
* \brief Allows restore of default values on different level (application, user or complete)
*
* \param type represents the quality of the data to backup: all, application, user
* \param defaultSource source of the default to allow reset
* \param applicationID the application identifier
* \param user_no the user ID
* \param seat_no the seat number (seat 0 to 3)
*
* \return positive value: number of bytes restored; negative value: error code (\ref PAS_RETURNS)
*/
long persadmin_data_restore_to_default(PersASSelectionType_e type, PersASDefaultSource_e defaultSource, const char* applicationID, unsigned int user_no, unsigned int seat_no);


/**
 * @brief Delete user data
 *
 * @param type represent the quality of the data to backup: all, application, user
 * @param user_no the user ID
 * @param seat_no the seat number (seat 1 to 4)
 *
 * @return positive value: number of deleted bytes; negative value: error code
 */
long persadmin_user_data_delete(unsigned int user_no, unsigned int seat_no);


/**
 * @brief Allow to extend the configuration for persistency of data from specified level (application, user).
 *        Used during new persistency entry installation
 *
 * @param resource_file name of the persistency resource configuration to integrate
 *
 * @return positive value: number of modified entries in the resource configuration; negative value: error code
 */
long persadmin_resource_config_add(char* resource_file);


/**
 * @brief Allow the modification of the resource properties from data (key-values and files)
 *
 * @param resource_config_file name of the persistency resource configuration to integrate
 *
 * @return positive value: number of modified properties in the resource configuration; negative value: error code
 */
long persadmin_resource_config_change_properties(char* resource_config_file);


/**
 * @brief Allow the copy of user related data between different users
 *
 * @param src_user_no the user ID source
 * @param src_seat_no the seat number source (seat 0 to 3)
 * @param dest_user_no the user ID destination
 * @param dest_seat_no the seat number destination (seat 0 to 3)
 *
 * @return positive value: number of bytes copied; negative value: error code
 */
long persadmin_user_data_copy(unsigned int src_user_no, unsigned int src_seat_no, unsigned int dest_user_no, unsigned int dest_seat_no);

/**
 * @brief Set PAS shutdown state
 *
 * @param state the shutdown state : true if the shutdown is pending, false otherwise
 *
 * @return : void
 */
void persadmin_set_shutdown_state(bool_t state);

#endif /* SSW_PERS_ADMIN_SERVICE_H_ */
