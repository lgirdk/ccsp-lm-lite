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
#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

#
#   for Intel USG, Pumba 6, all ARM boards
#   LMLite custom makefile, included pre main makefile
#

#
#   component list
#


#
#   platform specific customization
#

include $(SDK_PATH)/.config

LDFLAGS += -L$(SDK_PATH)/ti/netdk/src/uipc

LDFLAGS += $(ldflags-y) -luipc -lpthread -lapi_dhcpv4c

# SDK_VERSION comes from intel_usg

ifeq ($(SDK_VERSION),)
SDK_VERSION = 4.2

$(info ***  SDK_VERSION not set ***)
endif

sdk_version := v$(SDK_VERSION)
ifneq (,$(findstring v4.2,$(sdk_version)))
# v4.2 and below
$(info *** v4.2 and below ***)
   LDFLAGS += -lswctl
else
# intel SDK v4.3 and above
$(info *** intel SDK v4.3 and above ***)
   LDFLAGS += -L$(SDK_PATH)/ti/netdk/src/ti_udhcp
   LDFLAGS += -L$(SDK_PATH)/ti/netdk/src/ti_dhcpv6
   LDFLAGS += -ldhcp4cApi
endif


UTOPIA_LDFLAGS = -lutapi -lutctx -lsyscfg -lsysevent -lulog
LDFLAGS += $(UTOPIA_LDFLAGS)

ifeq ($(CONFIG_SYSTEM_MOCA), y)
   LDFLAGS += -lmoca_mgnt
   CFLAGS += -DCONFIG_SYSTEM_MOCA
endif
