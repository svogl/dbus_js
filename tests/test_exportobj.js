/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

bus = DBus.sessionBus();

function Serv(bus) {
	this._bus = bus;
	this._iface = "at.beko.ListenerIface";
	this._sig = <interface name="org.freedesktop.DBus.Properties">
					<method name="Get">
					  <arg name="interface" direction="in" type="s"/>
					  <arg name="propname" direction="in" type="s"/>
					  <arg name="value" direction="out" type="v"/>
					</method>
					<method name="Set">
					  <arg name="interface" direction="in" type="s"/>
					  <arg name="propname" direction="in" type="s"/>
					  <arg name="value" direction="in" type="v"/>
					</method>
					<method name="GetAll">
					  <arg name="interface" direction="in" type="s"/>
					  <arg name="props" direction="out" type="a{sv}"/>
					</method>
				  </interface>
				;
	this.exported = ["func1", "func2"];

	this.props = new Object();

	function Get(arg) {
		print("GET " +arg);
		return this.props[arg];
	}

	function Set(arg, val) {
		this.props[arg] = val;
		print("SET " +arg+" to " + val);
	}

	function GetAll() {
		ret = new Array();
		for each (key in this.props) {
			ret += key;
		}
		print("GETALL " +ret);
		return ret;
	}
}

service = new Serv(bus);

print(service._sig);

bus.export("/dada", "at.beko.ListenerIface", service);


bus.requestBusName("at.beko.Listener");

Glib.mainloop();

