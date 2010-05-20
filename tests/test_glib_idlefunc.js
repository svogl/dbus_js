/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

i = 0;

hello = function() {
	print("hello");
}

funfun = function() {
	print("funfun");
	i++;
	if (i==10) {
		print("Success!");
		Glib.quit();
	}
}

Glib.idleFuncs = new Array();
Glib.enableIdleFuncs();

Glib.idleFuncs[Glib.idleFuncs.length] = hello;
Glib.idleFuncs[Glib.idleFuncs.length] = funfun;

Glib.mainloop();

