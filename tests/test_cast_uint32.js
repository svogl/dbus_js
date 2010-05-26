/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

LIB = "/opt/smarthome/lib/js/";
load(LIB + "common.js");

bus = DBus.sessionBus();

//bus.call("at.beko.smarthome.AudioController.Service", "/control", "org.freedesktop.DBus.Peer", "Ping");

//ret = bus.call("at.beko.smarthome.AudioController.Service", "/control", "at.beko.smarthome.AudioController.Interface", "toggleMasterVolumeMute");
//print("MUTE " + ret);

vol = new DBusTypeCast();
vol.type = "UInt32";
vol.value = 20;

for (i in [1,2,3,4,5]) {
	vol.value = 10*i;
	ret = bus.call("at.beko.smarthome.AudioController.Service", "/control", "at.beko.smarthome.AudioController.Interface", "setMasterVolume", vol );
}
print("returned " + ret);

