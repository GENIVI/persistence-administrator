#ifndef SSW_PERS_ADMIN_DBUS_H
#define SSW_PERS_ADMIN_DBUS_H

/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Interface: private - persistence admin service DBus access interface
*
* The file defines contains the defines according to
* https://collab.genivi.org/wiki/display/genivi/SysInfraEGPersistenceConceptInterface   
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author    Version  Reason
* 2013-09-17 uidu0250  1.0.0.0  CSP_WZ#5633:  Added persadmin_IsRegisteredToNSM function
* 2013-03-09 uidu0250  1.0.0.0	CSP_WZ#4480:  Added persadmin_QuitDBusMainLoop function
* 2013.04.04 uidu0250  1.0.0.0	CSP_WZ#2739:  Using PersCommonIPC for requests to PCL
* 2012.11.13 uidu0250  1.0.0.0  CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include "persComTypes.h"

/*
 * DBus identification of Persistence Administrator Service (PAS)
 */
#define PERSISTENCE_ADMIN_SVC_BUS_TYPE						G_BUS_TYPE_SYSTEM
#define PERSISTENCE_ADMIN_SVC_BUS_NAME						"org.genivi.persistence.admin_svc"
#define PERSISTENCE_ADMIN_SVC_OBJ_PATH						"/org/genivi/persistence/admin"


/**
 * \brief Initializes the DBus connection and enters the main DBus loop. Returns when the DBus connection is closed
 *
 * \return void
 */
void 	persadmin_RunDBusMainLoop(void);

/**
 * \brief Quits the main DBus loop
 *
 * \return void
 */
void 	persadmin_QuitDBusMainLoop(void);


/**
 * \brief Check if the registration to NSM was performed successfully.
 *
 * \return true if registered to NSM, false otherwise
 */
bool_t 	persadmin_IsRegisteredToNSM(void);

#ifdef __cplusplus
}
#endif

#endif /* SSW_PERS_ADMIN_DBUS_H */
