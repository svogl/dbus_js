/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

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


function DirInfo() {
	this.name=""; 
	this.dirs = new Object();
	this.services = new Object();
}

function Introspectable(bus) {
	this._bus = bus;
	this._iface = "org.freedesktop.DBus.Introspectable";
	this._sig =   <interface name="org.freedesktop.DBus.Introspectable">
						<method name="Introspect">
						  <arg name="xml_data" type="s" direction="out"/>
						</method>
					  </interface>;

	this.root = new DirInfo();

	this.Introspect = function(path) {
		print("INTROSPECT --> " + this);

		head = '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">\n';
		ret = <node/>;

		dir = this.root; 
		p = path.split("/");
		for each (comp in p) {
			if (comp != "") {
				if ( dir.dirs[comp] != null ) {
					dir = dir.dirs[comp];
				} else {
					print("path component not found" + comp);
					return head+ret;
				}
			}
		}
		try {
			for each (srv in dir.services) {
				ret.node += srv._sig;
			}
			for each (d in dir.dirs) {
				n = <node name={d.name} />;
				ret.node += n;
			}
		} catch (e) {
			print("CAUGHT AN " + e);
		}
		return head+ret.toXMLString();
	}

	this.expose = function(oPath, service) {
		//enter into dir, pass along
		p = oPath.split("/");
		part = "";
		path = this.root;

		for each (comp in p) {
			if (comp != "") {
				// named path element. go down or add
				if ( path[comp] == null) {
					d = new DirInfo();
					d.name=comp;
					path.dirs[comp]=d;

					// auto-export introspection:
					part += "/"+comp;
					d.services[this._iface] = this; // add introspection
					this._bus.export(part, this._iface, this); 
				}
				path = path.dirs[comp];
			}
		}
		path.services[service._iface] = service;
		this._bus.export(oPath, service._iface, service); 
	}

	// make introspection accessible by default:
	this.expose("/", this);
	this._bus._intro = this;
}

bus = DBus.sessionBus();

intro = new Introspectable(bus);

//bus._intro = intro;
//intro.expose("/", intro);

hello = new Hello();
intro.expose("/foo", hello);

quit = new Quit();
intro.expose("/", quit);


print(bus._intro.Introspect("/"));

bus.requestBusName("at.beko.inspect");

Glib.mainloop();




