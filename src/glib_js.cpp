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
    JSContext *cx;
    JSObject* glib;

    gpointer glibPtr;
} GlibData;

static GlibData* glibData;


/*************************************************************************************************/

/** Glib Constructor */
static JSBool GlibConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc == 0) {
        GlibData* dta = NULL;

        dta = new GlibData;

        if (!JS_SetPrivate(cx, obj, dta))
            return JS_FALSE;
        return JS_TRUE;
    }
    return JS_FALSE;
}

static void GlibDestructor(JSContext *cx, JSObject *obj) {
    printf("Destroying Glib object\n");
    GlibData* dta = (GlibData *) JS_GetPrivate(cx, obj);
    if (dta) {
        delete dta;
    }
}

/*************************************************************************************************/

///// Glib Function Table
static JSFunctionSpec _GlibFunctionSpec[] = {
    { 0, 0, 0, 0, 0}
};

/*************************************************************************************************/
///// Glib Class Definition

static JSClass Glib_jsClass = {
    "Glib", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, GlibDestructor, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};
/*************************************************************************************************/

/* static funcs: */
static JSBool Glib_s_mainloop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    printf("object is in mainloop: %p\n", obj);

    check_args((argc == 0), "must not pass an argument!\n");
    /*
    JSObject *bus = JS_ConstructObject(cx, &Glib_jsClass, NULL, NULL);
    glibData* dta = (glibData *) JS_GetPrivate(cx, bus);
     */
    g_main_run(mainloop);

    return JS_TRUE;
}

static JSBool Glib_s_quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    printf("object is in mainloop: %p\n", obj);

    check_args((argc == 0), "must not pass an argument!\n");

    g_main_loop_quit(mainloop);

    return JS_TRUE;
}

static gboolean glib_idle_func(gpointer p) {
    GlibData* data = (GlibData*) p;
    JSBool ret;
    if (!data) {
        data = (GlibData*) glibData;
    }
    JS_BeginRequest(data->cx);
    // begin transact
    jsval val;
    ret = JS_GetProperty(data->cx, data->glib, "idleFuncs", &val);

    if (!JSVAL_IS_OBJECT(val)) {
        dbg_err("Glib.idleFuncs not found! deactivating callback ret=" << ret);
        JS_EndRequest(data->cx);
        return false; // stop execution. try again later.
    }

    //ensure_val_is_object_or_fail(connection, message, ret, funcs, "service keys hash");
    if (JSVAL_IS_VOID(val) || JSVAL_IS_NULL(val)) {
        dbg_err("Glib.idleFuncs not found! deactivating callback ret=" << ret);
        JS_EndRequest(data->cx);
        return false;
    }
    JSObject* idleFuncs = JSVAL_TO_OBJECT(val);
    JSObject* iter = JS_NewPropertyIterator(data->cx, idleFuncs);

    jsval prop = JSVAL_NULL;
    do {
        jsid id;
        jsval val;
        fprintf(stderr, ",");
        ret = JS_NextProperty(data->cx, iter, &id);
        if (!ret || JSVAL_IS_VOID(val))
            break;
        ret = JS_LookupPropertyById(data->cx, idleFuncs, id, &val);
        if (!ret)
            continue;
        if (!JSVAL_IS_OBJECT(val) || !JS_ObjectIsFunction(data->cx, JSVAL_TO_OBJECT(val)))
            continue;

        JSFunction* func = JS_ValueToFunction(data->cx, val);
        jsval argv;
        jsval rval;
        ret = JS_CallFunction(data->cx, data->glib, func, 0, &argv, &rval);
        if (!ret) {
        }
    } while (prop != JSVAL_VOID);

    JS_EndRequest(data->cx);
    // end transact
    fprintf(stderr, ".");
    return true;
}

static JSBool Glib_s_idleEnable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    printf("enable idle func cx=%o \n", cx);

    pass_only_if((argc == 0), "must not pass an argument!\n");

    glibData->cx = cx;
    glibData->glib = obj;
    glibData->glibPtr = g_malloc(sizeof (GlibData*));

    g_idle_add(glib_idle_func, 0);

    return JS_TRUE;
}

static JSFunctionSpec _GlibStaticFunctionSpec[] = {
    { "mainloop", Glib_s_mainloop, 0, 0, 0},
    { "quit", Glib_s_quit, 0, 0, 0},
    { "enableIdleFuncs", Glib_s_idleEnable, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};


///// Actor Initialization Method

JSObject* GlibInit(JSContext *cx, JSObject *obj) {
    if (obj == NULL)
        obj = JS_GetGlobalObject(cx);

    JS_InitClass(cx, obj, NULL,
            &Glib_jsClass,
            GlibConstructor, 0,
            NULL, // properties
            _GlibFunctionSpec, // functions
            NULL, // static properties
            _GlibStaticFunctionSpec // static functions
            );

    glibData = new GlibData;
    glibData->cx = cx;

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
