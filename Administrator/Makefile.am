#######################################################################################################################
#
# Copyright (C) 2012 Continental Automotive Systems, Inc.
#
# Author: Ana.Chisca@continental-corporation.com
#
# Makefile template for the Persistence Administration Service
#
# Process this file with automake to produce a Makefile.in.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#######################################################################################################################

lib_LTLIBRARIES = libpers_admin_access_lib.la
bin_PROGRAMS    = pers_admin_svc

#
#
# persistence administration access library.
#
#

include_HEADERS= \
                inc/public/persistence_admin_service.h
                
libpers_admin_access_lib_la_CFLAGS = \
                                -I inc/private -I inc/public \
                                $(PERSCOMMON_CFLAGS)
                                
libpers_admin_access_lib_la_SOURCES = \
                                src/ssw_pers_admin_access_lib.c

libpers_admin_access_lib_la_LDFLAGS = -version-info $(PERS_ADMINACCESSLIB_VERSION)

# pkgconfig
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/libpersadminaccess.pc

#
#
# persistence administration service.
#
#

pers_admin_svc_CFLAGS = \
                            -I inc/private -I inc/public -I generated \
                            $(DLT_CFLAGS)           \
                            $(GIO_CFLAGS)           \
                            $(GIO_UNIX_CFLAGS)      \
                            $(GLIB_CFLAGS)          \
                            $(GOBJECT_CFLAGS)       \
                            $(DBUS_CFLAGS)          \
                            $(ARCHIVELIB_CFLAGS)    \
                            $(ZLIB_CFLAGS)          \
                            $(PERSCOMMON_CFLAGS)    \
                            $(JSON_CFLAGS)          \
                            $(NSM_CFLAGS)           \
                            $(SYSTEMD_CFLAGS)

pers_admin_svc_LDFLAGS = \
                            $(PLATFORM_LDFLAGS)
  
pers_admin_svc_LDADD = \
                            $(DLT_LIBS)             \
                            $(GIO_LIBS)             \
                            $(GIO_UNIX_LIBS)        \
                            $(GLIB_LIBS)            \
                            $(DBUS_LIBS)            \
                            $(GOBJECT_LIBS)         \
                            $(ARCHIVELIB_LIBS)      \
                            $(ZLIB_LIBS)            \
                            $(PERSCOMMON_LIBS)      \
                            $(JSON_LIBS)            \
                            $(SYSTEMD_LIBS)

pers_admin_svc_SOURCES = \
                            generated/NodeStateConsumer.c               \
                            generated/NodeStateLifecycleConsumer.c      \
                            src/ssw_pers_admin_backup.c                 \
                            src/ssw_pers_admin_common.c                 \
                            src/ssw_pers_admin_config.c                 \
                            src/ssw_pers_admin_database_helper.c        \
                            src/ssw_pers_admin_data_export.c            \
                            src/ssw_pers_admin_data_import.c            \
                            src/ssw_pers_admin_dbus.c                   \
                            src/ssw_pers_admin_files_helper.c           \
                            src/ssw_pers_admin_pcl.c                    \
                            src/ssw_pers_admin_recovery.c               \
                            src/ssw_pers_admin_restore_default.c        \
                            src/ssw_pers_admin_resource_config_add.c    \
                            src/ssw_pers_admin_service.c                \
                            src/ssw_pers_admin_user_delete_data.c

systemdsystemunit_DATA = config/pas-daemon.service

dbuspolicy_DATA = dbus_config/org.genivi.persistence.admin_svc.conf

EXTRA_DIST = $(systemdsystemunit_DATA) \
             $(dbuspolicy_DATA)
