/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

i = 0;

function Hello() {
	this.handle = function() {
		print("hello");
	}
}

function FunFun() {
	this.handle = function() {
		print("funfun");
		i++;
		if (i==10) {
			print("Success!");
			Glib.quit();
		}
	}
}

Glib.idleFuncs = new Array();
Glib.enableIdleFuncs();

Glib.idleFuncs[Glib.idleFuncs.length] = new Hello();
Glib.idleFuncs[Glib.idleFuncs.length] = new FunFun();

print("idles: " + Glib.idleFuncs);

Glib.mainloop();

