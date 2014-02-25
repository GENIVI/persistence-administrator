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

bin_PROGRAMS = persadmin_tool

#
#
# persistence administration service tool for development
#
#

persadmin_tool_CFLAGS = \
                            -I $(top_srcdir)/Administrator/inc/private -I $(top_srcdir)/Administrator/inc/public -I inc -I generated \
                            $(DLT_CFLAGS)           \
                            $(PERSCOMMON_CFLAGS)    

persadmin_tool_LDFLAGS = \
                            $(PLATFORM_LDFLAGS)
  
persadmin_tool_LDADD = \
                            $(DLT_LIBS)             \
                            $(top_srcdir)/Administrator/libpers_admin_access_lib.la

persadmin_tool_SOURCES = \
                            src/persadmin_tool.c
