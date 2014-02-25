/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ana.Chisca@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.10.04 uidl9757           CSP_WZ#5962:  Initialisation of Default Value does not work for big key (>63 bytes) 
* 2013.09.27 uidl9757           CSP_WZ#5781:  Fix memory leakage
* 2013.09.23 uidl9757           CSP_WZ#5781:  Watchdog timeout of pas-daemon
* 2013.07.03 uidl9757           CSP_WZ#4576:  Install "custom" keys with correct storage
* 2013.06.10 uidl9757           CSP_WZ#4285:  Custom plugin's name has to be extracted from value for "storage" tag
* 2013.04.03 uidl9757           CSP_WZ#3321:  Adapt to persComRct.h version 5.0.0.0
* 2013.04.02 uidl9757           CSP_WZ#2798:  Fix handling of "customID" and "custom_name"
* 2013.03.25 uidn3591           CSP_WZ#2798:  Initial version
*
**********************************************************************************************************************/

#include <json/json.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "dlt/dlt.h"

#include "persistence_admin_service.h"
#include "persComTypes.h"
#include "persComRct.h"
#include "ssw_pers_admin_config.h"

#define MAX_NESTING_DEPTH               ( 7)
#define UNDEFINED_SIZE                  (-1)

/* L&T context */
#define LT_HDR                          "CFG >> "
DLT_IMPORT_CONTEXT                      (persAdminSvcDLTCtx)

#define DEBUG

typedef struct _JSON_LookUp_s
{
    pstr_t   pstrValue_JSON;
    uint32_t u32Value_PAS;

} JSON_LookUp_s;


static JSON_LookUp_s g_JSON_LookUp_Table[] =
{
        /* Permissions */
        {"WO",                                                      PersistencePermission_WriteOnly},
        {"RO",                                                      PersistencePermission_ReadOnly},
        {"RW",                                                      PersistencePermission_ReadWrite},
        /* PersistencePolicy */
        {"NA",                                                      PersistencePolicy_na},
        {"cached",                                                  PersistencePolicy_wc},
        {"writethrough",                                            PersistencePolicy_wt},
        /* PersistenceStorage */
        {"local",                                                   PersistenceStorage_local},
        {"shared",                                                  PersistenceStorage_shared},
        {"hwinfo",                                                  PersistenceStorage_custom},
        {"early",                                                   PersistenceStorage_custom},
        {"emergency",                                               PersistenceStorage_custom},
        {"secure",                                                  PersistenceStorage_custom},
        {"custom1",                                                 PersistenceStorage_custom},
        {"custom2",                                                 PersistenceStorage_custom},
        {"custom3",                                                 PersistenceStorage_custom},
        /* PersAdminCfgInstallRules */
        {"PersAdminCfgInstallRules_NewInstall",                     PersAdminCfgInstallRules_NewInstall},
        {"PersAdminCfgInstallRules_Uninstall",                      PersAdminCfgInstallRules_Uninstall},
        {"PersAdminCfgInstallRules_DontTouch",                      PersAdminCfgInstallRules_DontTouch},
        {"PersAdminCfgInstallRules_UpdateAll",                      PersAdminCfgInstallRules_UpdateAll},
        {"PersAdminCfgInstallRules_UpdateAllSkipDefaultFactory",    PersAdminCfgInstallRules_UpdateAllSkipDefaultFactory},
        {"PersAdminCfgInstallRules_UpdateAllSkipDefaultConfig",     PersAdminCfgInstallRules_UpdateAllSkipDefaultConfig},
        {"PersAdminCfgInstallRules_UpdateAllSkipDefaultAll",        PersAdminCfgInstallRules_UpdateAllSkipDefaultAll},
        {"PersAdminCfgInstallRules_UpdateDefaultFactory",           PersAdminCfgInstallRules_UpdateDefaultFactory},
        {"PersAdminCfgInstallRules_UpdateDefaultConfig",            PersAdminCfgInstallRules_UpdateDefaultConfig},
        {"PersAdminCfgInstallRules_UpdateDefaultAll",               PersAdminCfgInstallRules_UpdateDefaultAll},
        {"PersAdminCfgInstallRules_UpdateSetOfResources",           PersAdminCfgInstallRules_UpdateSetOfResources},
        {"PersAdminCfgInstallRules_UninstallNonDefault",            PersAdminCfgInstallRules_UninstallNonDefault},
        /* PersAdminCfgInstallExceptions */
        {"PersAdminCfgInstallExceptions_Update",                    PersAdminCfgInstallExceptions_Update},
        {"PersAdminCfgInstallExceptions_DontTouch",                 PersAdminCfgInstallExceptions_DontTouch},
        {"PersAdminCfgInstallExceptions_Delete",                    PersAdminCfgInstallExceptions_Delete},
        /* */
        {"file",                                                    PersistenceResourceType_file},
        {"key",                                                     PersistenceResourceType_key},
        {"END_OF_LKUP_LIST", 0}
};


typedef enum _JSON_Key_LookUp_e
{
    JsonKey_Resources,
    JsonKey_Policy,
    JsonKey_Permission,
    JsonKey_Storage,
    JsonKey_MaxSize,
    JsonKey_Responsible,
    JsonKey_CustomID,
    JsonKey_CustomName,
    JsonKey_Size,
    JsonKey_Data,
	JsonKey_Type

} JSON_Key_LookUp_e;


static char* g_JSON_Key_Names[] =
{
   "resources",
   /**/
   "policy",
   "permission",
   "storage",
   "max_size",
   "responsible",
   "customID",
   "custom_name",
   /**/
   "size",
   "data",
   /**/
   "type"
};

static signed int  cfg_priv_read_file                       (signed int handlerRCT, char** ppBufferFile);
static signed int  cfg_priv_extract_rct_information         (char* pBuffer_in, signed int siBufferSize, PersistenceConfigurationKey_s* psConfig_out);
static signed int  cfg_priv_extract_rules_information       (char* pBuffer_in, PersAdminCfgInstallRules_e* peRule_out);
static signed int  cfg_priv_extract_exception_information   (char* pBuffer_in, PersAdminCfgInstallExceptions_e* peException_out);
static signed int  cfg_priv_extract_default_data_information(char* pBuffer_in, signed int siBufferSize_in, char* defaultDataBuffer_out, signed int siBufferSize_out);

static uint32_t    cfg_priv_json_to_pas                     (char* pBuffer_in);

