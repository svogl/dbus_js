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
	print("init bus" + bus );
	this._bus = bus;
	this._iface = "org.freedesktop.DBus.Introspectable";
	this._sig =   <interface name="org.freedesktop.DBus.Introspectable">
						<method name="Introspect">
						  <arg name="xml_data" type="s" direction="out"/>
						</method>
					  </interface>;

	this.root = new DirInfo();

	this.Introspect = function() {
		print("INTROSPECT -->");

		head = '<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">';
		ret = <node/>;
		try {
			for each (srv in this.root.services) {
				ret.node += srv._sig;
			}
			for each (dir in this.root.dirs) {
				n = <node name={dir.name} />;
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
		path = this.root;
print("path "+path);
		for each (comp in p) {
			if (comp != "") {
				// named path element. go down or add
				if ( path[comp] == null) {
					d = new DirInfo();
					d.name=comp;
					d.services[this._bus._intro._iface] = this._bus._intro; // add introspection
					path.dirs[comp]=d;
				}
				path = path.dirs[comp];
			//} else {
				// we're at the end of a path ending with slash ../../ or at root
				// we hope for the latter.
				// simply continue
			}
		}
print("p|| " +path.name);
		path.services[service._iface] = service;
		this._bus.keys[ oPath + "_" + service._iface ] = service;
	}

	this.test = function() {
	}
}

bus = DBus.sessionBus();
print("KEYS::: " + bus.keys);

intro = new Introspectable(bus);

bus._intro = intro;
intro.expose("/", intro);
bus.export("/", intro._iface, intro); 

hello = new Hello();
intro.expose("/", hello);

quit = new Quit();
intro.expose("/", quit);

print(bus._intro.Introspect());

bus.requestBusName("at.beko.inspect");

Glib.mainloop();




