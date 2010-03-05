
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

#include <dbus/dbus.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <glib.h>

#include "marshal.h"
using namespace std;


/************** forward decls: */
DBusHandlerResult
filter_func(DBusConnection* connection, DBusMessage* message, void* user_data);

DBusHandlerResult
message_handler(DBusConnection* connection, DBusMessage* message, void* user_data);

////////////////////////////////////////
/////   DBus class
////////////////////////////////////////

/// private data for the dbus connection.

typedef struct _cbi {
    string match;
    jsval callback;
} callbackInfo;

typedef struct _dbus {
    // my data;
    char* name; ///< bus name

    JSObject* self; ///< the javascript object
    JSContext* ctx; ///< the javascript context

    DBusConnection* connection;
    DBusError error;

    map<string, callbackInfo*> handlers;

} dbusData;

static JSBool DBus_addSigHandler(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    string matchRule;
    string key;
    DBusError dbus_error;

    check_args(argc == 5, "pass service + one callback function!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (service;ignored)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[2]), "arg[2] must be a string (interface)!");
    check_args(JSVAL_IS_STRING(argv[3]), "arg[3] must be a string (signal)!");
    check_args(JSVAL_IS_OBJECT(argv[4]), "arg[4] must be a callback function!");

    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);

    //JSString* servN = JSVAL_TO_STRING(argv[0]);
    JSString* oPath = JSVAL_TO_STRING(argv[1]);
    JSString* iFace = JSVAL_TO_STRING(argv[2]);
    JSString* sigName = JSVAL_TO_STRING(argv[3]);
    JSObject* fo = JSVAL_TO_OBJECT(argv[4]);
    check_args(JS_ObjectIsFunction(ctx, fo), "arg[4] must be a callback function!");

    dbus_error_init(&dbus_error);

    matchRule = "type='signal',interface='";
    matchRule += JS_GetStringBytes(iFace);
    matchRule += "',member='";
    matchRule += JS_GetStringBytes(sigName);
    matchRule += "',path='";
    matchRule += JS_GetStringBytes(oPath);
    matchRule += "'";
    dbg(cerr << "ADD MATCH: " << matchRule << "\n");

    key = JS_GetStringBytes(oPath);
    key += "/";
    key += JS_GetStringBytes(iFace);
    key += "/";
    key += JS_GetStringBytes(sigName);

    dbus_bus_add_match(dta->connection, matchRule.c_str(), &dbus_error);
    check_dbus_error(dbus_error);
    callbackInfo* cbi = new(callbackInfo);
    cbi->match = matchRule;
    cbi->callback = argv[4];
    dta->handlers[key] = cbi;

    return JS_TRUE;
}

static JSBool DBus_removeSigHandler(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    string matchRule;
    string key;
    DBusError dbus_error;

    check_args(argc == 5, "pass service + one callback function!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (service;ignored)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[0] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[2]), "arg[1] must be a string (interface)!");
    check_args(JSVAL_IS_STRING(argv[3]), "arg[2] must be a string (signal)!");
    check_args(JSVAL_IS_OBJECT(argv[4]), "arg[3] must be a callback function!");

    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);

    //JSString* servN = JSVAL_TO_STRING(argv[0]);
    JSString* oPath = JSVAL_TO_STRING(argv[0]);
    JSString* iFace = JSVAL_TO_STRING(argv[1]);
    JSString* sigName = JSVAL_TO_STRING(argv[2]);

    dbus_error_init(&dbus_error);

    matchRule = "type='signal',interface='";
    matchRule += JS_GetStringBytes(iFace);
    matchRule += "',member='";
    matchRule += JS_GetStringBytes(sigName);
    matchRule += "',path='";
    matchRule += JS_GetStringBytes(oPath);
    matchRule += "'";
    dbg(cerr << "REMOVE MATCH: " << matchRule << "\n");

    key = "sig:";
    key += JS_GetStringBytes(oPath);
    key += "/";
    key += JS_GetStringBytes(iFace);
    key += "/";
    key += JS_GetStringBytes(sigName);
    JS_DeleteProperty(ctx, obj, key.c_str());

    dbus_bus_remove_match(dta->connection, matchRule.c_str(), &dbus_error);

    return JS_TRUE;
}