static signed int  cfg_priv_json_parse_get_keys_size        (json_object* jsobj, char const * resourceID);
static signed int  cfg_priv_json_parse_get_keys_all_size    (json_object* jsobj);
static signed int  cfg_priv_json_parse_get_values_size      (json_object* jsobj, char const * resourceID);

/* recursive */
static signed int  cfg_priv_json_parse_get_keys             (json_object* jsobj, char const * resourceID, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter);
static signed int  cfg_priv_json_parse_get_keys_all         (json_object* jsobj, char** ppBuffer_out, signed int* psiBufferSize);
/* recursive */
static signed int  cfg_priv_json_parse_get_values           (json_object* jsobj, char const * resourceID, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter);
/* recursive */
static signed int  cfg_priv_json_parse_get_values_all       (json_object* jsobj, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter);
static signed int  cfg_priv_json_get_value                  (json_object *jsobj, char** ppBuffer_out, signed int* psiBufferSize);

#ifdef VERBOSE_TRACE
static void        print_buffer                             (char* pBuffer, unsigned int uiSize);
#endif


/***********************************************************************************************************************************
*********************************************** General usage **********************************************************************
***********************************************************************************************************************************/

/** 
 * \brief Obtain a handler to config file (RCT, rules, DB) indicated by cfgFilePathname
 *
 * \param cfgFilePathname       [in] absolute path to config file (length limited to \ref PERS_ORG_MAX_LENGTH_PATH_FILENAME)
 * \param eCfgFileType          [in]
 *
 * \return >= 0 for valid handler, negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgFileOpen(char const * filePathname, PersAdminCfgFileTypes_e eCfgFileType)
{
    /* invalid file handler */
    signed int siFileHandler = PAS_FAILURE_INVALID_PARAMETER;

    /* check input parameters */
    if( NIL == filePathname )
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgFileOpen"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
    }
    else
    {
        /* try to obtain handler */
        siFileHandler = open(filePathname, O_RDONLY);
        /* invalid file handler */
        if( siFileHandler < 0 )
        {
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgFileOpen"),
                    DLT_STRING("-"),
                    DLT_STRING("failed to open file"),
                    DLT_STRING(filePathname),
                    DLT_STRING("error"),
                    DLT_INT(siFileHandler));
            siFileHandler = PAS_FAILURE;
        }
    }

    /* return result */
    return siFileHandler;

} /* persAdmCfgFileOpen */

