#######################################################################################################################
#
# Copyright (C) 2012 Continental Automotive Systems, Inc.
#
# Author: Ana.Chisca@continental-corporation.com
#
# Makefile template for the Persistence Administration Service generated sources
#
# Process this file with automake to produce a Makefile.in.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#######################################################################################################################

# List generated sources here to assert that they are available at build start
BUILT_SOURCES = NodeStateConsumer.c        \
                NodeStateConsumer.h        \
                NodeStateLifecycleConsumer.c  \
                NodeStateLifecycleConsumer.h

# Delete all generated files on 'make clean'
CLEANFILES    = NodeStateConsumer.c        \
                NodeStateConsumer.h        \
                NodeStateLifecycleConsumer.c  \
                NodeStateLifecycleConsumer.h
                

# Targets to create generated sources during the build
NodeStateConsumer.c NodeStateConsumer.h: $(nsmdbusinterfacesdir)/org.genivi.NodeStateManager.Consumer.xml
	gdbus-codegen --interface org.genivi.NodeStateManager. --c-namespace NodeState --generate-c-code NodeStateConsumer $(nsmdbusinterfacesdir)/org.genivi.NodeStateManager.Consumer.xml

NodeStateLifecycleConsumer.c NodeStateLifecycleConsumer.h: $(nsmdbusinterfacesdir)/org.genivi.NodeStateManager.LifecycleConsumer.xml
	gdbus-codegen --interface org.genivi.NodeStateManager.LifeCycle --c-namespace NodeStateLifeCycle --generate-c-code NodeStateLifecycleConsumer $(nsmdbusinterfacesdir)/org.genivi.NodeStateManager.LifecycleConsumer.xml