SUBDIRS = source

lib_LTLIBRARIES=liblmapi.la
liblmapi_la_CPPFLAGS= -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ 
liblmapi_la_LDFLAGS= -lccsp_common
liblmapi_la_SOURCES= \
	source/lm_api.c
liblmapi_la_DEPENDENCIES=
liblmapi_la_LIBADD=$(liblmapi_la_DEPENDENCIES)
