#######################################################################################################################
#
# Copyright (C) 2012 Continental Automotive Systems, Inc.
#
# Author: Ana.Chisca@continental-corporation.com
#
# Makefile template for the Persistence Administration Service Test Framework
#
# Process this file with automake to produce a Makefile.in.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#######################################################################################################################

bin_PROGRAMS = pers_admin_test_framework

#
#
# persistence administration service test framework.
#
#

pers_admin_test_framework_CFLAGS = \
                            -I $(top_srcdir)/Administrator/inc/private -I $(top_srcdir)/Administrator/inc/public -I inc -I generated \
                            $(DLT_CFLAGS)           \
                            $(GIO_CFLAGS)           \
                            $(GIO_UNIX_CFLAGS)      \
                            $(GLIB_CFLAGS)          \
                            $(GOBJECT_CFLAGS)       \
                            $(DBUS_CFLAGS)          \
                            $(PERSCOMMON_CFLAGS)    \
                            $(ARCHIVELIB_CFLAGS)

pers_admin_test_framework_LDFLAGS = \
                            $(PLATFORM_LDFLAGS)
  
pers_admin_test_framework_LDADD = \
                            $(DLT_LIBS)             \
                            $(GIO_LIBS)             \
                            $(GIO_UNIX_LIBS)        \
                            $(GLIB_LIBS)            \
                            $(DBUS_LIBS)            \
                            $(GOBJECT_LIBS)         \
                            $(PERSCOMMON_LIBS)      \
                            $(ARCHIVELIB_LIBS)      \
                            $(top_srcdir)/Administrator/libpers_admin_access_lib.la

pers_admin_test_framework_SOURCES = \
                            $(top_srcdir)/Administrator/src/ssw_pers_admin_files_helper.c        \
                            generated/PasModuleTestGen.c                                         \
                            src/test_PAS.c                                                       \
                            src/test_pas_check_data_after_backup_application.c                   \
                            src/test_pas_check_data_after_backup_create_all.c                    \
                            src/test_pas_check_data_after_backup_user_all.c                      \
                            src/test_pas_check_data_after_backup_user2_seat_all.c                \
                            src/test_pas_check_data_after_delete_user2_data.c                    \
                            src/test_pas_check_data_after_reset.c                                \
                            src/test_PAS_dbus.c                                                  \
                            src/test_pas_import_all.c                                            \
                            src/test_pas_import_app.c                                            \
                            src/test_pas_import_source_content.c                                 \
                            src/test_pas_import_user.c                                           \
                            src/test_pas_recovery_All.c                                          \
                            src/test_pas_recovery_AllInitialContent.c                            \
                            src/test_pas_recovery_App1.c                                         \
                            src/test_pas_recovery_App1InitialContent.c                           \
                            src/test_pas_recovery_backup_content.c                               \
                            src/test_pas_recovery_User1.c                                        \
                            src/test_pas_recovery_User1InitialContent.c                          \
                            src/test_pas_recovery_User2Seat1InitialContent.c                     \
                            src/test_pas_recovery_Users.c                                        \
                            src/test_pas_resource_config_add.c                                   \
                            src/test_pas_restore_default_All.c                                   \
                            src/test_pas_restore_default_App1.c                                  \
                            src/test_pas_restore_default_User1.c                                 \
                            src/test_pas_restore_default_User2App1.c                             \
                            src/test_pas_restore_default_User2Seat1.c
