AM_CFLAGS = 

AM_CPPFLAGS = -Wall -Werror

ACLOCAL_AMFLAGS = -I m4

#hardware_platform = $(MACHINE)
hardware_platform = i686-linux-gnu

lib_LTLIBRARIES = liblmapi.la
#bin_PROGRAMS =

liblmapi_ladir = $(top_builddir)/source/.libs

liblmapi_la_CPPFLAGS = -I$(top_builddir)/include $(CPPFLAGS)

liblmapi_la_SOURCES = lm_api.c lm_api_test.c lm_main.c lm_util.c lm_wrapper.c
liblmapi_la_SOURCES_pc = $(liblmapi_la_SOURCES) lm_wrapper_priv_pc.c
liblmapi_la_SOURCES_arm =  $(liblmapi_la_SOURCES) lm_wrapper_priv_arm.c

liblmapi_la_LIBADD = 
