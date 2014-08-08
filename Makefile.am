SUBDIRS = source

lib_LTLIBRARIES=liblmapi.la
liblmapi_la_LDFLAGS=
liblmapi_la_SOURCES= \
	lm_api.c
liblmapi_la_DEPENDENCIES= \
	source/libCcspLMLite_source.la

liblmapi_la_LIBADD=$(liblmapi_la_DEPENDENCIES)