/**
 * \brief Close handler to config file
 *
 * \param handlerCfgFile        [in] handler obtained with persAdmCfgFileOpen
 *
 * \return 0 for success, negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgFileClose(signed int handlerCfgFile)
{
    signed int siResult = PAS_FAILURE_INVALID_PARAMETER;

    /* valid handler */
    if( 0 <= handlerCfgFile )
    {
        (void)close(handlerCfgFile);
        siResult = PAS_SUCCESS;
    }

    /* return result */
    return siResult;

} /* persAdmCfgFileClose() */

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
signed int persAdmCfgRctRead(signed int handlerRCT, char const * resourceID, PersistenceConfigurationKey_s* psConfig_out)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;
    char*       pBuffer_out = NIL;

    if( (NIL == resourceID)   ||
        (NIL == psConfig_out) ||
        (0 > handlerRCT))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgRctRead"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        #ifdef VERBOSE_TRACE
        /* some info */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
                        DLT_STRING("persAdmCfgRctRead"),
                        DLT_STRING("-"),
                        DLT_STRING(resourceID));
        #endif
        /* read file content */
        siResult = cfg_priv_read_file(handlerRCT, &pBufferFile);
        siResult = (0 == siResult) ? PAS_FAILURE : siResult;
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the RCT file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgRctRead"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size to accommodate values */
            siResult = cfg_priv_json_parse_get_values_size(jsobj, resourceID);
            siResult = (0 == siResult) ? PAS_FAILURE_NOT_FOUND : siResult;
        }

        if( PAS_SUCCESS < siResult )
        {
            pBuffer_out = (char*) malloc(((size_t)siResult * sizeof(char))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if( NIL == pBuffer_out )
            {
                /* raise & set error */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                        DLT_STRING("persAdmCfgRctRead"),
                        DLT_STRING("-"),
                        DLT_STRING("error"),
                        DLT_INT(siResult),
                        DLT_STRING("allocating"),
                        DLT_INT(siResult),
                        DLT_STRING("bytes"));
                siResult = PAS_FAILURE_OUT_OF_MEMORY;
            }
        }

        if( PAS_SUCCESS < siResult )
        {
            /* work with temporary variables */
            signed int siResultTemp     = siResult;
            char*      pBuffer_outTemp  = pBuffer_out;

            /* reset */
            (void)memset(pBuffer_out, 0, ((size_t)siResult * sizeof(char)));
            /* parse RCT file */
            siResultTemp = cfg_priv_json_parse_get_values(jsobj, resourceID, &pBuffer_outTemp, &siResultTemp, 0);
            /* interpret result */
            siResult = (siResultTemp != siResult) ? PAS_FAILURE : siResultTemp;
        }

        if( PAS_SUCCESS < siResult )
        {
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(pBuffer_out, (unsigned int)siResult);
#endif
            /* parse corresponding array and extract specific information */
            siResult = cfg_priv_extract_rct_information(pBuffer_out, siResult, psConfig_out);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* perform cleaning operations */
    if( NIL != pBuffer_out )
    {
        free(pBuffer_out); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBuffer_out = NIL;
    }

    if( NIL != pBufferFile )
    {
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* return 0 for success */
    return siResult;

} /* persAdmCfgRctRead() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Find the buffer's size needed to accommodate the list of resourceIDs in RCT
 * \note : resourceIDs in the list are separated by '\0'
 *
 * \param handlerRCT    [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRctGetSizeResourcesList(signed int handlerRCT)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL; /* deallocation is in caller's responsibility */

    if( 0 > handlerRCT )
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgRctGetSizeResourcesList"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerRCT, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the RCT file */
        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgRctGetSizeResourcesList"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size */
            siResult = cfg_priv_json_parse_get_keys_size(jsobj, g_JSON_Key_Names[JsonKey_Resources]);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */      
            json_object_put(jsobj) ;
        }
    }

    if(NIL != pBufferFile)
    {/* deallocation is in caller's responsibility */
        free(pBufferFile);
    }

    /* return size */
    return siResult;

} /* persAdmCfgRctGetSizeResourcesList() */


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
signed int persAdmCfgRctGetResourcesList(signed int handlerRCT, char* listBuffer_out, signed int listBufferSize)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;

    if( (NIL == listBuffer_out)   ||
        (0 > handlerRCT))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgRctGetResourcesList"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerRCT, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgRctGetResourcesList"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* work with temporary variables */
            char* pBuffer_outTemp = listBuffer_out;

            /* reset */
            (void)memset(listBuffer_out, 0, ((size_t)listBufferSize * sizeof(char)));
            siResult = listBufferSize;

            /* parse RCT file & search for 'resources' key */
            siResult = cfg_priv_json_parse_get_keys(jsobj, g_JSON_Key_Names[JsonKey_Resources], &pBuffer_outTemp, &siResult, 0);
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(listBuffer_out, (unsigned int)listBufferSize);
#endif
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }

        /* perform cleaning operations */
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* return number of bytes in buffer */
    return siResult;

} /* persAdmCfgRctGetResourcesList() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


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
signed int persAdmCfgRulesGetSizeFoldersList(signed int handlerRulesFile)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL; /* deallocation is in caller's responsibility */

    if( 0 > handlerRulesFile )
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
             DLT_STRING("persAdmCfgRulesGetSizeFoldersList"),
             DLT_STRING("-"),
             DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerRulesFile, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the install rules file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                 DLT_STRING("persAdmCfgRulesGetSizeFoldersList"),
                 DLT_STRING("-"),
                 DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size */
            siResult = cfg_priv_json_parse_get_keys_all_size(jsobj);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    if(NIL != pBufferFile)
    {/* deallocation is in caller's responsibility */
        free(pBufferFile);
    }
    
    /* return size */
    return siResult;

} /* persAdmCfgRulesGetSizeFoldersList() */


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
signed int persAdmCfgRulesGetFoldersList(signed int handlerRulesFile, char* listBuffer_out, signed int listBufferSize)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;

    if( (NIL == listBuffer_out)   ||
        (0 > handlerRulesFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgRulesGetFoldersList"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerRulesFile, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgRulesGetFoldersList"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* work with temporary variables */
            char* pBuffer_outTemp = listBuffer_out;

            /* reset */
            (void)memset(listBuffer_out, 0, ((size_t)listBufferSize * sizeof(char)));
            siResult = listBufferSize;

            /* parse install rules file & get all keys */
            siResult = cfg_priv_json_parse_get_keys_all(jsobj, &pBuffer_outTemp, &siResult);
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(listBuffer_out, (unsigned int)listBufferSize);
#endif
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }

        /* perform cleaning operations */
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* return number of bytes in buffer */
    return siResult;

} /* persAdmCfgRulesGetFoldersList() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Get the rule for the app/group install folder
 *
 * \param handlerRulesFile  [in] handler obtained with persAdmCfgFileOpen
 * \param folderName        [in] name of the app/group install folder
 * \param peRule_out        [out]buffer where to return the rule for the folder
 *
 * \return 0 for success, or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgRulesGetRuleForFolder(signed int handlerRulesFile, char* folderName, PersAdminCfgInstallRules_e* peRule_out)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;
    char*       pBuffer_out = NIL;

    if( (NIL == folderName) ||
        (NIL == peRule_out) ||
        (0 > handlerRulesFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgRulesGetRuleForFolder"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerRulesFile, &pBufferFile);
        siResult = (0 == siResult) ? PAS_FAILURE : siResult;
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the install file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgRulesGetRuleForFolder"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size to accommodate values */
            siResult = cfg_priv_json_parse_get_values_size(jsobj, folderName);
            siResult = (0 == siResult) ? PAS_FAILURE_NOT_FOUND : siResult;
        }

        if( PAS_SUCCESS < siResult )
        {
            pBuffer_out = (char*) malloc(((size_t)siResult * sizeof(char))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if( NIL == pBuffer_out )
            {
                /* raise & set error */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                        DLT_STRING("persAdmCfgRulesGetRuleForFolder"),
                        DLT_STRING("-"),
                        DLT_STRING("error"),
                        DLT_INT(siResult),
                        DLT_STRING("allocating"),
                        DLT_INT(siResult),
                        DLT_STRING("bytes"));
                siResult = PAS_FAILURE_OUT_OF_MEMORY;
            }
        }

        if( PAS_SUCCESS < siResult )
        {
            /* work with temporary variables */
            signed int siResultTemp     = siResult;
            char*      pBuffer_outTemp  = pBuffer_out;

            /* reset */
            (void)memset(pBuffer_out, 0, ((size_t)siResult * sizeof(char)));
            /* parse install rules file */
            siResultTemp = cfg_priv_json_parse_get_values(jsobj, folderName, &pBuffer_outTemp, &siResultTemp, 0);
            /* interpret result */
            siResult = (siResultTemp != siResult) ? PAS_FAILURE : siResultTemp;
        }

        if( PAS_SUCCESS < siResult )
        {
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(pBuffer_out, (unsigned int)siResult);
#endif

            /* parse corresponding array and extract specific information */
            siResult = cfg_priv_extract_rules_information(pBuffer_out, peRule_out);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* perform cleaning operations */
    if( NIL != pBuffer_out )
    {
        free(pBuffer_out); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBuffer_out = NIL;
    }

    if( NIL != pBufferFile )
    {
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* return 0 for success */
    return siResult;

} /* persAdmCfgRulesGetRuleForFolder() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


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
signed int persAdmCfgExcGetSizeResourcesList(signed int handlerExceptionsFile)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL; /* deallocation is in caller's responsibility */

    if( 0 > handlerExceptionsFile )
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
             DLT_STRING("persAdmCfgExcGetSizeResourcesList"),
             DLT_STRING("-"),
             DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerExceptionsFile, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the install exceptions file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                 DLT_STRING("persAdmCfgExcGetSizeResourcesList"),
                 DLT_STRING("-"),
                 DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size */
            siResult = cfg_priv_json_parse_get_keys_all_size(jsobj);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    if(NIL != pBufferFile)
    {/* deallocation is in caller's responsibility */
        free(pBufferFile) ;
    }

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
         DLT_STRING("persAdmCfgExcGetSizeResourcesList"),
         DLT_STRING("-"),
         DLT_STRING("size ="),
         DLT_INT(siResult));

    /* return size */
    return siResult;

} /* persAdmCfgExcGetSizeResourcesList() */


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
signed int persAdmCfgExcGetFoldersList(signed int handlerExceptionsFile, char* listBuffer_out, signed int listBufferSize)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;

    if( (NIL == listBuffer_out)   ||
        (0 > handlerExceptionsFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgExcGetFoldersList"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerExceptionsFile, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgExcGetFoldersList"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* work with temporary variables */
            char* pBuffer_outTemp = listBuffer_out;

            /* reset */
            (void)memset(listBuffer_out, 0, ((size_t)listBufferSize * sizeof(char)));
            siResult = listBufferSize;

            /* parse install exceptions file & get all keys */
            siResult = cfg_priv_json_parse_get_keys_all(jsobj, &pBuffer_outTemp, &siResult);
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(listBuffer_out, (unsigned int)listBufferSize);
#endif
        }

        /* perform cleaning operations */
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* return number of bytes in buffer */
    return siResult;

} /* persAdmCfgExcGetFoldersList() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Get the rule for the app/group install folder
 *
 * \param handlerRulesFile  [in] handler obtained with persAdmCfgFileOpen
 * \param resource          [in] identifier of the resource affected by the exception
 * \param peException_out   [out]buffer where to return the exception for the resource
 *
 * \return 0 for success, or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgExcGetExceptionForResource(signed int handlerExceptionsFile, char* resource, PersAdminCfgInstallExceptions_e* peException_out)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;
    char*       pBuffer_out = NIL;

    if( (NIL == resource) ||
        (NIL == peException_out) ||
        (0 > handlerExceptionsFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgExcGetExceptionForResource"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerExceptionsFile, &pBufferFile);
        siResult = (0 == siResult) ? PAS_FAILURE : siResult;
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the install file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgExcGetExceptionForResource"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size to accommodate values */
            siResult = cfg_priv_json_parse_get_values_size(jsobj, resource);
            siResult = (0 == siResult) ? PAS_FAILURE_NOT_FOUND : siResult;
        }

        if( PAS_SUCCESS < siResult )
         {
             pBuffer_out = (char*) malloc(((size_t)siResult * sizeof(char))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
             if( NIL == pBuffer_out )
             {
                 /* raise & set error */
                 DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                         DLT_STRING("persAdmCfgExcGetExceptionForResource"),
                         DLT_STRING("-"),
                         DLT_STRING("error"),
                         DLT_INT(siResult),
                         DLT_STRING("allocating"),
                         DLT_INT(siResult),
                         DLT_STRING("bytes"));
                 siResult = PAS_FAILURE_OUT_OF_MEMORY;
             }
         }

         if( PAS_SUCCESS < siResult )
         {
             /* work with temporary variables */
             signed int siResultTemp     = siResult;
             char*      pBuffer_outTemp  = pBuffer_out;

             /* reset */
             (void)memset(pBuffer_out, 0, ((size_t)siResult * sizeof(char)));
             /* parse install exceptions file */
             siResultTemp = cfg_priv_json_parse_get_values(jsobj, resource, &pBuffer_outTemp, &siResultTemp, 0);
             /* interpret result */
             siResult = (siResultTemp != siResult) ? PAS_FAILURE : siResultTemp;
         }

         if( PAS_SUCCESS < siResult )
         {
#ifdef VERBOSE_TRACE
             /* debug only */
             print_buffer(pBuffer_out, (unsigned int)siResult);
#endif
             /* parse corresponding array and extract specific information */
             siResult = cfg_priv_extract_exception_information(pBuffer_out, peException_out);
         }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* perform cleaning operations */
    if( NIL != pBuffer_out )
    {
        free(pBuffer_out); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBuffer_out = NIL;
    }

    if( NIL != pBufferFile )
    {
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* return 0 for success */
    return siResult;

} /* persAdmCfgExcGetExceptionForResource() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/***********************************************************************************************************************************
*********************************************** Group content related **************************************************************
***********************************************************************************************************************************/

