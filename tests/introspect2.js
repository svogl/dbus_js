/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

LIB = "/opt/smarthome/lib/js/";

load(LIB + "common.js");
load(LIB + "shc_gui_client.js");


function Hello() {
	this._iface = "at.beko.Hello";
	this._sig =   <interface name="at.beko.Hello">
						<method name="Hello">
						  <arg name="who" type="s" direction="in"/>
						</method>
						<method name="Who">
						  <arg name="who" type="s" direction="out"/>
						</method>
					  </interface>;

	this.Hello = function(who) {
		print("Hello, " + who);
	}

	this.Who = function() {
		return "world";
	}
}

function Quit() {
	this._iface = "at.beko.Glib";
	this._sig =   <interface name={this._iface}>
						<method name="quit">
						</method>
				  </interface>;

	this.quit = function() {
		Glib.quit();
	}
}



bus = DBus.sessionBus();

intro = new Introspectable(bus);

hello = new Hello();
intro.expose("/foo", hello);

quit = new Quit();
intro.expose("/", quit);


print(bus._intro.Introspect("/"));

bus.requestBusName("at.beko.inspect");

Glib.mainloop();




