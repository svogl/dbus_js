#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <map>

#include <assert.h>

#include <jsapi.h>
#include <jsfun.h>

//#include <glib.h>
#include <glib-2.0/glib/gmain.h>
#include <glib-2.0/glib.h>

#include "marshal.h"
using namespace std;


GMainLoop* mainloop = NULL;


////////////////////////////////////////
/////   Glib class
////////////////////////////////////////

typedef struct _Glib {
	// my data;
	char* name;	    ///< bus name 

	JSObject* self;     ///< the javascript object
	JSContext*  cx;     ///< the javascript context

} glibData;


/*************************************************************************************************/
/** Glib Constructor */
static JSBool GlibConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) 
{
	int i;
	if (argc == 0) {
		glibData* dta = NULL;

		dta = (glibData*)calloc(sizeof(glibData), 1);
		
		if (!JS_SetPrivate(cx, obj, dta)) 
			return JS_FALSE;
		return JS_TRUE;
	}
	return JS_FALSE;
}

static void GlibDestructor(JSContext *cx, JSObject *obj) {
	printf("Destroying Glib object\n");
	glibData* dta = (glibData *)JS_GetPrivate(cx, obj);
	if (dta) {
		free(dta->name);
		free(dta);
	}
}

/*************************************************************************************************/

///// Glib Function Table
static JSFunctionSpec _GlibFunctionSpec[] = {
//	{ "connectToSignal",    Glib_connectToSignal,   0, 0, 0 },
//	{ "emitSignal",          Glib_emitSignal,    	0, 0, 0 },
/*
	{ "position",          Actor_set_position,    0, 0, 0 },
	{ "size",              Actor_set_size,    0, 0, 0 },
	{ "addActor",          Actor_container_add,    0, 0, 0 },
	{ "showAll",           Actor_show_all,    0, 0, 0 },
	{ "connect",           Actor_connect,    0, 0, 0 },
	{ "gtk_connect",       Actor_gtk_connect,    0, 0, 0 },
*/
	{ 0, 0, 0, 0, 0 }
};

/*************************************************************************************************/
///// Glib Class Definition

static JSClass Glib_jsClass = {
	"Glib", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, // add/del prop
	JS_PropertyStub, JS_PropertyStub, // get/set prop
	JS_EnumerateStub, JS_ResolveStub, // enum / resolve
	JS_ConvertStub, GlibDestructor,  // convert / finalize
	0, 0, 0, 0,                  
	0, 0, 0, 0
};
/*************************************************************************************************/
/* static funcs: */
static JSBool Glib_s_mainloop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	printf("object is in mainloop: %p\n",obj);

	check_args( (argc == 0), "must not pass an argument!\n");

	JSObject *bus = JS_ConstructObject(cx, &Glib_jsClass, NULL,NULL);
	glibData* dta = (glibData *)JS_GetPrivate(cx, bus);

        g_main_run(mainloop);

	*rval = OBJECT_TO_JSVAL(bus);
	return JS_TRUE;
}

static JSFunctionSpec _GlibStaticFunctionSpec[] = {
	{ "mainloop",   Glib_s_mainloop,          0, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};


///// Actor Initialization Method
JSObject* GlibInit(JSContext *cx, JSObject *obj) {
	if (obj==NULL)
		obj = JS_GetGlobalObject(cx);

	// Clutter
	JS_InitClass(cx, obj, NULL, 
			&Glib_jsClass, 
			GlibConstructor, 0, 
			NULL, // properties 
			 _GlibFunctionSpec, // functions
    	                NULL, // static properties
			_GlibStaticFunctionSpec // static functions
			);

	/***************************** glib main loop omot */
  /* Will store the result of the RequestName RPC here. */

  /* Create a main loop that will dispatch callbacks. */
  mainloop = g_main_loop_new(NULL, FALSE);
  if (mainloop == NULL) {
    /* Print error and terminate. */
    fprintf(stderr, "Could not create GMainLoop!");
	exit(1);
  }
}

/*************************************************************************************************/
