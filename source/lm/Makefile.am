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
AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
if CCSP_ARCH_ARM
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

if CCSP_ARCH_ATOM
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

if CCSP_ARCH_PC
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
endif

AM_LDFLAGS = -lccsp_common
AM_LDFLAGS += -lsysevent
AM_LDFLAGS += -lutapi
AM_LDFLAGS += -lutctx
AM_LDFLAGS += -lulog

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu
noinst_LTLIBRARIES = libCcspLM.la
libCcspLM_la_CPPFLAGS = -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/custom -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/include -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/linux -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/package/slap/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/http/include -I$(top_srcdir)/../CcspCommonLibrary/source/util_api/ansc/include -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/common/MessageBusHelper/include -I$(top_srcdir)/../hal/inlcude -I$(top_srcdir)/source/Ssp $(CPPFLAGS)
libCcspLM_la_SOURCES = 	cosa_hosts_dml.c \
						plugin_main.c \
						lm_wrapper_priv.c \
						lm_api_test.c \
						lm_main.c \
						lm_wrapper.c \ 
						lm_util.c \
						network_devices_status_avropack.c  \
						webpa_interface.c \
						cosa_ndstatus_dml.c \ 
						network_devices_status.c \
						cosa_reports_internal.c \
						network_devices_traffic_avropack.c  \
						cosa_ndtraffic_dml.c \ 
						network_devices_traffic.c


libCcspLM_la_LDFLAGS =  -lccsp_common -lsysevent -lutapi -lutctx -lulog -lavro -lmsgpack -lbase64 -luuid


