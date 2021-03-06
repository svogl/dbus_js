
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

typedef struct _exi {
    int flags; ///< jsobject implementing the sig.
    JSObject* o; ///< the object implementing the interface
    jsval obj; ///< jsobject implementing the sig.
    string interface; ///< the dbus interface
    string path; ///< the dbus interface
    string signature; ///< the xml signature for introspection
} expInfo;

typedef struct _path {
    string name; // name of this path
    map<string, struct _path*> paths;
    map<string, expInfo*> services;
} pathInfo;

typedef struct _dbus {
    // my data;
    char* name; ///< bus name

    JSObject* self; ///< the javascript bus object
    JSContext* ctx; ///< the javascript context

    DBusConnection* connection;
    string unique_name; ///< the bus connection name
    DBusError error;

    map<string, callbackInfo*> sigHandlers;

    map<string, expInfo*> expObjects;

    map<string, void*> busNames;

    pathInfo root;
} dbusData;

/// private data for the dbus message object

typedef struct _msg {
    DBusConnection* con;
    DBusMessage* msg;
} msgData;


////////////////////////////////////////
/////   DBus class
////////////////////////////////////////

static JSBool DBus_addSigHandler(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    string matchRule = "";
    string key = "";
    DBusError dbus_error;

    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);

    // update:: use one dict object for matching
    if (argc == 1 && jsvalIsDictObject(ctx, argv[0])) {
        // dict entries contain match rule:
        JSObject* opts = JSVAL_TO_OBJECT(argv[0]);
        jsval arg;

        if (JS_GetProperty(ctx, opts, "sigtype", &arg)
                && JSVAL_IS_STRING(arg)) {
            check_args(JSVAL_IS_STRING(arg), "argument must be a string! %d", arg);
            JSString* argval = JSVAL_TO_STRING(arg);
            matchRule += "type='";
            matchRule += JS_GetStringBytes(argval);
            matchRule += "',";
            key += JS_GetStringBytes(argval);
            key += "/";
        };
        if (JS_GetProperty(ctx, opts, "interface", &arg) && JSVAL_IS_STRING(arg)) {
            check_args(JSVAL_IS_STRING(arg), "argument must be a string!");
            JSString* argval = JSVAL_TO_STRING(arg);
            matchRule += "interface='";
            matchRule += JS_GetStringBytes(argval);
            matchRule += "',";
            key += JS_GetStringBytes(argval);
            key += "/";
        };
        if (JS_GetProperty(ctx, opts, "path", &arg) && JSVAL_IS_STRING(arg)) {
            check_args(JSVAL_IS_STRING(arg), "argument must be a string!");
            JSString* argval = JSVAL_TO_STRING(arg);
            matchRule += "path='";
            matchRule += JS_GetStringBytes(argval);
            matchRule += "',";
            key += JS_GetStringBytes(argval);
            key += "/";
        };
        if (JS_GetProperty(ctx, opts, "member", &arg) && JSVAL_IS_STRING(arg)) {
            check_args(JSVAL_IS_STRING(arg), "argument must be a string!");
            JSString* argval = JSVAL_TO_STRING(arg);
            matchRule += "member='";
            matchRule += JS_GetStringBytes(argval);
            matchRule += "',";
            key += JS_GetStringBytes(argval);
            key += "/";
        };
        dbg2_err("match = " << matchRule);
        dbg2_err("key = " << key);

    } else {
        check_args(argc == 5, "pass service + one callback function!");
        check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (service;ignored)!");
        check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (obj path)!");
        check_args(JSVAL_IS_STRING(argv[2]), "arg[2] must be a string (interface)!");
        check_args(JSVAL_IS_STRING(argv[3]), "arg[3] must be a string (signal)!");
        check_args(JSVAL_IS_OBJECT(argv[4]), "arg[4] must be a callback function!");

        //JSString* servN = JSVAL_TO_STRING(argv[0]);
        JSString* oPath = JSVAL_TO_STRING(argv[1]);
        JSString* iFace = JSVAL_TO_STRING(argv[2]);
        JSString* sigName = JSVAL_TO_STRING(argv[3]);

        JSObject* fo = JSVAL_TO_OBJECT(argv[4]);
        check_args(JS_ObjectIsFunction(ctx, fo), "arg[4] must be a callback function!");

        matchRule = "type='signal',interface='";
        matchRule += JS_GetStringBytes(iFace);
        matchRule += "',member='";
        matchRule += JS_GetStringBytes(sigName);
        matchRule += "',path='";
        matchRule += JS_GetStringBytes(oPath);
        matchRule += "'";

        dbg2_err("ADD MATCH: " << matchRule );

        key = JS_GetStringBytes(oPath);
        key += "/";
        key += JS_GetStringBytes(iFace);
        key += "/";
        key += JS_GetStringBytes(sigName);
    }

    callbackInfo* cbi = new(callbackInfo);
    cbi->match = matchRule;
    cbi->callback = argv[4];

    dbus_error_init(&dbus_error);
    dbus_bus_add_match(dta->connection, matchRule.c_str(), &dbus_error);
    check_dbus_error(dbus_error);
    dta->sigHandlers[key] = cbi;

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

