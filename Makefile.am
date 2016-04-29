##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
if CCSP_ARCH_PC
SUBDIRS = source-pc
else
SUBDIRS = source
endif

lib_LTLIBRARIES=liblmapi.la
liblmapi_la_CPPFLAGS= -D_ANSC_LINUX -D_ANSC_USER -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/custom -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/include -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/linux -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/slap/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/http/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/MessageBusHelper/include -I$(top_srcdir)/../hal/include

if CCSP_ARCH_ARM
liblmapi_la_CPPFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

if CCSP_ARCH_ATOM
liblmapi_la_CPPFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

if CCSP_ARCH_PC
liblmapi_la_CPPFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

liblmapi_la_LDFLAGS= -lccsp_common

if CCSP_ARCH_PC
liblmapi_la_SOURCES= ${SUBDIRS}/lm_api.c
else
liblmapi_la_SOURCES= ${SUBDIRS}/lm_api.c
endif
liblmapi_la_DEPENDENCIES=
liblmapi_la_LIBADD=$(liblmapi_la_DEPENDENCIES)
