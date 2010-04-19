/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

bus = DBus.sessionBus();

bus.requestBusName("at.beko.Listener");

Glib.mainloop();
