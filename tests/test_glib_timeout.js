/**
	this file shall test the functionality for exporting interfaces on the session bus.   
 */
DSO.load("dbus"); 

function Timeout1() {
	this.handle = function() {
		print("hello1");
	}
}
function Timeout2() {
	this.handle = function() {
		print("hello2");
		Glib.addTimeout(new Timeout3(), 100);
	}
}
function Timeout3() {
	this.count=0;
	this.handle = function() {
		print("hello3");
		this.count++;
		return this.count<2;
	}
}

function Quit() {
	this.handle = function() {
		print("Success: This is the end");
		Glib.quit();
	}
}

Glib.addTimeout(new Timeout1(), 100);
Glib.addTimeout(new Timeout2(), 100);
Glib.addTimeout(new Quit(), 600);

Glib.mainloop();