/** DBus connect to signal: */
static JSBool DBus_emitSignal(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    DBusMessage *message;
    DBusMessageIter iter;

    check_args(argc >= 3, "argc must be >=3!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (interface)!");
    check_args(JSVAL_IS_STRING(argv[2]), "arg[2] must be a string (signal)!");

    JSString* opath = JSVAL_TO_STRING(argv[0]);
    JSString* iface = JSVAL_TO_STRING(argv[1]);
    JSString* signame = JSVAL_TO_STRING(argv[2]);

    message = dbus_message_new_signal(JS_GetStringBytes(opath),
            JS_GetStringBytes(iface),
            JS_GetStringBytes(signame));
    check_args(message != NULL, "OOM!");

    if (argc > 3) {
        dbus_message_iter_init_append(message, &iter);
        DBusMarshalling::appendArgs(ctx, obj, message, &iter, argc - 3, argv + 3);
    }

    dbus_connection_send(connection, message, NULL);
    dbus_connection_flush(connection);
    dbus_message_unref(message);

    return JS_TRUE;
}

static JSBool DBus_requestBusName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    DBusError dbus_error;

    check_args(argc == 3, "argc must be 1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (bus name)!");

    JSString* busName = JSVAL_TO_STRING(argv[0]);

    dbus_error_init(&dbus_error);

    dbus_bus_request_name(connection,
            JS_GetStringBytes(busName),
            0, // flags
            &dbus_error);
    check_dbus_error(dbus_error);

    return JS_TRUE;
}

static JSBool DBus_callMethod(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    DBusMessage *message;
    DBusMessageIter iter;
    DBusMessage *reply;
    DBusError dbus_error;

    check_args(argc >= 4, "argc must be 1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (service name)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[2]), "arg[2] must be a string (interface)!");
    check_args(JSVAL_IS_STRING(argv[3]), "arg[3] must be a string (member to call)!");

    JSString* sName = JSVAL_TO_STRING(argv[0]);
    JSString* oPath = JSVAL_TO_STRING(argv[1]);
    JSString* iFace = JSVAL_TO_STRING(argv[2]);
    JSString* method = JSVAL_TO_STRING(argv[3]);

    dbus_error_init(&dbus_error);

    dbg2(cerr << "sName " << JS_GetStringBytes(sName) << endl);
    dbg2(cerr << "opath " << JS_GetStringBytes(oPath) << endl);
    dbg2(cerr << "iface " << JS_GetStringBytes(iFace) << endl);
    dbg2(cerr << "member " << JS_GetStringBytes(method) << endl);

    message = dbus_message_new_method_call(
            JS_GetStringBytes(sName),
            JS_GetStringBytes(oPath),
            JS_GetStringBytes(iFace),
            JS_GetStringBytes(method));

    if (message == NULL) {
        return JS_FALSE;
    }

    if (argc > 4) {
        dbus_message_iter_init_append(message, &iter);

        DBusMarshalling::appendArgs(ctx, obj, message, &iter, argc - 4, argv + 4);
    } else {
    }
    // only blocking calls!
    reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &dbus_error);
    dbus_message_unref(message);

    check_dbus_error(dbus_error);

    const char* sig = dbus_message_get_signature(reply);
    dbg2(cerr << "reply sig::: " << sig << endl);

    int length = 0;
    dbus_message_iter_init(reply, &iter);
    while (dbus_message_iter_next(&iter)) {
        length++;
    }

    dbus_message_iter_init(reply, &iter);
    jsval* vector = new jsval[length];

    JSBool success = DBusMarshalling::getVariantArray(ctx, &iter, &length, vector);

    dbg2(cerr << "reply array length: " << length << endl);

    if (success) {
        if (length == 1) {
            *rval = vector[0];

        } else {

            cerr << "TODO: create array object!" << endl;
        }
    }
    delete vector;

    return JS_TRUE;
}
/*************************************************************************************************/

