/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

LIB = "/opt/smarthome/lib/js/";

load(LIB + "common.js");
load(LIB + "shc_gui_client.js");

bus = DBus.sessionBus();

bus.requestBusName("at.beko.Test");


function Client(bus) {
	this._bus = bus;
	this._path = "/dBusListener";
	this._iface = "at.beko.TestIF";
	this._sig =    <interface name="at.beko.smarthome.GuiControllerClient.Interface">
			  <annotation name="org.freedesktop.dbus.DBusInterfaceName" value="at.beko.smarthome.GuiControllerClient.Interface" />
				  <method name="quit" >
					   <arg type="v" direction="in"/>
					   <arg type="i" direction="out"/>
				  </method>
				  <method name="play" >
					   <arg type="v" direction="in"/>
					   <arg type="i" direction="out"/>
				  </method>
				  <method name="pause" >
					   <arg type="v" direction="in"/>
					   <arg type="i" direction="out"/>
				  </method>
			  <signal name="applicationFinished">
			  </signal>
			 </interface>;

	this.play = function(arg) {
		print("JS::Client::play() " + arg);
		return 112358;
	}
	this.pause = function(arg) {
		print("JS::Client::pause() " + arg);
		return 2468;
	}

	this.quit = function(arg) {
		print("JS::Client::quit() " + arg);
		Glib.quit();
		return 4321;
	}
}

guic = new GuiClient(bus);
intro.export(guic._path, guic);

Glib.mainloop();