/**
 * \brief Find the buffer's size needed to accommodate the list of members in the group
 *
 * \param handlerGroupContent   [in] handler obtained with persAdmCfgFileOpen
 *
 * \return needed size [byte], or negative value for error (\ref PERSADM_CFG_ERROR_CODES_DEFINES)
 */
signed int persAdmCfgGroupContentGetSizeMembersList(signed int handlerGroupContent)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL; /* deallocation is in caller's responsibility */

    if( 0 > handlerGroupContent )
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
             DLT_STRING("persAdmCfgGroupContentGetSizeMembersList"),
             DLT_STRING("-"),
             DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerGroupContent, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the groups file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                 DLT_STRING("persAdmCfgGroupContentGetSizeMembersList"),
                 DLT_STRING("-"),
                 DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size */
            siResult = cfg_priv_json_parse_get_keys_all_size(jsobj);
        }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */            
            json_object_put(jsobj) ;
        }
    }

    if(NIL != pBufferFile)
    {/* deallocation is in caller's responsibility */
        free(pBufferFile);
    }
    
    /* return size */
    return siResult;

} /* persAdmCfgGroupContentGetSizeMembersList() */


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
signed int persAdmCfgGroupContentGetMembersList(signed int handlerGroupContent, char* listBuffer_out, signed int listBufferSize)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;

    if( (NIL == listBuffer_out)   ||
        (0 > handlerGroupContent))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgGroupContentGetMembersList"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(handlerGroupContent, &pBufferFile);
    }

    if( PAS_SUCCESS < siResult )
    {
        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgGroupContentGetMembersList"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* work with temporary variables */
            char* pBuffer_outTemp = listBuffer_out;

            /* reset */
            (void)memset(listBuffer_out, 0, (size_t)listBufferSize * sizeof(char));
            siResult = listBufferSize;

            /* parse groups file & get all keys */
            siResult = cfg_priv_json_parse_get_keys_all(jsobj, &pBuffer_outTemp, &siResult);
#ifdef VERBOSE_TRACE
            /* debug only */
            print_buffer(listBuffer_out, (unsigned int)listBufferSize);
#endif
        }

        /* perform cleaning operations */
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* return number of bytes in buffer */
    return siResult;

} /* persAdmCfgGroupContentGetMembersList() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


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
signed int persAdmCfgReadDefaultData(signed int hDefaultDataFile, char const * resourceID, char* defaultDataBuffer_out, signed int bufferSize)
{
    signed int  siResult    = PAS_SUCCESS;
    char*       pBufferFile = NIL;
    char*       pBuffer_out = NIL;

    if( (NIL == resourceID) ||
        (NIL == defaultDataBuffer_out) ||
        (0 > hDefaultDataFile))
    {
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("persAdmCfgReadDefaultData"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* read file content */
        siResult = cfg_priv_read_file(hDefaultDataFile, &pBufferFile);
        siResult = (0 == siResult) ? PAS_FAILURE : siResult;
    }

    if( PAS_SUCCESS < siResult )
    {
        /* parse the default data file */

        json_object* jsobj = NIL; /* deallocation is in caller's responsibility */
        jsobj = json_tokener_parse(pBufferFile);
        if( NIL == jsobj )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("persAdmCfgReadDefaultData"),
                    DLT_STRING("-"),
                    DLT_STRING("json_tokener_parse"));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* get size to accommodate values */
            siResult = cfg_priv_json_parse_get_values_size(jsobj, resourceID);
            siResult = (0 == siResult) ? PAS_FAILURE_NOT_FOUND : siResult;
        }

        if( PAS_SUCCESS < siResult )
        {
             pBuffer_out = (char*) malloc(((size_t)siResult * sizeof(char))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
             if( NIL == pBuffer_out )
             {
                 /* raise & set error */
                 DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                         DLT_STRING("persAdmCfgReadDefaultData"),
                         DLT_STRING("-"),
                         DLT_STRING("error"),
                         DLT_INT(siResult),
                         DLT_STRING("allocating"),
                         DLT_INT(siResult),
                         DLT_STRING("bytes"));
                 siResult = PAS_FAILURE_OUT_OF_MEMORY;
             }
        }

         if( PAS_SUCCESS < siResult )
         {
             /* work with temporary variables */
             signed int siResultTemp     = siResult;
             char*      pBuffer_outTemp  = pBuffer_out;

             /* reset */
             (void)memset(pBuffer_out, 0, (size_t)siResult * sizeof(char));
             /* parse default data file */
             siResultTemp = cfg_priv_json_parse_get_values(jsobj, resourceID, &pBuffer_outTemp, &siResultTemp, 0);
             /* interpret result */
             siResult = (siResultTemp != siResult) ? PAS_FAILURE : siResultTemp;
         }

         if( PAS_SUCCESS < siResult )
         {
#ifdef VERBOSE_TRACE
             /* debug only */
             print_buffer(pBuffer_out, (unsigned int)siResult);
#endif
             /* parse corresponding array and extract specific information */
             siResult = cfg_priv_extract_default_data_information(pBuffer_out, siResult, defaultDataBuffer_out, bufferSize);
         }

        if(NIL != jsobj)
        {/* deallocation is in caller's responsibility */
            json_object_put(jsobj) ;
        }
    }

    /* perform cleaning operations */
    if( NIL != pBuffer_out )
    {
        free(pBuffer_out); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBuffer_out = NIL;
    }

    if( NIL != pBufferFile )
    {
        free(pBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pBufferFile = NIL;
    }

    /* size of data */
    return siResult;

} /* persAdmCfgReadDefaultData() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static signed int cfg_priv_read_file(signed int handlerRCT, char** ppBufferFile)
{
    signed int  siResult = PAS_SUCCESS;
    struct stat sstat;

    if( NIL == ppBufferFile )
    {
        /* raise & set error */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                DLT_STRING("cfg_priv_read_file"),
                DLT_STRING("-"),
                DLT_STRING("invalid input parameters"));
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        /* re-position at the start of file */
        (void)lseek(handlerRCT, 0, SEEK_SET);
        
        siResult = fstat(handlerRCT, &sstat);
        if( PAS_SUCCESS > siResult )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("cfg_priv_read_file"),
                    DLT_STRING("-"),
                    DLT_STRING("stat error"),
                    DLT_INT(siResult));
            siResult = PAS_FAILURE;
        }
        else
        {
            /* allocate memory to read in the file */
            *ppBufferFile = (char*) malloc(((size_t)sstat.st_size + 1 /* '\0' */) * sizeof(char)); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if( NIL == (*ppBufferFile) )
            {
                /* raise & set error */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                        DLT_STRING("persAdmCfgRctRead"),
                        DLT_STRING("-"),
                        DLT_STRING("error"),
                        DLT_INT(siResult),
                        DLT_STRING("allocating"),
                        DLT_INT(sstat.st_size + 1),
                        DLT_STRING("bytes"));
                siResult = PAS_FAILURE_OUT_OF_MEMORY;
            }
        }
    }

    if( PAS_SUCCESS <= siResult )
    {
        /* reset */
        (void)memset(*ppBufferFile, 0, ((size_t)(sstat.st_size + 1 /* '\0' */) * sizeof(char)) );

        siResult = read(handlerRCT, *ppBufferFile, (size_t)sstat.st_size);
        if( siResult != sstat.st_size )
        {
            /* raise & set error */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR),
                    DLT_STRING("cfg_priv_read_file"),
                    DLT_STRING("-"),
                    DLT_STRING("read error"),
                    DLT_INT(siResult),
                    DLT_STRING("!="),
                    DLT_INT(sstat.st_size));
            siResult = PAS_FAILURE;

            /* free the allocated memory */
            free(*ppBufferFile); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            *ppBufferFile = NIL;
        }
    }

    /* return number of bytes in the file */
    return siResult;

} /* cfg_priv_read_file() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/* Assumption that pBuffer_in contains information in the following form: "key1\0data1\0key2\0data2\0.." */
static signed int cfg_priv_extract_rct_information(char* pBuffer_in, signed int siBufferSize, PersistenceConfigurationKey_s * psConfig_out)
{
    signed int siResult = PAS_SUCCESS;

    /* paranoia check */
    if( (NIL == pBuffer_in)   ||
        (NIL == psConfig_out) ||
        (0 > siBufferSize))
    {
        /* set error */
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        char* pBufferTemp = pBuffer_in;

        /* reset the buffer content */
        (void)memset(psConfig_out, 0x0, sizeof(PersistenceConfigurationKey_s)) ;
        
        while( pBufferTemp < (pBuffer_in + siBufferSize) )/* TODO: compute end size only once */
        {
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Policy], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                psConfig_out->policy = (PersistencePolicy_e)cfg_priv_json_to_pas(pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Storage], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                psConfig_out->storage = (PersistenceStorage_e)cfg_priv_json_to_pas(pBufferTemp);
                if(PersistenceStorage_custom == psConfig_out->storage)
                {
                    /* for the situation when WebTool does not provide the name of the custom plugin in a separate tag "custom_name" */
                    (void)snprintf(psConfig_out->custom_name, sizeof(psConfig_out->custom_name), "%s", pBufferTemp);
                }
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Type], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                psConfig_out->type = (PersistenceResourceType_e)cfg_priv_json_to_pas(pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Permission], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                psConfig_out->permission = cfg_priv_json_to_pas(pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_MaxSize], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                psConfig_out->max_size = (unsigned int)atoi(pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Responsible], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                (void)snprintf(psConfig_out->reponsible, sizeof(psConfig_out->reponsible), "%s", pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_CustomName], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                (void)snprintf(psConfig_out->custom_name, sizeof(psConfig_out->custom_name), "%s", pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_CustomID], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                (void)snprintf(psConfig_out->customID, sizeof(psConfig_out->customID), "%s", pBufferTemp);
            }

            pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
        }
    }

    return siResult;

} /* cfg_priv_extract_rct_information() */


