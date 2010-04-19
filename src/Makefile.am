lib_LTLIBRARIES=libdbus_js.la 

AM_CPPFLAGS=-Wall -I. -DXP_UNIX -DOSSP -DEXPORT_JS_API -DJS_HAS_FILE_OBJECT -DHAVE_CONFIG_H

libdbus_js_la_SOURCES = glib_js.cpp  dbus_js.cpp marshal.cpp
# DbusIntrospect.cpp
libdbus_js_la_CFLAGS  = $(XULU_CFLAGS) $(GLIB_CFLAGS) $(JS_CFLAGS) $(DBUS_CFLAGS)
libdbus_js_la_CXXFLAGS  = $(XULU_CFLAGS) $(GLIB_CFLAGS) $(JS_CFLAGS) $(DBUS_CFLAGS)
libdbus_js_la_LDFLAGS = $(XULU_LIBS) $(GLIB_LIBS) $(JS_LIBS) $(DBUS_LIBS) -ldbus-glib-1

noinst_HEADERS =  marshal.h
# DbusIntrospect.h

