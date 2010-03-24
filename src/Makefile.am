lib_LTLIBRARIES=libdbus_js.la 

AM_CPPFLAGS=-Wall -I. -DXP_UNIX -DOSSP -DEXPORT_JS_API -DJS_HAS_FILE_OBJECT -DHAVE_CONFIG_H

libbluez_js_la_SOURCES = bluez_js.c
libbluez_js_la_CFLAGS  = $(GTK_CFLAGS) $(JS_CFLAGS) $(bluez_CFLAGS)
libbluez_js_la_LDFLAGS = $(GTK_LIBS) $(JS_LIBS) $(bluez_LIBS)

libdbus_js_la_SOURCES = glib_js.cpp  dbus_js.cpp marshal.cpp
###MozJSDBusMarshalling.cpp
libdbus_js_la_CFLAGS  = $(GLIB_CFLAGS) $(JS_CFLAGS) $(DBUS_CFLAGS)
libdbus_js_la_CXXFLAGS  = $(GLIB_CFLAGS) $(JS_CFLAGS) $(DBUS_CFLAGS)
libdbus_js_la_LDFLAGS = $(GLIB_LIBS) $(JS_LIBS) $(DBUS_LIBS) -ldbus-glib-1

noinst_HEADERS =  marshal.h

