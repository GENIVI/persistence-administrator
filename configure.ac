#######################################################################################################################
#
# Copyright (C) 2012 Continental Automotive Systems, Inc.
#
# Author: cosmin.cernat@continental-corporation.com
#
# Configure template for the Persistence Administration Service
#
# Process this file with autoconf to produce a configure script.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#######################################################################################################################

dnl **************************************************************************
dnl *** First, define all of the version numbers up front                  ***
dnl *** In particular, this allows the version macro to be used in AC_INIT ***
dnl **************************************************************************
m4_define([PERS_PACKAGE_VERSION_S],[1.0.8])
m4_define([PERS_ADMINACCESSLIB_VERSION_N],[1000000])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_INIT([persistence-administration-service],[PERS_PACKAGE_VERSION_S])
AC_COPYRIGHT([Copyright (c) 2012 Continental Automotive GmbH])
AC_PREREQ([2.50])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], ["$ACLOCAL_FLAGS -I m4"])

dnl *********************************
dnl *** Shared library versioning ***
dnl *********************************
PERS_ADMINACCESSLIB_CURRENT=$((((PERS_ADMINACCESSLIB_VERSION_N() / 1000) / 1000) % 1000 ))
PERS_ADMINACCESSLIB_REVISION=$(( (PERS_ADMINACCESSLIB_VERSION_N() / 1000) % 1000 ))
PERS_ADMINACCESSLIB_AGE=$((  PERS_ADMINACCESSLIB_VERSION_N() % 1000 ))
PERS_ADMINACCESSLIB_VERSION=$PERS_ADMINACCESSLIB_CURRENT:$PERS_ADMINACCESSLIB_REVISION:$PERS_ADMINACCESSLIB_AGE
AC_SUBST(PERS_ADMINACCESSLIB_VERSION)

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_CXX()
AC_PROG_INSTALL()

dnl ***************************
dnl *** Initialize lib tool ***
dnl ***************************
AC_DISABLE_STATIC()
AC_PROG_LIBTOOL()

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([sys/file.h])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([memcpy])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([munmap])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strrchr])

AC_CHECK_FUNCS([ftruncate])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strtol])

dnl ********************************************************************
dnl *** Check for typedefs, structures, and compiler characteristics ***
dnl ********************************************************************
AC_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.30.0]) 		
PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0 >= 2.30.0]) 
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.30.0]) 	
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.30.0]) 	
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.4.10])		
PKG_CHECK_MODULES([DLT], [automotive-dlt >= 2.2.0])	
PKG_CHECK_MODULES([ZLIB], [zlib >= 1.2.5])		
PKG_CHECK_MODULES([ARCHIVELIB], [libarchive >= 3.0.4])	
PKG_CHECK_MODULES([PERSCOMMON], [libperscommon >= 1.0.1])
PKG_CHECK_MODULES(JSON, [ json-c >= 0.11 ], [],
       [PKG_CHECK_MODULES(JSON, [ json >= 0.9 ])])
PKG_CHECK_MODULES([NSM], [node-state-manager])
PKG_CHECK_MODULES(SYSTEMD, [libsystemd >= 209 ], [],
       [PKG_CHECK_MODULES(SYSTEMD, [ libsystemd-daemon >= 37 ])])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl *************************************
dnl *** Define extra paths            ***
dnl *************************************
AC_ARG_WITH([systemdsystemunitdir],
        AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
        [],
        [with_systemdsystemunitdir=$(pkg-config --silence-errors --variable=systemdsystemunitdir systemd)])
AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
AC_ARG_WITH([dbuspolicydir],
        AS_HELP_STRING([--with-dbuspolicydirdir=DIR], [Directory for D-Bus system policy files]),
        [],
        [with_dbuspolicydir=$(pkg-config --silence-errors --variable=sysconfdir dbus-1)/dbus-1/system.d])
AC_SUBST([dbuspolicydir], [$with_dbuspolicydir])

# Derive path of the NSM dbus xml models
AC_SUBST([nsmdbusinterfacesdir], [$(pkg-config --silence-errors --variable=dbusinterfacesdir node-state-manager)])

dnl ************************************
dnl *** Define configure output      ***
dnl ************************************
AC_CONFIG_FILES([   Makefile 
                    Administrator/Makefile 
                    Administrator/generated/Makefile
                    Administrator/config/pas-daemon.service 
                    Administrator/pkgconfig/libpersadminaccess.pc 
                    test/pers_svc_test/Makefile 
                    test/persadmin_tool/Makefile])
AC_OUTPUT