/** DBus message handling - callbacks, utility functions */

DBusHandlerResult
filter_func(DBusConnection* connection, DBusMessage* message, void* user_data) {
    dbusData* dta = (dbusData*) user_data;

    const char *interface;
    const char *name;
    const char *path;
    string key;
    DBusMessageIter iter;

    cout << "----- filter handlers:" << endl;
    map<string, callbackInfo*>& m = dta->handlers;
    map<string, callbackInfo*>::iterator it;
    it = m.begin();
    while (it != m.end()) {
        cout << it->first << " ---> " << it->second->match << endl;
        it++;
    }
    cout << "----- filter handlers." << endl;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        interface = dbus_message_get_interface(message);
        name = dbus_message_get_member(message);
        path = dbus_message_get_path(message);

        key = path;
        key += "/";
        key += interface;
        key += "/";
        key += name;
        cerr << "filter_func key::" << key << endl;

        if ((it = m.find(key)) != m.end()) {

            cout << "----- found in map!!!\n";
        } else {

            cout << "----- not found in map.\n";
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        JSObject* busObj = dta->self;
        JSFunction *callback = NULL;
        jsval valp;

#if 0
        JSBool ret = JS_GetProperty(dta->ctx, busObj, key.c_str(), &valp);
        if (!ret) {
            cerr << "ret false for prop " << key << endl;
        } else {
            if (JSVAL_IS_NULL(valp)) {
                cerr << "null. no match\n";
                goto fail;
            }
            if (JSVAL_IS_VOID(valp)) {
                cerr << "void. no match\n";
                goto fail;
            }
            if (!JSVAL_IS_OBJECT(valp)) {
                cerr << " key->value should be a callback. " << valp << endl;
                goto fail;
            }
            JSObject* fo = JSVAL_TO_OBJECT(valp);
            if (!JS_ObjectIsFunction(dta->ctx, fo)) {
                cerr << " key->value should be a callback.\n";
                goto fail;
            }

            callback = JS_ValueToFunction(dta->ctx, valp);
        }
#else
        valp = it->second->callback;

        if (JSVAL_IS_VOID(valp)) {
            dbg3_err("void. no match");
            goto fail;
        } else if (JSVAL_IS_NULL(valp)) {
            dbg3_err("null. no match");
            goto fail;
        } else if (!JSVAL_IS_OBJECT(valp)) {
            dbg3_err(" key->value should be a callback. " << valp);
            goto fail;
        }
        JSObject* fo = JSVAL_TO_OBJECT(valp);
        if (!JS_ObjectIsFunction(dta->ctx, fo)) {
            cerr << " key->value should be a callback.\n";
            goto fail;
        }

        callback = JS_ValueToFunction(dta->ctx, valp);

#endif
        /*
                if (callback == NULL)
                    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
         */
        dbg2(cerr << "filter_func key::found handler" << endl);
        /// @TODO: restriction at the moment: only one callback func per signal...

        int length = 0;

        dbus_message_iter_init(message, &iter);
        while (dbus_message_iter_next(&iter)) {
            length++;
        }
        dbg3(cerr << "length = " << length << endl);

        jsval* vector = new jsval[length];

        dbus_message_iter_init(message, &iter);

        DBusMarshalling::getVariantArray(dta->ctx, &iter, &length, vector);
        dbg3(cerr << "length after unmarsh = " << length << endl);

        //nsIVariant *result;
        //  info->callback->Method(interface, name, args, length, &result);
        jsval rval;

        JSObject * args = JS_NewArrayObject(dta->ctx, length, vector);
        jsval argv = OBJECT_TO_JSVAL(args);

        if (callback == NULL)
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        else
            JS_CallFunction(dta->ctx, busObj, callback, 1, &argv, &rval);

        delete vector;
        return DBUS_HANDLER_RESULT_HANDLED;
    } else {
        cerr << "dbus ignore msg type " << dbus_message_get_type(message) << endl;
    }
fail:
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusHandlerResult
message_handler(DBusConnection* connection,
        DBusMessage* message,
        void* user_data) {
    const char *interface;
    const char *member;
    JSBool ret;
    DBusMessageIter iter;
    int length;
    jsval* vector;

    dbusData* dta = (dbusData*) user_data;

    member = dbus_message_get_member(message);
    interface = dbus_message_get_interface(message);

    cerr << "interface " << interface << endl;
    cerr << "member    " << member << endl;

    string key = "";
    key += interface;
    key += "::";
    key += member;

    JSObject* busObj = dta->self;
    JSFunction *callback = NULL;
    jsval valp;

    ret = JS_GetProperty(dta->ctx, busObj, key.c_str(), &valp);
    if (!ret) {
        cerr << "ret false for prop " << key << endl;
    } else {
        if (!JSVAL_IS_OBJECT(valp)) {
            cerr << " key->value should be a callback. " << valp << endl;
            goto fail;
        }
        JSObject* fo = JSVAL_TO_OBJECT(valp);
        if (!JS_ObjectIsFunction(dta->ctx, fo)) {
            cerr << " key->value should be a callback.\n";
            goto fail;
        }

        callback = JS_ValueToFunction(dta->ctx, valp);
    }
    if (callback == NULL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        // @TODO: throw exception over dbus?
    }

    cerr << "filter_func key::found handler" << endl;
    /// @TODO: restriction at the moment: only one callback func per signal...

    length = 0;

    dbus_message_iter_init(message, &iter);
    while (dbus_message_iter_next(&iter)) {
        length++;
    }
    cerr << "length = " << length << endl;
    {
        jsval rval;
        vector = new jsval[length];

        dbus_message_iter_init(message, &iter);

        DBusMarshalling::getVariantArray(dta->ctx, &iter, &length, vector);
        cerr << "length after = " << length << endl;

        JSObject* args = JS_NewArrayObject(dta->ctx, length, vector);
        jsval argv = OBJECT_TO_JSVAL(args);

        if (callback == NULL)
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        else
            JS_CallFunction(dta->ctx, busObj, callback, 1, &argv, &rval);

        // TODO: unref array object?
        delete vector;
    }
    return DBUS_HANDLER_RESULT_HANDLED;
fail:
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static JSBool DBus_registerObject(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    DBusObjectPathVTable vtable = {
        NULL, // unregister function
        &message_handler
    };

    check_args(argc == 1, "argc must be ==1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (obj path)!");

    JSString* opath = JSVAL_TO_STRING(argv[0]);

    enforce_notnull(
            dbus_connection_register_object_path(connection,
            JS_GetStringBytes(opath),
            &vtable,
            dta)); /// @TODO: throw exception
    return JS_TRUE;
}

static JSBool DBus_unregisterObject(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    check_args(argc == 1, "argc must be ==1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (obj path)!");

    JSString* opath = JSVAL_TO_STRING(argv[0]);

    enforce_notnull(
            dbus_connection_unregister_object_path(connection,
            JS_GetStringBytes(opath)));
    return JS_TRUE;
}

/*************************************************************************************************/

/** DBus Constructor */
static JSBool DBusConstructor(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc == 0) {
        dbusData* dta = NULL;

        //dta = (dbusData*) calloc(sizeof (dbusData), 1);
        dta = new( dbusData);

        if (!JS_SetPrivate(ctx, obj, dta))
            return JS_FALSE;
        return JS_TRUE;
    }
    return JS_FALSE;
}

static void DBusDestructor(JSContext *ctx, JSObject *obj) {
    printf("Destroying DBus object\n");
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    if (dta) {
        free(dta->name);
        delete (dta);
    }
}

/*************************************************************************************************/

///// DBus Function Table
static JSFunctionSpec _DBusFunctionSpec[] = {
    { "registerObject", DBus_registerObject, 0, 0, 0},
    { "unregisterObject", DBus_unregisterObject, 0, 0, 0},
    { "addSigHandler", DBus_addSigHandler, 0, 0, 0},
    { "removeSigHandler", DBus_removeSigHandler, 0, 0, 0},
    { "requestBusName", DBus_requestBusName, 0, 0, 0},
    { "emitSignal", DBus_emitSignal, 0, 0, 0},
    { "call", DBus_callMethod, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};

/*************************************************************************************************/
///// DBus Class Definition

static JSClass DBus_jsClass = {
    "DBus", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, DBusDestructor, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};

JSClass DBusResult_jsClass = {
    "DBusResult", 0,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, 0, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};

JSClass DBusError_jsClass = {
    "DBusError", 0,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, 0, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};

/*************************************************************************************************/
/* static funcs: */

/**
 * Connect to the DBUS bus
 */
static JSBool _DBus_connect(dbusData* dta, DBusBusType type) {
    // initialise the error value
    dbus_error_init(&dta->error);

    // connect to the DBUS system bus, and check for errors
    dta->connection = dbus_bus_get(type, &dta->error);
    if (dbus_error_is_set(&dta->error)) {
        fprintf(stderr, "Connection Error (%s)\n", dta->error.message);
        dbus_error_free(&dta->error);
        return JS_FALSE;
    }
    if (dta->connection == NULL) {
        fprintf(stderr, "Connection is NULL\n");
        return JS_FALSE;
    }

    dbus_connection_setup_with_g_main(dta->connection, NULL);

    enforce_notnull(
            dbus_connection_add_filter(dta->connection,
            filter_func, dta, NULL));

    return JS_TRUE;
}

static JSBool DBus_s_systemBus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    printf("object is in systemBus: %p\n", obj);

    check_args((argc == 0), "must not pass an argument!\n");

    JSObject *bus = JS_ConstructObject(ctx, &DBus_jsClass, NULL, NULL);
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, bus);
    dta->ctx = ctx;
    dta->self = bus;

    if (JS_FALSE == _DBus_connect(dta, DBUS_BUS_SYSTEM)) {
        return JS_FALSE;
    }
    dta->name = strdup("system");

    *rval = OBJECT_TO_JSVAL(bus);
    return JS_TRUE;
}

