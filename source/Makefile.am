AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_LDFLAGS = -lccsp_common

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu
bin_PROGRAMS = CcspLMLite
CcspLMLite_CPPFLAGS = $(CPPFLAGS)
CcspLMLite_SOURCES = lm_wrapper_priv_pc.c lm_api_test.c lm_main.c lm_wrapper.c lm_util.c
CcspLMLite_LDFLAGS =  -lccsp_common -llmapi