static JSBool DBus_exportObject(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    string matchRule = "";
    string key = "";
    DBusError dbus_error;

    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);

    check_args(argc == 3, "pass service + one callback function!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (interface)!");
    check_args(JSVAL_IS_OBJECT(argv[2]), "arg[2] must be the exposed object!");

    //JSString* servN = JSVAL_TO_STRING(argv[0]);
    JSString* oPath = JSVAL_TO_STRING(argv[0]);
    JSString* iFace = JSVAL_TO_STRING(argv[1]);

    key = JS_GetStringBytes(oPath);
    key += "_";
    key += JS_GetStringBytes(iFace);

    dbg3_err("export object - keykey " << key);

    expInfo* exi = new expInfo;
    exi->o = JSVAL_TO_OBJECT(argv[2]);
    exi->interface = JS_GetStringBytes(iFace);
    exi->path = JS_GetStringBytes(oPath);

    matchRule += "type='method_call',path='";
    matchRule += JS_GetStringBytes(oPath);
    matchRule += "',interface='";
    matchRule += JS_GetStringBytes(iFace);
    matchRule += "'";

    dbus_error_init(&dbus_error);
    dbus_bus_add_match(dta->connection, matchRule.c_str(), &dbus_error);
    check_dbus_error(dbus_error);

    dta->expObjects[key] = exi;

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

    check_args((argc == 1), "argc must be 1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (bus name)!");

    JSString* busName = JSVAL_TO_STRING(argv[0]);

    dbus_error_init(&dbus_error);

    dbus_bus_request_name(connection,
            JS_GetStringBytes(busName),
            0, // flags
            &dbus_error);
    check_dbus_error(dbus_error);
    dta->unique_name = JS_GetStringBytes(busName);
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

    dbg2_err( "sName " << JS_GetStringBytes(sName) );
    dbg2_err( "opath " << JS_GetStringBytes(oPath) );
    dbg2_err( "iface " << JS_GetStringBytes(iFace) );
    dbg2_err( "member " << JS_GetStringBytes(method) );

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
    dbg2_err("reply sig::: " << sig );

    int length = 0;
    dbus_message_iter_init(reply, &iter);
    length = dbus_iter_length(&iter);
    dbus_message_iter_init(reply, &iter);

    jsval* vector = new jsval[length];

    JSBool success = DBusMarshalling::getVariantArray(ctx, &iter, &length, vector);

    dbg2_err("reply array length: " << length );

    if (success) {
        if (length == 0) {
            // nil.
        } else if (length == 1) {
            *rval = vector[0];

        } else {
            cerr << "TODO: create array object of return values!" << endl;
        }
    }
    delete[] vector;

    return JS_TRUE;
}