static JSBool DBus_s_sessionBus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    printf("object is in sessionBus: %p\n", obj);

    check_args((argc == 0), "must not pass an argument!\n");

    JSObject *bus = JS_ConstructObject(ctx, &DBus_jsClass, NULL, NULL);
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, bus);
    dta->ctx = ctx;
    dta->self = bus;

    if (JS_FALSE == _DBus_connect(dta, DBUS_BUS_SESSION)) {
        return JS_FALSE;
    }
    dta->name = strdup("session");

    *rval = OBJECT_TO_JSVAL(bus);
    return JS_TRUE;
}

static JSFunctionSpec _DBusStaticFunctionSpec[] = {
    { "sessionBus", DBus_s_sessionBus, 0, 0, 0},
    { "systemBus", DBus_s_systemBus, 0, 0, 0},
    { 0, 0, 0, 0, 0}
};

///// Actor Initialization Method

JSObject* DBusInit(JSContext *ctx, JSObject *obj) {
    if (obj == NULL)
        obj = JS_GetGlobalObject(ctx);

    // DBus bus connections
    JS_InitClass(ctx, obj, NULL,
            &DBus_jsClass,
            DBusConstructor, 0,
            NULL, // properties
            _DBusFunctionSpec, // functions
            NULL, // static properties
            _DBusStaticFunctionSpec // static functions
            );

    // result object
    JS_InitClass(ctx, obj, NULL,
            &DBusResult_jsClass,
            0, 0, NULL, // properties
            0, NULL, 0);

    // error obj
    JS_InitClass(ctx, obj, NULL,
            &DBusResult_jsClass,
            0, 0, NULL, // properties
            0, NULL, 0);
}

/*************************************************************************************************/
extern JSObject* GlibInit(JSContext *ctx, JSObject *obj);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int js_DSO_load(JSContext *context) {
        if (!DBusInit(context, NULL)) {
            fprintf(stderr, "Cannot init DBus class\n");
            return EXIT_FAILURE;
        }

        if (!GlibInit(context, NULL)) {
            fprintf(stderr, "Cannot init Glib class\n");
            return EXIT_FAILURE;
        }

        return JS_TRUE;
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */

