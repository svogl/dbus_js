
#include <jsapi.h>
#include <dbus/dbus.h>

#include <string>
#include <map>

using namespace std;

class DBusMarshalling 
{
public:

    static JSBool appendArgs(JSContext *cx, JSObject *obj,
				DBusMessage*     message,
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

    static JSBool marshallVariant(JSContext *ctx, JSObject *obj, DBusMessage *, DBusMessageIter *, jsval * );

private:

    static JSBool marshallTypedJSObject(JSContext* ctx, JSObject* aObj, DBusMessageIter* iter);

    static JSBool marshallJSObject(JSContext* ctx, JSObject* aObj, DBusMessageIter* iter);
    static JSBool marshallJSProperty(JSContext* ctx, JSObject* aObj, jsval& propval, DBusMessageIter* iter, const JSBool& isVariant);
    static JSBool JSObjectHasVariantValues(JSContext* ctx, JSObject* obj);

    static JSBool unMarshallIter(JSContext *ctx,int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallBasic(JSContext *ctx,int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallArray(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);
    static JSBool unMarshallDict(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val);

    /*
    static const int getDataTypeSize(PRUint16);
    static const int getDataTypeAsDBusType(PRUint16);
    static const char *getDBusTypeAsSignature(int);
*/
};



extern JSClass DBusResult_jsClass;
extern JSClass DBusError_jsClass;



/***********************/
/* from the interface idl:
  nsIVariant CallMethod(in PRUint16 aBusType,
                        in AUTF8String aServiceName,
                        in AUTF8String aObjectPath,
                        in AUTF8String aInterfaceName,
                        in AUTF8String aMethodName,
                        in unsigned long aArgsLength,
                        [array, size_is(aArgsLength)]
                        in nsIVariant aArgs,
                        in IJSCallback aCallback);

  void EmitSignal(in PRUint16 aBusType,
                  in AUTF8String aObjectPath,
                  in AUTF8String aInterfaceName,
                  in AUTF8String aSignalName,
                  in unsigned long aArgsLength,
                  [array, size_is(aArgsLength)]
                  in nsIVariant aArgs);

  unsigned long ConnectToSignal(in PRUint16 aBusType,
                                in AUTF8String aServiceName,
                                in AUTF8String aObjectPath,
                                in AUTF8String aInterfaceName,
                                in AUTF8String aSignalName,
                                in IJSCallback aCallback);

  void DisconnectFromSignal(in PRUint16 aBusType,
                            in unsigned long aId);

  boolean RequestService(in PRUint16 aBusType,
                         in AUTF8String aServiceName);
*/
/***********************/




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
    }

#define MOZJSDBUS_CALL_OOMCHECK(_exp)      \
        if (!(_exp)) {                     \
            return JS_FALSE; \
        }                                  \