static JSBool DBus_callMethodAA(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    DBusConnection *connection = dta->connection;
    DBusMessage *message;
    DBusMessageIter iter;
    DBusMessage *reply;
    DBusError dbus_error;

    check_args(argc == 5, "argc must be 5!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (service name)!");
    check_args(JSVAL_IS_STRING(argv[1]), "arg[1] must be a string (obj path)!");
    check_args(JSVAL_IS_STRING(argv[2]), "arg[2] must be a string (interface)!");
    check_args(JSVAL_IS_STRING(argv[3]), "arg[3] must be a string (member to call)!");
    check_args(JSVAL_IS_OBJECT(argv[4]), "arg[4] must be an (array) object!");

    JSString* sName = JSVAL_TO_STRING(argv[0]);
    JSString* oPath = JSVAL_TO_STRING(argv[1]);
    JSString* iFace = JSVAL_TO_STRING(argv[2]);
    JSString* method = JSVAL_TO_STRING(argv[3]);

    dbus_error_init(&dbus_error);

    dbg2_err( "sName " << JS_GetStringBytes(sName) );
    dbg2_err( "opath " << JS_GetStringBytes(oPath) );
    dbg2_err( "iface " << JS_GetStringBytes(iFace) );
    dbg2_err( "member " << JS_GetStringBytes(method) );

    message = dbus_message_new_method_call(
            JS_GetStringBytes(sName),
            JS_GetStringBytes(oPath),
            JS_GetStringBytes(iFace),
            JS_GetStringBytes(method));

    if (message == NULL) {
        return JS_FALSE;
    }

    JSObject* argArray = JSVAL_TO_OBJECT(argv[4]);
    check_args(JS_IsArrayObject(ctx, argArray), "must pass an array object!");

    if (argc > 4) {
        dbus_message_iter_init_append(message, &iter);

        DBusMarshalling::marshallVariant(ctx, obj, &iter, &argv[4]);
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
    length = dbus_iter_length(&iter);
    dbus_message_iter_init(reply, &iter);

    jsval* vector = new jsval[length];

    JSBool success = DBusMarshalling::getVariantArray(ctx, &iter, &length, vector);

    dbg2(cerr << "reply array length: " << length << endl);

    if (success) {
        if (length == 0) {
            // nil.
        } else if (length == 1) {
            *rval = vector[0];

        } else {
            cerr << "TODO: create array object of return values!" << endl;
        }
    }
    delete vector;

    return JS_TRUE;
}

/*************************************************************************************************/
#ifdef DEBUG_DBUS_LOWLEVEL

/** DBus message handling - callbacks, utility functions */
static void printer(DBusMessageIter* iter) {
    do {
        char* sig = dbus_message_iter_get_signature(iter);
        char* val;
        dbg2_err("  sig " << sig);
        if ('s' == *sig) {
            dbus_message_iter_get_basic(iter, &val);
            dbg2_err("  bas " << val);
        } else if ('a' == *sig // array
                || 'r' == *sig // struct
                || 'e' == *sig // struct
                || '{' == *sig) { // dict
            dbg2_err("  container " << sig);
            DBusMessageIter sub;
            dbus_message_iter_recurse(iter, &sub);
            printer(&sub);
            dbg2_err("  container done: " << sig);
        } else {
            dbg2_err("  unknown sig  " << sig);
        }
    } while (dbus_message_iter_next(iter));
}
#endif

/** DBusArgsToJsvalArray converts the dbus message arguments to a jsval array. it returns the number
 * of arguments in len.
 */
static JSBool DBusArgsToJsvalArray(JSContext *ctx, DBusMessage* message, int* len, jsval** vals) {
    DBusMessageIter iter;
    int length = 0;
    jsval* vector;

    dbus_message_iter_init(message, &iter);
    while (dbus_message_iter_next(&iter)) {
        length++;
    }
    dbg3_err("length = " << length << endl);

    vector = new jsval[length];

    dbus_message_iter_init(message, &iter);

    DBusMarshalling::getVariantArray(ctx, &iter, &length, vector);
    dbg3_err("length after unmarsh = " << length << endl);

    *len = length;
    *vals = vector;
    return JS_TRUE;
}
/** DBusArgsToJsvalArrayPreprend acts as DBusArgsToJsvalArray, but saves the first slot for
 * method call meta-data. vector[0] can be filled by handleMethodcall with opath or at a later
 * time with a dbusmesssage object.
 */
static JSBool DBusArgsToJsvalArrayPreprend(JSContext *ctx, DBusMessage* message, int* len, jsval** vals) {
    DBusMessageIter iter;
    int length = 0;
    jsval* vector;

    dbus_message_iter_init(message, &iter);
    while (dbus_message_iter_next(&iter)) {
        length++;
    }
    dbg3_err("length = " << length << endl);

    vector = new jsval[length+1];

    dbus_message_iter_init(message, &iter);

    DBusMarshalling::getVariantArray(ctx, &iter, &length, vector+1);
    dbg3_err("length after unmarsh = " << length << endl);

    *len = length+1;
    *vals = vector;
    return JS_TRUE;
}

JSBool DBusArgsToArrayObj(JSContext *ctx, DBusMessage* message, int* len, JSObject** argObj) {
    jsval* vec = 0;
    int l = 0;

    DBusArgsToJsvalArray(ctx, message, &l, &vec);

    JSObject * args = JS_NewArrayObject(ctx, l, vec);

    delete vec;
    *argObj = args;
    return JS_TRUE;
}


#define ensure_val_is_object_or_fail(con, message, ret, val, msg)  \
    { bool send = false; \
    if ((ret) == JS_FALSE ) { \
        dbg2_err("handleMethodCall - ret is false: " << msg); \
        send = true; \
    } else \
    if (JSVAL_IS_VOID(val) || JSVAL_IS_NULL(val)) { \
        dbg2_err("handleMethodCall - val is nil or void: " << msg); \
        send = true; \
    } else \
    if (!JSVAL_IS_OBJECT(val)) { \
        dbg2_err("val must be an object: " << msg); \
        send = true; \
    } \
    if (send) { \
        dbus_uint32_t ser=0; \
        DBusMessage* err = dbus_message_new_error(message, "at.beko.Error.ObjectNotFound", msg); \
        dbus_connection_send(con, err, &ser); \
        dbus_connection_flush(con); \
        dbus_message_unref(err); \
        return DBUS_HANDLER_RESULT_HANDLED; \
        }\
    }

/** handleMethodCall is triggered if a method call addresses this connection or it does not have
 * a fixed destination set. Anyway, we forward the call to the js layer for further handling.
 *
 */
static DBusHandlerResult handleMethodCall(DBusConnection* connection, DBusMessage* message, dbusData* dta) {
    JSObject* bus = dta->self;
    jsval val;
    JSBool ret;
    const char* method = dbus_message_get_member(message);
    const char* path = dbus_message_get_path(message);

    string key = dbus_message_get_path(message);
    key += "_";
    key += dbus_message_get_interface(message);

    cerr << "*** bus obj " << hex << bus << dec << endl;
    cerr << "*** looking for key " << key << endl;

    ret = JS_GetProperty(dta->ctx, bus, "keys", &val);
    ensure_val_is_object_or_fail(connection, message, ret, val, "service keys hash");
    JSObject* keys = JSVAL_TO_OBJECT(val);

    map<string, expInfo*>::iterator info = dta->expObjects.find(key);
    JSObject* srv = NULL;
    if (info != dta->expObjects.end()) {
        srv = info->second->o;
        cerr << "found service entry at " << key << " o " << hex << srv << dec << endl;
    } else {
        cerr << "no service match for " << key << endl;
        info = dta->expObjects.begin();
        while (info != dta->expObjects.end()) {
            cerr << " exi " << info->first << endl;
            info++;
        }
    }
    ret = JS_TRUE;
    val = OBJECT_TO_JSVAL(srv); // hack on
    ensure_val_is_object_or_fail(connection, message, ret, val, "service object");

    // check for function name:
    JS_GetProperty(dta->ctx, srv, method, &val);
    ensure_val_is_object_or_fail(connection, message, ret, val, "service method");
    JSObject* cbObj = JSVAL_TO_OBJECT(val);
    if (!JS_ObjectIsFunction(dta->ctx, cbObj)) {
        cerr << " key->value should be a callback.\n";
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    dbg2_err("handleMethodCall - CALL!");
    JSFunction* callback = JS_ValueToFunction(dta->ctx, val);
    fflush(stdout);

    int argc;
    JSObject* methArg;
    jsval* argv;

    DBusArgsToJsvalArrayPreprend(dta->ctx, message, &argc, &argv);
    JSString* sp = JS_NewStringCopyZ(dta->ctx, path);
    argv[0] = STRING_TO_JSVAL(sp);

    dbus_uint32_t serial = 0;

    //  finally - the function call!
    jsval rval;

    cerr << "func call argc " << argc << endl;
    ret = JS_CallFunction(dta->ctx, srv, callback, argc, argv, &rval);
    cerr << "function return value " << ret << endl;

    if (ret == JS_FALSE) {
        cerr << "FALSE" << endl;
        // TODO: Send a DBUS Exception
    }

    // create a reply from the message
    DBusMessage* reply = dbus_message_new_method_return(message);
    // append result.
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply, &iter);

    // now check the return value
    if (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval)) {
        // no return value - nothing to do
        dbg2_err("no return value");
	} else if ( JSVAL_IS_STRING(rval) ) {
		DBusMarshalling::marshallBasicValue(dta->ctx, cbObj, &iter, &rval);
    } else {
        ret = DBusMarshalling::marshallVariant(dta->ctx, cbObj, &iter, &rval);
    }
	
    // send the reply && flush the connection
    if (!dbus_connection_send(connection, reply, &serial)) {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1); /// TODO: define strategy..!
    } else {
		dbg3_err( "serial " << serial)  ;
    }

    dbus_connection_flush(connection);

    // free the reply
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

