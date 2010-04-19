
#include <jsapi.h>
#include <dbus/dbus.h>

#include <string>
#include <map>

using namespace std;

class DBusMarshalling {
public:

    static JSBool appendArgs(JSContext *cx, JSObject *obj,
            DBusMessage* message,
            DBusMessageIter* iter,
            uintN argc, jsval *argv);

    /**getVariantarray parses the arguments found in iter. it needs
     * as its input a  jsval array of length elements, matching the
     * number of array elements in iter (you might want to iterate over
     * the array first to get the number.
     *
     * @param ctx the Javascript context
     * @param iter the dbus message iter from which to read values
     * @param length - length of the vector - will be set to the number of elements parsed!
     * @param vector - the argument array to fill.
     * @return number of parsed elements
     */
    static JSBool getVariantArray(JSContext *ctx, DBusMessageIter *iter, int *length, jsval* vector);

    static JSBool marshall(DBusMessage **);

    static JSBool marshallVariant(JSContext *ctx, JSObject *obj, /*DBusMessage *,*/ DBusMessageIter *, jsval *);

    static JSBool marshallBasicValue(JSContext *ctx, JSObject *obj, /*DBusMessage *,*/ DBusMessageIter *, jsval *basicVal);
private:

    static JSBool marshallDictObject(JSContext *ctx, JSObject *obj, /*DBusMessage *,*/ DBusMessageIter *);

    static JSBool marshallTypedJSObject(JSContext* ctx, JSObject* aObj, DBusMessageIter* iter);

    static JSBool marshallJSObject(JSContext* ctx, JSObject* aObj, DBusMessageIter* iter);
    static JSBool marshallJSArray(JSContext* ctx, JSObject* aObj, DBusMessageIter* iter);
    static JSBool marshallJSProperty(JSContext* ctx, JSObject* aObj, jsval& propval, DBusMessageIter* iter, const JSBool& isVariant);
    static JSBool JSObjectHasVariantValues(JSContext* ctx, JSObject* obj);

    static JSBool unMarshallIter(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallBasic(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallArray(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallDict(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);

    /*
    static const int getDataTypeSize(PRUint16);
    static const int getDataTypeAsDBusType(PRUint16);
    static const char *getDBusTypeAsSignature(int);
     */
};


/** These are used in the marshaler for passing back objects.
 *
 */
extern JSClass DBusResult_jsClass; ///< js dbus result object
extern JSClass DBusError_jsClass; ///< js dbus error object

/************** helpers */
#define check_args(assert, ...) if (!(assert)) { \
		fprintf(stderr, "%s:%d :: ", __FUNCTION__, __LINE__ );\
		fprintf(stderr, __VA_ARGS__);\
		return JS_FALSE; \
	}

#define check_dbus_error(error) \
    \
    if (dbus_error_is_set(&(error))) { \
        fprintf(stderr, "** DBUS ERROR **\n   Name: %s\n   Message: %s\n", \
               error.name, error.message); \
        dbus_error_free(&error); \
        return JS_FALSE; \
    } // TODO: raise dbus exception / error object instead!

#define enforce_notnull(_exp)      \
         check_args( (_exp), "are we oom?")

extern int indent;
#define DEBUG_LEVEL 3
#define DEBUG_DBUS_LOWLEVEL

#define ind() { int _i=indent; while(_i--) { cerr << "  ";} }

#define dbg(x) if (DEBUG_LEVEL>=1) { ind(); x ; }
#define dbg2(x) if (DEBUG_LEVEL>=2) {ind();  x ; }
#define dbg3(x) if (DEBUG_LEVEL>=3) {ind();  x ; }

#define  dbg_err(x) if (DEBUG_LEVEL>=1) {ind();  cerr << __FUNCTION__ << ":" << __LINE__ << " " << x << endl; }
#define dbg2_err(x) if (DEBUG_LEVEL>=2) {ind();  cerr << __FUNCTION__ << ":" << __LINE__ << " " << x << endl; }
#define dbg3_err(x) if (DEBUG_LEVEL>=3) {ind();  cerr << __FUNCTION__ << ":" << __LINE__ << " " << x << endl; }

/** walk over all elements to count the number of elements.
 * Side-effects: you have to re-init the iterator after calling this function!
 */
static inline int dbus_iter_length(DBusMessageIter* iter) {
    int current_type;
    int length=0;
    while ((current_type = dbus_message_iter_get_arg_type (iter)) != DBUS_TYPE_INVALID) {
        length++;
        dbus_message_iter_next (iter);
    }
    return length;
}

extern JSClass DBusDict_jsClass;
extern JSClass DBusResult_jsClass;
#include <iostream>
using namespace std;

static inline bool isDictObject(JSContext* ctx, JSObject* obj) { 
    JSClass* cls =  JS_GetClass(ctx, obj);
//	cerr << hex << " isDict? cls= " << cls << " against " << &DBusDict_jsClass << dec << endl;
	return (cls == &DBusDict_jsClass);
}

static inline bool jsvalIsDictObject(JSContext* ctx, jsval propkey) { 
	JSObject* obj = JSVAL_TO_OBJECT(propkey);
	return isDictObject(ctx, obj);
}


