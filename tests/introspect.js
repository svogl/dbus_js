/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

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
		this.test();
		//ret = "";
		ret = <node/>;
		try {
			print("INTROSPECT --> me " + this);
			print("INTROSPECT --> iface " + this._iface);
			print("INTROSPECT --> bus " + this._bus);
	//return "<node/>";
			if (this._bus.keys == null) {
				print("INTROSPECT --> keys null" );
			} else {
				print("INTROSPECT --> keys not null" );
			}
			print("INTROSPECT -->" + this._bus.keys);
			print("INTROSPECT -->" + this._bus.keys);
			print("INTROSPECT -->" + this._bus.keys);
			for each (key in this._bus.keys) {
				//n = <node/>;
				//n.name = key;
				//ret += n;
				print("k " + key);
			}
			print("INTROSPECT ::::: " + ret + " ::::::");
		} catch (e) {
		print("CAUGHT AN " + e);
		return "<node/>";
		}
		return ""+ret;
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
					path[comp]=d;
				}
				path = path[comp];
			//} else {
				// we're at the end of a path ending with slash ../../ or at root
				// we hope for the latter.
				// simply continue
			}
		}
		path.services[service._iface] = service;
		this._bus.keys[ oPath + "_" + service._iface ] = service;
	}

	this.test = function() {
		try {
			print("PECT --> me " + this);
			print("PECT --> iface " + this._iface);
			print("PECT --> bus " + this._bus);
			print("PECT -->" + this._bus.keys);
			if (this._bus.keys == null) {
				print("PECT --> keys null" );
			} else {
				print("PECT --> keys not null" );
				for each (key in this._bus.keys) {
					print("    k " + key + " " + this._bus.keys[key]);
				}
			}
			print("PECT ::::: " + ret + " ::::::");
		} catch (e) {
			print("CAUGHT AN " + e);
		}
	}
}

bus = DBus.sessionBus();
print("KEYS::: " + bus.keys);

intro = new Introspectable(bus);

bus._intro = intro;

intro.expose("/", intro);
intro.expose("/foobar", intro);


bus.export("/", intro._iface, intro); 

intro.test();
bus._intro.test();

bus.requestBusName("at.beko.inspect");

Glib.mainloop();

print(intro._sig);

s1=<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <node name="com"/>
</node>;

print("***************s1\n"+s1);

/*





method call sender=:1.94 -> dest=com.Skype.API serial=144 path=/; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=144
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <node name="com"/>
</node>
"
method call sender=:1.94 -> dest=com.Skype.API serial=145 path=/; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=145
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <node name="com"/>
</node>
"
method call sender=:1.94 -> dest=com.Skype.API serial=146 path=/com; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=146
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <node name="Skype"/>
</node>
"
method call sender=:1.94 -> dest=com.Skype.API serial=147 path=/com; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=147
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <node name="Skype"/>
</node>
"
method call sender=:1.94 -> dest=com.Skype.API serial=148 path=/com/Skype; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=148
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="com.Skype.API">
    <method name="Invoke">
      <arg type="s" direction="out"/>
      <arg name="request" type="s" direction="in"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="Set">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="in"/>
    </method>
    <method name="GetAll">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="values" type="a{sv}" direction="out"/>
      <annotation name="com.trolltech.QtDBus.QtTypeName.Out0" value="QVariantMap"/>    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
</node>
"
method call sender=:1.94 -> dest=com.Skype.API serial=149 path=/com/Skype; interface=org.freedesktop.DBus.Introspectable; member=Introspect
method return sender=:1.64 -> dest=:1.94 reply_serial=149
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="com.Skype.API">
    <method name="Invoke">
      <arg type="s" direction="out"/>
      <arg name="request" type="s" direction="in"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="Set">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="in"/>
    </method>
    <method name="GetAll">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="values" type="a{sv}" direction="out"/>
      <annotation name="com.trolltech.QtDBus.QtTypeName.Out0" value="QVariantMap"/>    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
</node>
"
signal sender=org.freedesktop.DBus -> dest=(null destination) serial=8 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=NameOwnerChanged
   string ":1.93"
   string ":1.93"
   string ""
signal sender=org.freedesktop.DBus -> dest=(null destination) serial=9 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=NameOwnerChanged
   string ":1.94"
   string ":1.94"
   string ""
signal sender=org.freedesktop.DBus -> dest=(null destination) serial=10 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=NameOwnerChanged
   string "org.freedesktop.ReserveDevice1.Audio0"
   string ""
   string ":1.0"
method call sender=:1.0 -> dest=org.freedesktop.DBus serial=27 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=RequestName
   string "org.freedesktop.ReserveDevice1.Audio0"
   uint32 5
signal sender=org.freedesktop.DBus -> dest=(null destination) serial=11 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=NameOwnerChanged
   string "org.freedesktop.ReserveDevice1.Audio0"
   string ":1.0"
   string ""
method call sender=:1.0 -> dest=org.freedesktop.DBus serial=28 path=/org/freedesktop/DBus; interface=org.freedesktop.DBus; member=ReleaseName
   string "org.freedesktop.ReserveDevice1.Audio0"

*/