static signed int cfg_priv_extract_rules_information(char* pBuffer_in, PersAdminCfgInstallRules_e* peRule_out)
{
    signed int siResult = PAS_SUCCESS;

    /* paranoia check */
    if( (NIL == pBuffer_in) ||
        (NIL == peRule_out) )
    {
        /* set error */
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        *peRule_out = (PersAdminCfgInstallRules_e)cfg_priv_json_to_pas(pBuffer_in);
    }

    return siResult;

} /* cfg_priv_extract_rules_information() */


static signed int cfg_priv_extract_exception_information(char* pBuffer_in, PersAdminCfgInstallExceptions_e* peException_out)
{
    signed int siResult = PAS_SUCCESS;

    /* paranoia check */
    if( (NIL == pBuffer_in) ||
        (NIL == peException_out) )
    {
        /* set error */
        siResult = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
        *peException_out = (PersAdminCfgInstallExceptions_e)cfg_priv_json_to_pas(pBuffer_in);
    }

    return siResult;

} /* cfg_priv_extract_exception_information() */


static signed int cfg_priv_extract_default_data_information(char* pBuffer_in, signed int siBufferSize_in, char* defaultDataBuffer_out, signed int siBufferSize_out)
{
    signed int siDataSize = 0;

    /* paranoia check */
    if( (NIL == pBuffer_in) ||
        (NIL == defaultDataBuffer_out) ||
        (0 > siBufferSize_out))
    {
        /* set error */
        siDataSize = PAS_FAILURE_INVALID_PARAMETER;
    }
    else
    {
    	char* pBufferData      = NIL ;
        char* pBufferTemp      = pBuffer_in;

        /* size'\0'15'\0'data'\0'0102030405060708090A0B0C0D0E0F'\0'; */
        while( pBufferTemp < (pBuffer_in + siBufferSize_in) )
        {
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Size], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                siDataSize = atoi(pBufferTemp);
            }
            if( 0 == strcmp(g_JSON_Key_Names[JsonKey_Data], pBufferTemp) )
            {
                pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
                pBufferData = pBufferTemp ;
            }
            pBufferTemp += (strlen(pBufferTemp) + 1 /* \0 */);
        }

        if( (0 > siDataSize) || (siDataSize > siBufferSize_out) || (NIL == pBufferData) )
        {
            /* raise & set error */
            siDataSize = PAS_FAILURE_INVALID_PARAMETER;
        }
        else
        {
            unsigned int i = 0;
            pBufferTemp    = pBufferData;

            /* parse buffer */
            while( pBufferTemp < (pBufferData + (siDataSize * 2)) )
            {
                (void)sscanf(pBufferTemp, "%2x", (unsigned int*)(defaultDataBuffer_out + i));
                /* printf("%d : 0x%x\n", i, *(defaultDataBuffer_out + i)); */
                /* step over 2 characters */
                pBufferTemp += 2;
                /* move on to the next element in output buffer */
                ++i;
            }
        }
    }

    /* return number of bytes */
    return siDataSize;

} /* cfg_priv_extract_default_data_information() */


