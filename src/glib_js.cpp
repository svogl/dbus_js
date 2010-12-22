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

#include <glib.h>

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

    check_args((argc == 0), "must not pass an argument!\n");
    /*
    JSObject *bus = JS_ConstructObject(cx, &Glib_jsClass, NULL, NULL);
    glibData* dta = (glibData *) JS_GetPrivate(cx, bus);
     */
    g_main_run(mainloop);

    return JS_TRUE;
}

static JSBool Glib_s_quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

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

    JS_AddRoot(data->cx, &iter);

    jsval prop = JSVAL_NULL;
    do {
        jsid id;
        jsval val;

        ret = JS_NextProperty(data->cx, iter, &id);

        if (!ret || JSVAL_IS_VOID(val))
            break;
        ret = JS_LookupPropertyById(data->cx, idleFuncs, id, &val);

        if (!ret)
            continue;
        if (!JSVAL_IS_OBJECT(val) )
            continue;
        JSObject* handler = JSVAL_TO_OBJECT(val);

        jsval argv;
        jsval rval;

        ret = JS_CallFunctionName(data->cx, handler, "handle", 0, &argv, &rval);

    } while (prop != JSVAL_VOID);

    JS_RemoveRoot(data->cx, &iter);
    JS_EndRequest(data->cx);
    // end transact
    return true;
}

static JSBool Glib_s_idleEnable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    pass_only_if((argc == 0), "must not pass an argument!\n");

    glibData->cx = cx;
    glibData->glib = obj;
    glibData->glibPtr = g_malloc(sizeof (GlibData*));

    GlibData* data = (GlibData*)glibData->glibPtr;
    data->cx = cx;
    data->glib = obj;

    g_idle_add(glib_idle_func, glibData->glibPtr);

    return JS_TRUE;
}

static JSBool Glib_s_idleDisable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    pass_only_if((argc == 0), "must not pass an argument!\n");

    g_idle_remove_by_data(glibData->glibPtr);
    glibData->glibPtr = 0;
    return JS_TRUE;
}

typedef struct _timeoutData {
    JSContext* cx;
    JSObject* handler;
} TimeoutData;

gboolean _glib_timout_func(gpointer ptr) {
    JSBool ret;
    if (!ptr) {
        dbg_err("ptr is null!");
        return false;
    }
    TimeoutData* tod = (TimeoutData*)ptr;

    jsval argv, rval;
    JS_BeginRequest(tod->cx);
    ret = JS_CallFunctionName(tod->cx, tod->handler, "handle", 0, &argv, &rval);
    JS_EndRequest(tod->cx);
    if (JSVAL_IS_BOOLEAN(rval)) {
        JSBool cont = JSVAL_TO_BOOLEAN(rval);
        if (!cont)
            g_free(ptr); // not needed any more
        return cont;
    }
    return false;
}

static JSBool Glib_s_addTimeout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbg3_err("add timeout");

    pass_only_if((argc == 2), "must not pass an argument!\n");
    pass_only_if(JSVAL_IS_OBJECT(argv[0]), "arg[0] must be object!");
    pass_only_if(JSVAL_IS_INT(argv[1]), "arg[1] must be an int (ms)!");

    JSObject* handler = JSVAL_TO_OBJECT(argv[0]);
    jsint interval = JSVAL_TO_INT(argv[1]);

    // TODO: check for handle function....
    if (interval <= 0) {
        dbg_err("interval < 0");
        return JS_FALSE;
    }

    gpointer ptr = g_malloc(sizeof (TimeoutData));
    TimeoutData* tod = (TimeoutData*)ptr;
    tod->cx = cx;
    tod->handler = handler;

    g_timeout_add(interval, _glib_timout_func, ptr);

    return JS_TRUE;
}

static JSFunctionSpec _GlibStaticFunctionSpec[] = {
    { "mainloop", Glib_s_mainloop, 0, 0, 0},
    { "quit", Glib_s_quit, 0, 0, 0},
    { "enableIdleFuncs", Glib_s_idleEnable, 0, 0, 0},
    { "disableIdleFuncs", Glib_s_idleDisable, 0, 0, 0},
    { "addTimeout", Glib_s_addTimeout, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};


///// Actor Initialization Method

JSObject* GlibInit(JSContext *cx, JSObject *obj) {
    if (obj == NULL)
        obj = JS_GetGlobalObject(cx);

    JSObject* o = JS_InitClass(cx, obj, NULL,
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
    return o;
}

JSBool GlibExit(JSContext *ctx)
{
    g_main_loop_quit(mainloop);
    g_main_destroy(mainloop);
    mainloop=0;
    if (glibData) {
        delete glibData;
        glibData=0;
    }

    return JS_TRUE;
}


/*************************************************************************************************/
