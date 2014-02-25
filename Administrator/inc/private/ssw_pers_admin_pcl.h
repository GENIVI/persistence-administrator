#ifndef SSW_PERS_ADMIN_PCL_H
#define SSW_PERS_ADMIN_PCL_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Interface: private - persistence admin service PCL access interface (lock/unlock/sync)
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013.04.04 uidu0250  1.0.0.0	CSP_WZ#2739:  Using PersCommonIPC for requests to PCL
* 2012.11.13 uidu0250  1.0.0.0  CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include "persComTypes.h"



/**
 * \brief Init the PAS IPC module
 *
 * \return 0 for success, negative value for error (see persComErrors.h)
 */
sint_t		persadmin_InitIpc(void);



/**
 * \brief Request the PCL to sync and lock the memory access
 *
 * \return 0 for success, negative value for error (see persComErrors.h)
 */
sint_t		persadmin_SendLockAndSyncRequestToPCL(void);



/**
 * \brief Request the PCL to unlock the memory access
 *
 * \return 0 for success, negative value for error (see persComErrors.h)
 */
sint_t		persadmin_SendUnlockRequestToPCL(void);



/**
 * \brief Check if the memory access is already locked
 *
 * \return true if the memory access is locked, false otherwise
 */
bool_t		persadmin_IsPCLAccessLocked(void);



#ifdef __cplusplus
}
#endif

#endif /* SSW_PERS_ADMIN_DBUS_H */
