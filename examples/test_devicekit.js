/* This is a simple example that listens for events from the disk subsystem.
   
 */
DSO.load("dbus"); 


bus = DBus.systemBus();

function Disk(bus, oPath) {
	this.bus = bus;
	this.oPath = oPath;
}


function DeviceKit(bus) {
	this.bus = bus;
	this.service = "org.freedesktop.DeviceKit.Disks";
	this.iFace = "org.freedesktop.DeviceKit.Disks";
	this.oPath = "/org/freedesktop/DeviceKit/Disks";

	this.disks = {};

	this.devAdd = function(opath) {
		print("JS::: added device " + opath);
		disks[opath] = Disk(this.bus, opath);
	};

	this.devRem = function (opath) {
		print("JS::: removed device " + opath);
		Glib.quit();
	},

	this.devChg = function(opath) {
		print("JS::: changed device " + opath);
	}
	this.enumDevices = function() {
		ret = this.bus.call( this.service, this.oPath, this.iFace, "EnumerateDevices");
		return ret;
	}

	this.enumDeviceFiles = function() {
		ret = this.bus.call( this.service, this.oPath, this.iFace, "EnumerateDeviceFiles");
		return ret;
	}
	print(bus);

	// register signal handlers:
	val = this.bus.addSigHandler( this.service, this.oPath, this.iFace, "DeviceAdded", this.devAdd);
	val = this.bus.addSigHandler( this.service, this.oPath, this.iFace, "DeviceRemoved", this.devRem);
	val = this.bus.addSigHandler( this.service, this.oPath, this.iFace, "DeviceChanged", this.devChg);
};



devKit = new DeviceKit(bus);

print(">>>>>>>> devKit Properties...");
for (var i in devKit)
{
	print(">>> "+ i);
}
print(">>>>>>>> devKit Properties.");


if (false) {

print("---\n\n");
for(var i=0; i < devKit.length; i++ ){
	print("-> " + devKit[i] );
}
print("---\n\n");

print("--- EnumerateDevices\n\n");
ret = devKit.enumDevices();
for(var i=0; i < ret.length; i++ ){
	print("-> " + ret[i] );
}


} 

Glib.mainloop();

print("------- done");

