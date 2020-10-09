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

ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libCcspLM.la

libCcspLM_la_CPPFLAGS = -I$(top_srcdir)/source/Ssp

libCcspLM_la_SOURCES = 	cosa_hosts_dml.c \
						plugin_main.c \
						lm_wrapper_priv.c \
						lm_api_test.c \
						lm_main.c \
						lm_wrapper.c \ 
						lm_util.c \
						network_devices_status_avropack.c  \
						webpa_interface.c \
                                                webpa_pd_with_seshat.c \
                                                webpa_pd_without_seshat.c \
						cosa_ndstatus_dml.c \ 
						network_devices_status.c \
						cosa_reports_internal.c \
						network_devices_traffic_avropack.c  \
						cosa_ndtraffic_dml.c \ 
						network_devices_traffic.c \
						cosa_xhosts_dml.c \
                        network_devices_interface.c \
                        device_presence_detection.c \
                        cosa_managementserver_apis.c \
                        cosa_managementserver_dml.c
                        

libCcspLM_la_LDFLAGS = -lccsp_common -lsyscfg -lsysevent -lutapi -lutctx -lulog -lavro -lmsgpackc -ltrower-base64 -luuid -lm -lrt -pthread -lnanomsg -lcimplog -lwrp-c -llibparodus -lsecure_wrapper
if ENABLE_SESHAT
libCcspLM_la_LDFLAGS += -llibseshat
endif