static const string corePath = "/org/freedesktop/DBus";
static const string coreInterface = "org.freedesktop.DBus";

static DBusHandlerResult handleCoreSignal(DBusConnection* connection, DBusMessage* message, dbusData* dta) {
    const char *name;
    name = dbus_message_get_member(message);
    DBusMessageIter iter;

    if (strcmp("NameAcquired", name) == 0) {
        string n;
        char *val;

        dbus_message_iter_init(message, &iter);
        dbus_message_iter_get_basic(&iter, &val);
        dbg2_err("string [" << val << "]");
        n = val;
        dta->busNames[val] = NULL;
    } else if (strcmp("NameOwnerChanged", name) == 0) {
    } else {
        dbg2_err("unhandled core signal :: " << name);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult
filter_func(DBusConnection* connection, DBusMessage* message, void* user_data) {
    dbusData* dta = (dbusData*) user_data;
    int type = dbus_message_get_type(message);
    const char *interface;
    const char *name;
    const char *path;
    string key;
    DBusMessageIter iter;

    interface = dbus_message_get_interface(message);
    name = dbus_message_get_member(message);
    path = dbus_message_get_path(message);

    if (type == DBUS_MESSAGE_TYPE_SIGNAL) {

        if (corePath.compare(path) == 0 && coreInterface.compare(interface) == 0) {
            dbg2_err("handle core signal :: " << name);
            return handleCoreSignal(connection, message, dta);
        }

        key = path;
        key += "/";
        key += interface;
        key += "/";
        key += name;
        dbg2_err("filter_func key:: " << key);

        map<string, callbackInfo*>& m = dta->sigHandlers;
        map<string, callbackInfo*>::iterator it;
#if DEBUG_LEVEL>3
        cout << "----- filter handlers:" << endl;
        it = m.begin();
        while (it != m.end()) {
            cout << it->first << " ---> " << it->second->match << endl;
            it++;
        }
        cout << "----- filter handlers." << endl;
#endif

#ifdef DEBUG_DBUS_LOWLEVEL
        dbg2_err("signature " << dbus_message_get_signature(message));
        dbus_message_iter_init(message, &iter);
        printer(&iter);
#endif
        if ((it = m.find(key)) != m.end()) {
            dbg2_err("----- found in map!!!");
        } else {

            dbg2_err("----- not found in map.");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        JSObject* busObj = dta->self;
        JSFunction *callback = NULL;
        jsval valp;

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
        dbg3_err("length = " << length );

        jsval* vector = new jsval[length];

        dbus_message_iter_init(message, &iter);

        DBusMarshalling::getVariantArray(dta->ctx, &iter, &length, vector);
        dbg3_err( "length after unmarsh = " << length );

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
    } else if (type == DBUS_MESSAGE_TYPE_METHOD_CALL) {
        //we are addressed?!
        const char* dest = dbus_message_get_destination(message);

        map<string, void*>::iterator iter;
        if (dest && dta->busNames.find(dest) != dta->busNames.end()) {
            //cerr << "holla, wir sans\n";
        } else {
            //cerr << "fnk. no for " << dest << " " << name << endl;
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        return handleMethodCall(connection, message, dta);

    } else {
#ifdef DEBUG_DBUS_LOWLEVEL
        switch (dbus_message_get_type(message)) {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
                dbg2_err("dbus ignore method call " << path << " " << interface << " " << name);
                break;
            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                dbg2_err("dbus ignore method return" << path << " " << interface << " " << name);
                break;
            case DBUS_MESSAGE_TYPE_ERROR:
                dbg2_err("dbus ignore method error" << path << " " << interface << " " << name);
                goto fail;
            default:
                dbg2_err("dbus ignore msg type " << dbus_message_get_type(message));
                goto fail;
        }
        dbg2_err("signature " << dbus_message_get_signature(message));
        dbus_message_iter_init(message, &iter);
        while (dbus_message_iter_has_next(&iter)) {
            printer(&iter);
            dbus_message_iter_next(&iter);
        }
        dbg2_err("signature done ");
#endif
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
    length = dbus_iter_length(&iter);
    dbus_message_iter_init(message, &iter);

    cerr << "length = " << length << endl;
    {
        jsval rval;
        vector = new jsval[length];

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

/** setName sets the dbus name of the active connection. this is used internally to filter 
 * method calls.
 */
static JSBool DBus_setName(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);

    check_args(argc == 1, "argc must be ==1!");
    check_args(JSVAL_IS_STRING(argv[0]), "arg[0] must be a string (unique name)!");

    JSString* uname = JSVAL_TO_STRING(argv[0]);

    dta->unique_name = JS_GetStringBytes(uname);
    dbg2_err("MY NAME IS " << dta->unique_name);
    return JS_TRUE;
}

/*************************************************************************************************/

/** DBus Constructor */
static JSBool DBusConstructor(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (argc == 0) {
        dbusData* dta = NULL;

        //dta = (dbusData*) calloc(sizeof (dbusData), 1);
        dta = new( dbusData);

        cerr << "new DBus obj " << hex << dta << dec << endl;

        if (!JS_SetPrivate(ctx, obj, dta))
            return JS_FALSE;
        return JS_TRUE;
    }
    return JS_FALSE;
}

static void DBusDestructor(JSContext *ctx, JSObject *obj) {
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, obj);
    cerr << "delete DBus obj " << hex << dta << dec << endl;
    if (dta) {
        free(dta->name);

        dbus_connection_remove_filter(dta->connection,
                filter_func, dta);
        if (dta->connection) {
            dbus_connection_unref(dta->connection);
            dta->connection = 0;
        }
        
        map<string, callbackInfo*>::iterator cbI = dta->sigHandlers.begin();
        while (cbI != dta->sigHandlers.end()) {
            delete cbI->second;
            cbI++;
        }

        map<string, expInfo*>::iterator obI = dta->expObjects.begin();
        while (obI != dta->expObjects.end()) {
            delete obI->second;
            obI++;
        }

        map<string, void*>::iterator bnI = dta->busNames.begin();
        while (bnI != dta->busNames.end()) {
            //delete bnI->second;
            bnI++;
        }
        delete (dta);
    }
}

static JSBool DBusDictConstructor(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if (!argc) {
        return JS_TRUE;
    }
    if (argc==2 || argc == 3) {
        // set a key/value pair
        fail_if_not(JSVAL_IS_STRING(argv[0]), "key must be a string");
        if (argc==3)
            fail_if_not(JSVAL_IS_STRING(argv[2]), "typecast must be a string");

        JS_SetProperty(ctx,obj,"key", &argv[0]);
        JS_SetProperty(ctx,obj,"value", &argv[1]);

        if (argc==3) {
            jsval typeval;
            JS_SetProperty(ctx,obj,"typecast", &argv[2]);
        }
        return JS_TRUE;
    }
    dbg_err("construct a dbus dict either empty or with a key/value pair (plus optional typecast for value)!");
    return JS_FALSE;
}


static JSBool DBusObjConstructor(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return JS_TRUE;
}

static void DBusObjDestructor(JSContext *ctx, JSObject *obj) {
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
    { "setName", DBus_setName, 0, 0, 0}, // sets the unique name captured by a NameAcquired signal.
    { "call", DBus_callMethod, 0, 0, 0},
    { "callArrayArg", DBus_callMethodAA, 0, 0, 0},
    { "export", DBus_exportObject, 0, 0, 0},
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

JSClass DBusDict_jsClass = {
    "DBusDict", 0,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, 0, // convert / finalize
    0, 0, 0, 0,
    0, 0, 0, 0
};

JSClass DBusTypeCast_jsClass = {
    "DBusTypeCast", 0,
    JS_PropertyStub, JS_PropertyStub, // add/del prop
    JS_PropertyStub, JS_PropertyStub, // get/set prop
    JS_EnumerateStub, JS_ResolveStub, // enum / resolve
    JS_ConvertStub, 0, // convert / finalize
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
    check_args((argc == 0), "must not pass an argument!\n");

    JSObject *bus = JS_ConstructObject(ctx, &DBus_jsClass, NULL, NULL);
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, bus);
    dta->ctx = ctx;
    dta->self = bus;
    dta->root.name = "";

    if (JS_FALSE == _DBus_connect(dta, DBUS_BUS_SYSTEM)) {
        return JS_FALSE;
    }
    dta->name = strdup("system");

    *rval = OBJECT_TO_JSVAL(bus);
    return JS_TRUE;
}

static JSBool DBus_s_sessionBus(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    check_args((argc == 0), "must not pass an argument!\n");

    JSObject *bus = JS_ConstructObject(ctx, &DBus_jsClass, NULL, NULL);
    dbusData* dta = (dbusData *) JS_GetPrivate(ctx, bus);
    dta->ctx = ctx;
    dta->self = bus;
    dta->root.name = "";

    JSObject* keys = JS_NewObject(ctx, &DBus_jsClass, NULL, NULL);
    jsval kv = OBJECT_TO_JSVAL(keys);
    JS_SetProperty(ctx, bus, "keys", &kv);

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
            &DBusDict_jsClass,
            DBusDictConstructor, 0, NULL, // properties
            0, NULL, 0);
    //typecast helper object
    JS_InitClass(ctx, obj, NULL,
            &DBusTypeCast_jsClass,
            DBusObjConstructor, 0, NULL, // properties
            0, NULL, 0);
    // result object
    JS_InitClass(ctx, obj, NULL,
            &DBusResult_jsClass,
            DBusObjConstructor,
            0, NULL, // properties
            0, NULL, 0);

    // error obj
    return JS_InitClass(ctx, obj, NULL,
            &DBusError_jsClass,
            DBusObjConstructor, 0, NULL, // properties
            0, NULL, 0);

}

JSBool DBusExit(JSContext *ctx) {
    return JS_TRUE;
}

/*************************************************************************************************/
extern JSObject* GlibInit(JSContext *ctx, JSObject *obj);
extern int GlibExit(JSContext *ctx);

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

    int js_DSO_unload(JSContext *context) {
        if (!DBusExit(context)) {
            fprintf(stderr, "Cannot finalize DBus\n");
            return EXIT_FAILURE;
        }

        if (!GlibExit(context)) {
            fprintf(stderr, "Cannot finalize Glib\n");
            return EXIT_FAILURE;
        }

        return JS_TRUE;
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */
