/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

LIB = "/opt/smarthome/lib/js/";
load(LIB + "common.js");

bus = DBus.sessionBus();

bus.requestBusName("at.beko.Test");

Glib.mainloop();
