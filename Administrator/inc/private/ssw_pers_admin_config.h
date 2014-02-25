#ifndef OSS_PERSISTENCE_ADMIN_CONFIG_H
#define OSS_PERSISTENCE_ADMIN_CONFIG_H
/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Interface: protected - Access to configuration files (json, xml)   
*
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.03.25 uidl9757  1.0.0.0  Created
*
**********************************************************************************************************************/


#ifdef __cplusplus
extern "C"
{
#endif  /* #ifdef __cplusplus */

#include "persComDataOrg.h"
#include "persComDbAccess.h"
#include "persComRct.h"


/*
 *  Structure of uncompressed WebTool's output
 *
 *  []resource
 *   ...[]public
 *   .   ...installRules.json
 *   .   ...[]public
 *   .       ...[]file
 *   .       .      []defaultData
 *   .       .      []configurableDefaultData
 *   .       .      []initialData
 *   .       ...[]key
 *   .       .      factoryDefaultData.json
 *   .       .      configurableDefaultData.json
 *   .       .      initialDefault.json
 *   .       ...installExceptions.json
 *   .       ...resourceconfiguration.json
 *   ...[]groups
 *   .   ...installRules.json
 *   .   ...[]10
 *   .       ...[]file
 *   .       .      []defaultData
 *   .       .      []configurableDefaultData
 *   .       .      []initialData
 *   .       ...[]key
 *   .       .      factoryDefaultData.json
 *   .       .      configurableDefaultData.json
 *   .       .      initialDefault.json
 *   .       ...GroupContent.json
 *   .       ...installExceptions.json
 *   .       ...resourceconfiguration.json
 *   .      []20
 *   ...[]apps
 *       ...installRules.json
 *       ...[]one_application
 *       .   ...[]file
 *       .   .      []defaultData
 *       .   .      []configurableDefaultData
 *       .   .      []initialData
 *       .   ...[]key
 *       .   .      factoryDefaultData.json
 *       .   .      configurableDefaultData.json
 *       .   .      initialDefault.json
 *       .   .  installExceptions.json
 *       .   ...resourceconfiguration.json
 *       ...[]another_application
 */

#define PERSADM_CFG_RESOURCE_ROOT_FOLDER_NAME "resource"
#define PERSADM_CFG_RESOURCE_PUBLIC_FOLDER_NAME "public"
#define PERSADM_CFG_RESOURCE_PUBLIC_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_ROOT_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_PUBLIC_FOLDER_NAME
#define PERSADM_CFG_RESOURCE_GROUPS_FOLDER_NAME "groups"
#define PERSADM_CFG_RESOURCE_GROUPS_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_ROOT_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_GROUPS_FOLDER_NAME
#define PERSADM_CFG_RESOURCE_APPS_FOLDER_NAME "apps"
#define PERSADM_CFG_RESOURCE_APPS_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_ROOT_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_APPS_FOLDER_NAME

#define PERSADM_CFG_RESOURCE_RCT_FILENAME "resourceconfiguration.json"
#define PERSADM_CFG_RESOURCE_INSTALL_RULES_FILENAME "installRules.json"
#define PERSADM_CFG_RESOURCE_INSTALL_EXCEPTIONS_FILENAME "installExceptions.json"
#define PERSADM_CFG_RESOURCE_GROUP_CONTENT_FILENAME "GroupContent.json"

#define PERSADM_CFG_RESOURCE_ROOT_FILEDATA_FOLDER_NAME "file"
#define PERSADM_CFG_RESOURCE_FACTORY_DEFAULT_FILEDATA_FOLDER_NAME PERS_ORG_DEFAULT_DATA_FOLDER_NAME
#define PERSADM_CFG_RESOURCE_CONFIG_DEFAULT_FILEDATA_FOLDER_NAME PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME
#define PERSADM_CFG_RESOURCE_NON_DEFAULT_FILEDATA_FOLDER_NAME "initialData.json"

#define PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME "key"
#define PERSADM_CFG_RESOURCE_FACTORY_DEFAULT_KEYDATA_FILENAME "factoryDefaultData.json"
#define PERSADM_CFG_RESOURCE_CONFIG_KEYDEFAULT_DATA_FILENAME "configurableDefaultData.json"
#define PERSADM_CFG_RESOURCE_NON_DEFAULT_KEYDATA_FILENAME "initialDefault.json"

 
/** types of configuration files */
typedef enum PersAdminCfgFileTypes_e_
{
    PersAdminCfgFileType_InstallRules    = 0,    /**< config rules for all the applications or groups */
    PersAdminCfgFileType_InstallExceptions,      /**< config rules for a single application or group */
    PersAdminCfgFileType_RCT,                    /**< resource configuration table */
    PersAdminCfgFileType_Database,               /**< database with default or init values for resourceIDs */
    PersAdminCfgFileType_GroupContent,           /**< list with the applications that are member of a group */

   /** insert new entries here ... */
    PersAdminCfgFileType_LastEntry         /**< last entry */
} PersAdminCfgFileTypes_e;


/** global rules */
typedef enum PersAdminCfgInstallRules_e_
{
    PersAdminCfgInstallRules_NewInstall    = 0,             /**< delete existent data and install the new data */
    PersAdminCfgInstallRules_Uninstall,                     /**< remove the existent data */
    PersAdminCfgInstallRules_DontTouch,                     /**< don't touch the application/group/resource */
    PersAdminCfgInstallRules_UpdateAll,                     /**< update(merge) the existent data with new data (factory-default, config-default and non-default) */
    PersAdminCfgInstallRules_UpdateAllSkipDefaultFactory,   /**< update(merge) the existent data with new data (config-default and non-default) */
    PersAdminCfgInstallRules_UpdateAllSkipDefaultConfig,    /**< update(merge) the existent data with new data (factory-default and non-default) */
    PersAdminCfgInstallRules_UpdateAllSkipDefaultAll,       /**< update(merge) the existent data with new data (non-default) */
    PersAdminCfgInstallRules_UpdateDefaultFactory,          /**< update(merge) the existent data with new data (factory-default) */
    PersAdminCfgInstallRules_UpdateDefaultConfig,           /**< update(merge) the existent data with new data (config-default) */
    PersAdminCfgInstallRules_UpdateDefaultAll,              /**< update(merge) the existent data with new data (factory-default, config-default) */
    PersAdminCfgInstallRules_UpdateSetOfResources,          /**< update/remove data for a set of resources (factory-default, config-default and non-default) */
    PersAdminCfgInstallRules_UninstallNonDefault,           /**< uninstall non-default data (similar to reset to default) */

   /** insert new entries here ... */
    PersAdminCfgInstallRules_LastEntry            /**< last entry */
} PersAdminCfgInstallRules_e;


/** local rules */
typedef enum PersAdminCfgInstallExceptions_e_
{
    PersAdminCfgInstallExceptions_Update    = 0,        /**< update(merge) the resource's existent data with new data */
    PersAdminCfgInstallExceptions_DontTouch,            /**< don't touch the resource's data */
    PersAdminCfgInstallExceptions_Delete,               /**< remove the resource's existent data */

   /** insert new entries here ... */
    PersAdminCfgInstallExceptions_LastEntry             /**< last entry */
} PersAdminCfgInstallExceptions_e;

 
/** entry in global rules file */
typedef struct PersAdminCfgInstallRulesEntry_s_
{
     char                           responsible[PERS_RCT_MAX_LENGTH_RESPONSIBLE];       /**< name of the application/group */
     PersAdminCfgInstallRules_e     eRule;                                              /**< config rule for itemIdentifier */
} PersAdminCfgInstallRulesEntry_s;

/** entry in local rules file */
typedef struct PersAdminCfgRuleLocalEntry_s_
{
    char                                itemIdentifier[PERS_DB_MAX_LENGTH_KEY_NAME];        /**< name of the resource */
     PersAdminCfgInstallExceptions_e    eRule;                                              /**< config rule for itemIdentifier */
} PersAdminCfgRuleLocalEntry_s;

/** entry in database */
typedef struct PersAdminCfgDbEntry_s_
{
    char                        itemIdentifier[PERS_DB_MAX_LENGTH_KEY_NAME];        /**< name of the resource */
    char                        data[PERS_DB_MAX_SIZE_KEY_DATA];                    /**< config rule for itemIdentifier */
} PersAdminCfgDbEntry_s ;


/** entry in group content */
typedef struct PersAdminCfgGroupContentEntry_s_
{
    char                        appName[PERS_RCT_MAX_LENGTH_RESPONSIBLE];           /**< name of the application that is member of the group */
} PersAdminCfgGroupContentEntry_s ;

/** entry in resource config table */
typedef struct PersAdminCfgRctEntry_s_
{
    char                            resourceID[PERS_RCT_MAX_LENGTH_RESOURCE_ID];        /**< identifier of the resource */
    PersistenceConfigurationKey_s   sConfig ;                                           /**< configuration for resourceID */
} PersAdminCfgRctEntry_s ;

/** entry in config files for key type resources */
typedef struct PersAdminCfgKeyDataEntry_s_
{
    char    identifier[PERS_DB_MAX_LENGTH_KEY_NAME];            /**< identifier of the resource */
    int     dataSize ;                                          /**< size of data */ 
    char    data[PERS_DB_MAX_SIZE_KEY_DATA];                    /**< init/default data for the key type resource */
} PersAdminCfgKeyDataEntry_s ;


/***********************************************************************************************************************************
*********************************************** General usage **********************************************************************
***********************************************************************************************************************************/

/** 
 * \brief Obtain a handler to config file (RCT, rules, DB) indicated by cfgFilePathname
 *
 * \param cfgFilePathname       [in] absolute path to config file (length limited to \ref PERS_ORG_MAX_LENGTH_PATH_FILENAME)
 * \param eCfgFileType          [in] absolute path to config file (length limited to \ref PERS_ORG_MAX_LENGTH_PATH_FILENAME)
 *
 * \return >= 0 for valid handler, negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgFileOpen(char const * filePathname, PersAdminCfgFileTypes_e eCfgFileType) ;

/**
 * \brief Close handler to config file
 *
 * \param handlerCfgFile        [in] handler obtained with persCfgFileOpen
 *
 * \return 0 for success, negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgFileClose(signed int handlerCfgFile) ;

/***********************************************************************************************************************************
*********************************************** RCT related ************************************************************************
***********************************************************************************************************************************/

/**
 * \brief read a resourceID's configuration from RCT
 *
 * \param handlerRCT    [in] handler obtained with persAdmCfgFileOpen
 * \param resourceID    [in] resource's identifier (length limited to \ref PERS_RCT_MAX_LENGTH_RESOURCE_ID)
 * \param psConfig_out  [out]where to return the configuration for resourceID
 *
 * \return 0 for success, or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRctRead(signed int handlerRCT, char const * resourceID, PersistenceConfigurationKey_s* psConfig_out) ;


/**
 * \brief Find the buffer's size needed to accommodate the list of resourceIDs in RCT
 * \note : resourceIDs in the list are separated by '\0'
 *
 * \param handlerRCT    [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRctGetSizeResourcesList(signed int handlerRCT) ;

/**
 * \brief Get the list of the resourceIDs in RCT
 * \note : resourceIDs in the list are separated by '\0'
 *
 * \param handlerRCT        [in] handler obtained with persAdmCfgFileOpen
 * \param listBuffer_out    [out]buffer where to return the list of resourceIDs
 * \param listBufferSize    [in] size of listBuffer_out
 *
 * \return list size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRctGetResourcesList(signed int handlerRCT, char* listBuffer_out, signed int listBufferSize) ;


/***********************************************************************************************************************************
*********************************************** Install Rules related **************************************************************
***********************************************************************************************************************************/

/**
 * \brief Find the buffer's size needed to accommodate the list of apps/groups names in install rules file
 * \note : the items in the list are separated by '\0'
 *
 * \param handlerRulesFile    [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRulesGetSizeFoldersList(signed int handlerRulesFile) ;

/**
 * \brief Get the list of apps/groups names in install rules file
 * \note : the items in the list are separated by '\0'
 *
 * \param handlerRulesFile  [in] handler obtained with persAdmCfgFileOpen
 * \param listBuffer_out    [out]buffer where to return the list of apps/groups names
 * \param listBufferSize    [in] size of listBuffer_out
 *
 * \return list size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRulesGetFoldersList(signed int handlerRulesFile, char* listBuffer_out, signed int listBufferSize) ;

/**
 * \brief Get the rule for the app/group install folder
 *
 * \param handlerRulesFile  [in] handler obtained with persAdmCfgFileOpen
 * \param folderName        [in] name of the app/group install folder
 * \param peRule_out        [out]buffer where to return the rule for the folder
 *
 * \return 0 for success, or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRulesGetRuleForFolder(signed int handlerRulesFile, char* folderName, PersAdminCfgInstallRules_e* peRule_out) ;


/***********************************************************************************************************************************
*********************************************** Install Exceptions related *********************************************************
***********************************************************************************************************************************/

/**
 * \brief Find the buffer's size needed to accommodate the list of resources in install exceptions file
 * \note : the items in the list are separated by '\0'
 *
 * \param handlerExceptionsFile     [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgExcGetSizeResourcesList(signed int handlerExceptionsFile) ;

/**
 * \brief Get the list of resources in install exceptions file
 * \note : the items in the list are separated by '\0'
 *
 * \param handlerExceptionsFile     [in] handler obtained with persAdmCfgFileOpen
 * \param listBuffer_out            [out]buffer where to return the list of apps/groups names
 * \param listBufferSize            [in] size of listBuffer_out
 *
 * \return list size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgExcGetFoldersList(signed int handlerExceptionsFile, char* listBuffer_out, signed int listBufferSize) ;

/**
 * \brief Get the rule for the app/group install folder
 *
 * \param handlerRulesFile  [in] handler obtained with persAdmCfgFileOpen
 * \param resource          [in] identifier of the resource affected by the exception
 * \param peException_out   [out]buffer where to return the exception for the resource
 *
 * \return 0 for success, or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgExcGetExceptionForResource(signed int handlerExceptionsFile, char* resource, PersAdminCfgInstallExceptions_e* peException_out) ;

/***********************************************************************************************************************************
*********************************************** Group content related **************************************************************
***********************************************************************************************************************************/

/**
 * \brief Find the buffer's size needed to accomodate the list of members in the group
 *
 * \param handlerGroupContent   [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgGroupContentGetSizeMembersList(signed int handlerGroupContent) ;


/**
 * \brief Get the list of members in the group
 * \note : resourceIDs in the list are separated by '\0'
 *
 * \param handlerGroupContent   [in] handler obtained with persAdmCfgFileOpen
 * \param listBuffer_out        [out]buffer where to return the list of resourceIDs
 * \param listBufferSize        [in] size of listBuffer_out
 *
 * \return list size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgGroupContentGetMembersList(signed int handlerGroupContent, char* listBuffer_out, signed int listBufferSize) ;


/***********************************************************************************************************************************
*********************************************** Default data related **************************************************************
***********************************************************************************************************************************/

/**
 * \brief Get the default value for the key type resourceID
 *
 * \param hDefaultDataFile      [in] handler obtained with persAdmCfgFileOpen
 *
 * \return size of default data (0 means there is no default data for the key), or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgReadDefaultData(signed int hDefaultDataFile, char const * resourceID, char* defaultDataBuffer_out, signed int bufferSize) ;


#ifdef __cplusplus
}
#endif /* extern "C" { */

#endif /* OSS_PERSISTENCE_ADMIN_CONFIG_H */