/* JSON <-> PAS */

static uint32_t cfg_priv_json_to_pas(char* pBuffer_in)
{
    uint32_t u32Result     = 0;
    uint32_t i             = 0;
    bool_t   bFound        = false;
    uint32_t u32ElementsNo = sizeof(g_JSON_LookUp_Table)/sizeof(g_JSON_LookUp_Table[0]);

    for( i = 0; (i < u32ElementsNo) && (false == bFound); ++i )
    {
        if( 0 == strcmp(g_JSON_LookUp_Table[i].pstrValue_JSON, pBuffer_in) )
        {
            u32Result = g_JSON_LookUp_Table[i].u32Value_PAS;
            bFound = true;
        }
    }

    return u32Result;

} /* cfg_priv_json_to_pas() */


/* JSON PARSER */

static signed int cfg_priv_json_parse_get_keys(json_object* jsobj, char const * resourceID, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter)
{
    signed int          siResult = 0;
    enum json_type      type;
    char*               key      = NIL;
    struct json_object* val      = NIL;
    struct lh_entry*    entry    = NIL;

    if( (++uiNestingCounter) >= MAX_NESTING_DEPTH )
    {
        /* set error */
        siResult = PAS_FAILURE;
        /* printf("cfg_priv_json_parse_get_keys - MAX_NESTING_DEPTH\n"); */
    }

    if( NIL != json_object_get_object(jsobj) )
    {
        entry = json_object_get_object(jsobj)->head;
        key = ( NIL != entry ) ? (char*)entry->k : key;
        val = ( NIL != entry ) ? (struct json_object*)entry->v : val;

        while( (NIL != entry) && (NIL != key) && (NIL != val) && (0 <= siResult) )
        {
            if( 0 == strcmp(resourceID, key) )
            {
                type = json_object_get_type(val);
                if( json_type_object == type )
                {
                    siResult = cfg_priv_json_parse_get_keys_all(json_object_object_get(jsobj, key), ppBuffer_out, psiBufferSize);
                }
                break;
            }
            else
            {
                type = json_object_get_type(val);
                if( json_type_object == type )
                {
                    /* recursive */
                    signed int sTempResult = cfg_priv_json_parse_get_keys(json_object_object_get(jsobj, key), resourceID, ppBuffer_out, psiBufferSize, uiNestingCounter);
                    siResult = (sTempResult < 0) ? (sTempResult) : (siResult + sTempResult);
                }
            }

            /* move on to the next entry */
            entry = entry->next;
            key = ( NIL != entry ) ? (char*)entry->k : key;
            val = ( NIL != entry ) ? (struct json_object*)entry->v : val;
        }
    }

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_keys() */


static signed int cfg_priv_json_parse_get_keys_all(json_object* jsobj, char** ppBuffer_out, signed int* psiBufferSize)
{
    signed int          siResult    = 0;
    char*               key         = NIL;
    struct lh_entry*    entry       = NIL;
    /* remember starting point */
    char*               pBufferTemp      = *ppBuffer_out;
    signed int          siBufferSizeTemp = *psiBufferSize;

    if( NIL != json_object_get_object(jsobj) )
    {
        entry = json_object_get_object(jsobj)->head;
        key = ( NIL != entry ) ? (char*)entry->k : key;

        while( (NIL != entry) && (NIL != key) )
        {
            size_t szKey = strlen(key) + 1;
            if( NIL != pBufferTemp  )
            {
                if( (siResult + (signed int)szKey) <= siBufferSizeTemp )
                {
                    /* get key */
                    (void)snprintf(*ppBuffer_out, (size_t)(*psiBufferSize), "%s%c", key, '\0');
                    *ppBuffer_out  += szKey;
                    *psiBufferSize -= szKey;

                    /* get key length */
                    siResult += (signed int)szKey;
                }
                else
                {
                    siResult = PAS_FAILURE_BUFFER_TOO_SMALL;
                }
            }
            else
            {
                /* get key length */
                siResult += (signed int)szKey;
            }

            /* move on to the next entry */
            entry = entry->next;
            key = ( NIL != entry ) ? (char*)entry->k : key;
        }
    }

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_keys_all() */


static signed int cfg_priv_json_parse_get_values(json_object* jsobj, char const * resourceID, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter)
{
    signed int          siResult = 0;
    enum json_type      type;
    char*               key      = NIL;
    struct json_object* val      = NIL;
    struct lh_entry*    entry    = NIL;

    if( (++uiNestingCounter) >= MAX_NESTING_DEPTH )
    {
        /* set error */
        siResult = PAS_FAILURE;
        /* printf("cfg_priv_json_parse_get_values - MAX_NESTING_DEPTH\n"); */
    }

    if( NIL != json_object_get_object(jsobj) )
    {
        entry = json_object_get_object(jsobj)->head;
        key = ( NIL != entry ) ? (char*)entry->k : key;
        val = ( NIL != entry ) ? (struct json_object*)entry->v : val;

        while( (NIL != entry) && (NIL != key) && (NIL != val) && (0 <= siResult) )
        {
            if( 0 == strcmp(resourceID, key) )
            {
                /* key found; */
                type = json_object_get_type(val);
                if( json_type_object == type )
                {
                    siResult = cfg_priv_json_parse_get_values_all(json_object_object_get(jsobj, key), ppBuffer_out, psiBufferSize, uiNestingCounter);
                }
                else
                {
                    /* get value */
                    siResult = cfg_priv_json_get_value(val, ppBuffer_out, psiBufferSize);
                }

                /* exit for; */
                break;
            }
            else
            {
                type = json_object_get_type(val);
                if( json_type_object == type )
                {
                    /* recursive */
                    signed int sTempResult = cfg_priv_json_parse_get_values(json_object_object_get(jsobj, key), resourceID, ppBuffer_out, psiBufferSize, uiNestingCounter);
                    siResult = (sTempResult < 0) ? (sTempResult) : (siResult + sTempResult);
                }
            }

            /* move on to the next entry */
            entry = entry->next;
            key = ( NIL != entry ) ? (char*)entry->k : key;
            val = ( NIL != entry ) ? (struct json_object*)entry->v : val;
        }
    }

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_values() */


static signed int cfg_priv_json_parse_get_values_all(json_object* jsobj, char** ppBuffer_out, signed int* psiBufferSize, unsigned int uiNestingCounter)
{
    signed int          siResult   = 0;
    enum json_type      type;
    char*               key         = NIL;
    struct json_object* val         = NIL;
    struct lh_entry*    entry       = NIL;
    /* remember starting point */
    char*               pBufferTemp      = *ppBuffer_out;
    signed int          siBufferSizeTemp = *psiBufferSize;

    if( (++uiNestingCounter) >= MAX_NESTING_DEPTH )
    {
        /* set error */
        siResult = PAS_FAILURE;
        /* printf("cfg_priv_json_parse_get_values_all - MAX_NESTING_DEPTH\n"); */
    }

    if( NIL != json_object_get_object(jsobj) )
    {
        entry = json_object_get_object(jsobj)->head;
        key = ( NIL != entry ) ? (char*)entry->k : key;
        val = ( NIL != entry ) ? (struct json_object*)entry->v : val;

        while( (NIL != entry) && (NIL != key) && (NIL != val) && (0 <= siResult) )
        {
            size_t szKey = strlen(key) + 1;
            if( NIL != pBufferTemp  )
            {
                if( (siResult + (signed int)szKey) <= siBufferSizeTemp )
                {
                    /* get key */
                    (void)snprintf(*ppBuffer_out, (size_t)(*psiBufferSize), "%s%c", key, '\0');
                    *ppBuffer_out  += szKey;
                    *psiBufferSize -= szKey;

                    /* get key length */
                    siResult += (signed int)szKey;
                    /* (void)printf("key wanted = <%s>\n", key); */
                }
                else
                {
                    siResult = PAS_FAILURE_BUFFER_TOO_SMALL;
                }
            }
            else
            {
                /* get key length */
                siResult += (signed int)szKey;
                /* (void)printf("key = <%s>\n", key); */
            }

            type = json_object_get_type(val);
            if( json_type_object == type )
            {
                /* get value length */
                /* recursive */
                signed int sTempResult = cfg_priv_json_parse_get_values_all(json_object_object_get(jsobj, key), ppBuffer_out, psiBufferSize, uiNestingCounter);
                siResult = (sTempResult < 0) ? (sTempResult) : (siResult + sTempResult);
            }
            else
            {
                /* get value length */
                signed int sTempResult = cfg_priv_json_get_value(val, ppBuffer_out, psiBufferSize);
                siResult = (sTempResult < 0) ? (sTempResult) : (siResult + sTempResult);
            }

            /* move on to the next entry */
            entry = entry->next;
            key = ( NIL != entry ) ? (char*)entry->k : key;
            val = ( NIL != entry ) ? (struct json_object*)entry->v : val;
        }
    }

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_values_all() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static signed int cfg_priv_json_get_value(json_object *jsobj, char** ppBuffer_out, signed int* psiBufferSize)
{
    signed int          siResult = 0;
    enum json_type      type;
    /* remember starting point */
    char*               pBufferTemp      = *ppBuffer_out;
    signed int          siBufferSizeTemp = *psiBufferSize;

    /* get the type of the json object*/
    type = json_object_get_type(jsobj);

    switch( type )
    {
        case json_type_boolean:
        {
            siResult = PAS_FAILURE_OPERATION_NOT_SUPPORTED;
            (void)printf("NOT IMPLEMENTED - json_type_boolean\n");
            /*printf("value: %s \n", json_object_get_boolean(jsobj)? "true":"false");*/
            break;
        }
        case json_type_double:
        {
            siResult = PAS_FAILURE_OPERATION_NOT_SUPPORTED;
            (void)printf("NOT IMPLEMENTED - json_type_double\n");
            /*printf("value: %lf\n", json_object_get_double(jsobj)); */
            break;
        }
        case json_type_int:
        {
            siResult = PAS_FAILURE_OPERATION_NOT_SUPPORTED;
            (void)printf("NOT IMPLEMENTED - json_type_int\n");
            /* printf("value: %d\n", json_object_get_int(jsobj)); */
            break;
        }
        case json_type_string:
        {
            const char* sObjVal = json_object_get_string(jsobj);
            size_t szKey = strlen(sObjVal) + 1;

            if( NIL != pBufferTemp )
            {
                if( (signed int)szKey <= siBufferSizeTemp )
                {
                    /* get value */
                    (void)snprintf(*ppBuffer_out, (size_t)(*psiBufferSize), "%s%c", sObjVal, '\0');
                    *ppBuffer_out  += szKey;
                    *psiBufferSize -= szKey;

                    /* get value length */
                    siResult = (signed int)szKey;
                    /* (void)printf("value wanted = <%s>\n", sObjVal); */
                }
                else
                {
                    siResult = PAS_FAILURE_BUFFER_TOO_SMALL;
                }
            }
            else
            {
                /* get value length */
                siResult = (signed int)szKey;
                /* (void)printf("value = <%s>\n", sObjVal); */
            }

            break;
        }
        case json_type_array:
        {
            siResult = PAS_FAILURE_OPERATION_NOT_SUPPORTED;
            (void)printf("NOT IMPLEMENTED - json_type_array\n");
            break;
        }
        default:
        {
            siResult = PAS_FAILURE_OPERATION_NOT_SUPPORTED;
            (void)printf("NOT IMPLEMENTED\n");
            break;
        }
    }

    return siResult;

} /* cfg_priv_json_get_value() */


static signed int  cfg_priv_json_parse_get_keys_size(json_object* jsobj, char const * resourceID)
{
    char* pTemp = NIL;
    signed int siTempSize = UNDEFINED_SIZE;
    signed int siResult = cfg_priv_json_parse_get_keys(jsobj, resourceID, &pTemp, &siTempSize, 0);

    #ifdef VERBOSE_TRACE
    /* some info */
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
            DLT_STRING("cfg_priv_json_parse_get_keys_size"),
            DLT_STRING("-"),
            DLT_INT(siResult));
    #endif

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_keys_size() */


static signed int cfg_priv_json_parse_get_keys_all_size(json_object* jsobj)
{
    char* pTemp = NIL;
    signed int siTempSize = UNDEFINED_SIZE;
    signed int siResult = cfg_priv_json_parse_get_keys_all(jsobj, &pTemp, &siTempSize);

    #ifdef VERBOSE_TRACE
    /* some info */
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
            DLT_STRING("cfg_priv_json_parse_get_keys_all_size"),
            DLT_STRING("-"),
            DLT_INT(siResult));
    #endif

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_keys_all_size() */


static signed int cfg_priv_json_parse_get_values_size(json_object* jsobj, char const * resourceID)
{
    char* pTemp = NIL;
    signed int siTempSize = UNDEFINED_SIZE;
    signed int siResult = cfg_priv_json_parse_get_values(jsobj, resourceID, &pTemp, &siTempSize, 0);

    #ifdef VERBOSE_TRACE
    /* some info */
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR),
            DLT_STRING("cfg_priv_json_parse_get_values_size"),
            DLT_STRING("-"),
            DLT_INT(siResult));
    #endif

    /* return size */
    return siResult;

} /* cfg_priv_json_parse_get_values_size() */


#ifdef VERBOSE_TRACE
static void print_buffer(char* pBuffer, unsigned int uiSize)
{
    char*  ptemp = pBuffer;
    /* iterate through array and print keys and values */
    (void)printf("============================\n");
    (void)printf("size = %d\n", uiSize);
    while( ptemp < (pBuffer + uiSize) )
    {
       (void)printf("%s\n", ptemp);
       /* move on to the next */
       ptemp += (strlen(ptemp) + 1 /* \0 */);
    }
    (void)printf("============================\n");

} /* print_buffer() */
#endif


#if 0
void json_parse_array( json_object *jobj, char *key)
{
    enum json_type type;
    int arraylen = 0;
    int i;
    json_object * jvalue;
    json_object *jarray = jobj; /*Simply get the array*/

    if(key)
    {
        jarray = json_object_object_get(jobj, key); /*Getting the array if it is a key value pair*/
    }

    arraylen = json_object_array_length(jarray); /*Getting the length of the array*/
    printf("Array Length: %d \n",arraylen);

    for( i = 0; i< arraylen; i++ )
    {
        jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
        type = json_object_get_type(jvalue);
        if (type == json_type_array)
        {
          /* recursive */
          json_parse_array(jvalue, NIL);
        }
        else if (type != json_type_object)
        {
          printf("value[%d]: ",i);
          print_json_value(jvalue);
          printf("\n");
        }
        else
        {
            persadmin_priv_cfg_json_parse(jvalue);
        }
    }
}
#endif

