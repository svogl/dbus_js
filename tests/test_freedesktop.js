/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

bus = DBus.sessionBus();

ping = function(s1) {
	bus.setName(s1);
	print("PING "+s1);
}

bus.addSigHandler("","/org/freedesktop/DBus","org.freedesktop.DBus","NameAcquired", ping);

////org/freedesktop/DBus/org.freedesktop.DBus/NameAcquired

//bus.requestBusName("at.beko.Listener");

Glib.mainloop();
