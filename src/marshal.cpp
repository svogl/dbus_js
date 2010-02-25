#include "marshal.h"
#include <string.h>
#include <iostream>

using namespace std;

JSBool
DBusMarshalling::appendArgs(JSContext *cx, JSObject *obj,
        DBusMessage *message,
        DBusMessageIter *iter,
        uintN argc, jsval *argv) {
    JSBool ret = JS_TRUE;
    for (uintN i = 0; i < argc && ret == JS_TRUE; i++) {
        jsval* val = &argv[i];

        ret = marshallVariant(cx, obj, message, iter, val);
    }
    return JS_TRUE;
}

JSBool DBusMarshalling::marshallVariant(JSContext *cx, JSObject *obj, DBusMessage *message, DBusMessageIter *iter, jsval * val) {
    dbus_int32_t ret;

    if (val == NULL) {
        fprintf(stderr, "NULL value detected - skipping!\n");
        return JS_TRUE;
    }

    int tag = JSVAL_TAG(*val);
    if (JSVAL_IS_INT(*val)) {
        jsint iv = JSVAL_TO_INT(*val);
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_INT32,
                &iv);
    } else if (JSVAL_IS_STRING(*val)) {
        JSString* sv = JSVAL_TO_STRING(*val);
        /*
        jschar* jv = JS_GetStringChars(sv);
        fprintf(stderr, ".. a string  %02x %02x %02x %02x!\n",jv[0],jv[1],jv[2],jv[3]);
         */
        char* cv = JS_GetStringBytes(sv);
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_STRING,
                &cv);
    } else if (JSVAL_IS_BOOLEAN(*val)) {
        JSBool bv = JSVAL_TO_BOOLEAN(*val);
        dbus_int32_t d = bv;
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_BOOLEAN,
                &d);
    } else if (JSVAL_IS_DOUBLE(*val)) {
        jsdouble* db = JSVAL_TO_DOUBLE(*val);
        double d = (double) * db; // convert from float64
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_DOUBLE,
                &d);
    } else
        switch (tag) {
            case JSVAL_OBJECT:
            {
                // parse object....
                if (JSVAL_IS_OBJECT(*val))
                    fprintf(stderr, "its an object!\n");
                JSObject* ov = JSVAL_TO_OBJECT(*val);
                JSBool found;
                JSBool foundVal;

                JSBool br = marshallJSObject(cx, ov, iter);

            }
                break;
            default:
                fprintf(stderr, "illegal type (tag=%d) !\n", tag);
                if (JSVAL_IS_NUMBER(*val))
                    fprintf(stderr, "its a number !\n");
                if (JSVAL_IS_DOUBLE(*val))
                    fprintf(stderr, "its a double !\n");
                if (JSVAL_IS_INT(*val))
                    fprintf(stderr, "its a int !\n");
                if (JSVAL_IS_OBJECT(*val))
                    fprintf(stderr, "its a object !\n");
                if (JSVAL_IS_STRING(*val))
                    fprintf(stderr, "its a string !\n");
                if (JSVAL_IS_BOOLEAN(*val))
                    fprintf(stderr, "its a bool !\n");
                if (JSVAL_IS_NULL(*val))
                    fprintf(stderr, "its a null !\n");
                if (JSVAL_IS_VOID(*val))
                    fprintf(stderr, "its a void !\n");
                //ret = JS_FALSE;
        }
    return ret;
}

JSBool
DBusMarshalling::JSObjectHasVariantValues(JSContext* cx,
        JSObject* aObj) {
    //Always use "a{sv}" as the signature for now
    return JS_TRUE;
}

JSBool
DBusMarshalling::marshallJSObject(JSContext* cx,
        JSObject* aObj,
        DBusMessageIter* iter) {
    DBusMessageIter container_iter;
    int rv;

    JSBool hasVariantValues = JSObjectHasVariantValues(cx, aObj);
    MOZJSDBUS_CALL_OOMCHECK(
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
            DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
            DBUS_TYPE_STRING_AS_STRING
            DBUS_TYPE_VARIANT_AS_STRING
            DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
            &container_iter));

    JSObject* js_iter = JS_NewPropertyIterator(cx, aObj);
    jsid propid;
    if (!JS_NextProperty(cx, js_iter, &propid)) {
        return JS_FALSE;
    }

    while (propid != JSVAL_VOID) {
        jsval propval;
        if (!JS_IdToValue(cx, propid, &propval))
            return JS_FALSE;

        rv = marshallJSProperty(cx, aObj, propval, &container_iter,
                hasVariantValues);

        if (!JS_NextProperty(cx, js_iter, &propid)) {
            return JS_FALSE;
        }
    }

    dbus_message_iter_close_container(iter, &container_iter);
    printf("marshalled object\n");
    return JS_TRUE;
}

JSBool
DBusMarshalling::marshallTypedJSObject(JSContext* ctx,
        JSObject* obj,
        DBusMessageIter* iter) {
    JSBool found;
    JSBool ret;
    jsval type;
    ret = JS_HasProperty(ctx, obj, "dbusType", &found);

    check_args(ret == JS_TRUE, "hasProperty failed!");

    if (!found) {
        // general object.
        // TODO: serialize as json object later on....
        return JS_FALSE;
    }
    ret = JS_LookupProperty(ctx, obj, "dbusType", &type);
    check_args(ret == JS_TRUE, "Property lookup failed!");
    check_args(JSVAL_IS_STRING(type), "dbusType is not a String!");

    JSString *stype = JSVAL_TO_STRING(type);
    char* tval = JS_GetStringBytes(stype);


    if (strcmp("dict", tval) == 0) {

    } else if (strcmp("oPath", tval) == 0) {
    } else if (strcmp("typeSig", tval) == 0) {
    } else if (strcmp("int16", tval) == 0) {
    } else if (strcmp("uint16", tval) == 0) {
    } else if (strcmp("uint32", tval) == 0) {
    } else if (strcmp("int64", tval) == 0) {
    } else if (strcmp("uint64", tval) == 0) {
    } else {
        // general object.
        // TODO: serialize as json object later on....
    }
    /*
   ret = dbus_message_iter_append_basic(iter,
                                   DBUS_TYPE_BOOLEAN,
                                   &d);
     */
    return JS_TRUE;
}

JSBool
DBusMarshalling::marshallJSProperty(JSContext* cx,
        JSObject* aObj,
        jsval& propval,
        DBusMessageIter* iter,
        const JSBool& isVariant) {
    DBusMessageIter dictentry_iter;
    DBusMessageIter variant_iter;
    JSBool hasCast = JS_FALSE;
    dbus_int32_t dbus_type = DBUS_TYPE_INVALID;

    printf("marshalljsproperty\n");

    JSString* propname = JS_ValueToString(cx, propval);

    JS_GetUCProperty(cx, aObj, JS_GetStringChars(propname),
            JS_GetStringLength(propname), &propval);
    /*
        JS_GetProperty(cx, aObj, JS_GetStringChars(propname),
                         JS_GetStringLength(propname), &propval);
     */
    dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY,
            NULL, &dictentry_iter);

    /**** for the time being, we use utf8 strings.... */
    /*
nsDependentString propdepstring(JS_GetStringChars(propname),
                                JS_GetStringLength(propname));
nsCAutoString propstring = 
    PromiseFlatCString(NS_ConvertUTF16toUTF8(propdepstring));

const char* propstringbuf = propstring.get();
printf("* PROPERTY: %s\n", propstringbuf);
	
     */

    const char* propstringbuf = JS_GetStringBytes(propname);
    const char * typeval = propstringbuf;

    if (propstringbuf[3] == '_') {
        if (strncmp("i32_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_INT32;
        } else if (strncmp("u32_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_UINT32;
        } else if (strncmp("i16_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_INT16;
        } else if (strncmp("u16_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_UINT16;
        } else if (strncmp("i64_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_INT64;
        } else if (strncmp("u64_", typeval, 4) == 0) {
            dbus_type = DBUS_TYPE_UINT64;
        } else {
            fprintf(stderr, "unknown type code %s!\n", propstringbuf);
        }
        propstringbuf += 4;
    }

    MOZJSDBUS_CALL_OOMCHECK(
            dbus_message_iter_append_basic(&dictentry_iter,
            DBUS_TYPE_STRING,
            &propstringbuf));

    JSType type = JS_TypeOfValue(cx, propval);
    switch (type) {
        case JSTYPE_NUMBER:
        {
            /*Impossible to tell what DBus type JSTYPE_NUMBER
              corresponds to. Assume INT32 for now.
             */
            int intVal;
            JS_ValueToInt32(cx, propval, &intVal);
            printf("  VALUE: %d\n", intVal);
            if (dbus_type == DBUS_TYPE_INVALID || isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_INT32_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_INT32,
                        &intVal));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&dictentry_iter,
                        DBUS_TYPE_INT32,
                        &intVal));
            }
            break;
        }
        case JSTYPE_FUNCTION:
        case JSTYPE_STRING:
        {
            //Treat functions as strings for now.
            JSString* propvaljsstring = JS_ValueToString(cx, propval);
            /* TODO: UTF16
        nsDependentString propvaldepstring(
            JS_GetStringChars(propvaljsstring),
            JS_GetStringLength(propvaljsstring));
        nsCAutoString propvalstring =
            PromiseFlatCString(NS_ConvertUTF16toUTF8(propvaldepstring));
        const char* propvalstringbuf = propvalstring.get();
             */
            const char* propvalstringbuf = JS_GetStringBytes(propvaljsstring);
            printf("  VALUE: %s\n", propvalstringbuf);
            if (dbus_type == DBUS_TYPE_INVALID || isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_STRING_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_STRING,
                        &propvalstringbuf));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                if (dbus_type != DBUS_TYPE_INVALID) {
                    MOZJSDBUS_CALL_OOMCHECK(
                            dbus_message_iter_append_basic(&dictentry_iter,
                            dbus_type,
                            &propvalstringbuf));
                }
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&dictentry_iter,
                        DBUS_TYPE_STRING,
                        &propvalstringbuf));
            }
            break;
        }
        case JSTYPE_BOOLEAN:
        {
            JSBool propbool;
            JS_ValueToBoolean(cx, propval, &propbool);
            printf("  VALUE: %s\n", propbool ? "true" : "false");
            if (isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_BOOLEAN_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_BOOLEAN,
                        &propbool));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&dictentry_iter,
                        DBUS_TYPE_BOOLEAN,
                        &propbool));
            }
            break;
        }
        case JSTYPE_OBJECT:
        {
            /*JSObject* childObject;
            JS_ValueToObject(cx, propval, &childObject);
            JSBool rv = marshallJSObject(cx, childObject, &dictentry_iter);
            NS_ENSURE_SUCCESS(rv, rv);
             * */
            cerr << "implement object?! at " << __LINE__ << endl;
            break;
        }
        case JSTYPE_VOID:
        default:
            return JS_FALSE;
    }
    dbus_message_iter_close_container(iter, &dictentry_iter);
    return JS_TRUE;
}

/*************************************************************************/
JSBool DBusMarshalling::getVariantArray(JSContext *ctx, DBusMessageIter *iter, int *length, jsval* vector) {
    int len = 0;
    int current_type;
    JSBool ret = JS_TRUE;

    while ((current_type = dbus_message_iter_get_arg_type(iter))
            != DBUS_TYPE_INVALID && ret == JS_TRUE) {
        cerr << "gVA " << len << " " << current_type << "  sig = ";
    cerr << dbus_message_iter_get_signature(iter) << endl;
        ret = unMarshallIter(ctx, current_type, iter, &vector[len]);
        dbus_message_iter_next(iter);
        len++;
    }
    *length = len;
    return ret;
}

JSBool DBusMarshalling::unMarshallIter(JSContext *ctx, int current_type, DBusMessageIter *iter, jsval* val) {
    if (dbus_type_is_basic(current_type)) {
         cerr << "basic type.. ";
        return unMarshallBasic(ctx, current_type, iter, val);
    } else if (dbus_type_is_container(current_type)) {
        cerr << "container type.. " ;
        switch (current_type) {
            case DBUS_TYPE_ARRAY:
            {
                cerr <<  "array";
                return unMarshallArray(ctx, current_type, iter, val);
            }
            case DBUS_TYPE_DICT_ENTRY:
            {
                cerr << "dict.. "; //@TODO implement dict type
                return unMarshallDict(ctx, current_type, iter, val);
            }
            default:
            {
                cerr << "unknown container type " << (char)current_type << "\n";
                return JS_FALSE;
            }
        }
    } else {
        cerr << "what's this??? " << dbus_message_iter_get_signature(iter) << endl;
    }
    return JS_TRUE;
}

JSBool DBusMarshalling::unMarshallBasic(JSContext *ctx, int type, DBusMessageIter *iter, jsval* value) {
    switch (type) {
        case DBUS_TYPE_BOOLEAN:
        {
            JSBool val;
            dbus_message_iter_get_basic(iter, &val);
            *value = BOOLEAN_TO_JSVAL(val);
            break;
        }
        case DBUS_TYPE_BYTE:
        {
            char val; //PRUint8?
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            break;
        }
        case DBUS_TYPE_INT16:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsInt16(val);
            break;
        }
        case DBUS_TYPE_UINT16:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsUint16(val);
            break;
        }
        case DBUS_TYPE_INT32:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsInt32(val);
            break;
        }
        case DBUS_TYPE_UINT32:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsUint32(val);
            break;
        }
        case DBUS_TYPE_INT64:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsInt64(val);
            cerr << "POTENTIAL OVERFLOW \n";
            break;
        }
        case DBUS_TYPE_UINT64:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsUint64(val);
            cerr << "POTENTIAL OVERFLOW \n";
            break;
        }
        case DBUS_TYPE_DOUBLE:
        {
            double val;
            dbus_message_iter_get_basic(iter, &val);
            //variant->SetAsDouble(val);
            *value = DOUBLE_TO_JSVAL(val);
            break;
        }
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_OBJECT_PATH:
        {
            char *val;
            dbus_message_iter_get_basic(iter, &val);
 cerr << "string [" << val << "]";
            JSString *str = JS_NewStringCopyZ(ctx, val);
            *value = STRING_TO_JSVAL(str);
            break;
        }
        default:
        {
            return JS_FALSE;
        }
    }
    cerr<<endl;
    return JS_TRUE;
}

JSBool
DBusMarshalling::unMarshallArray(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val)
{
    JSBool ret;
    DBusMessageIter subiter;
    int length=0;
    jsval* vector;

    cerr << ">> array " << endl;

    dbus_message_iter_recurse(iter, &subiter);
    while (dbus_message_iter_next(&subiter)) {
        length++;
    }
    vector = new jsval[length]; //TODO: memleak?
    dbus_message_iter_recurse(iter, &subiter);
    cerr << "unMarsch len = " << length << " sig = " ;
    cerr << dbus_message_iter_get_signature(&subiter) << endl;

    ret = getVariantArray(ctx, &subiter, &length, vector);
    cerr << "unMarsch len = " << length << endl;
    if (ret == JS_FALSE)
          return ret;
    cerr << "unMarsch len = " << length << endl;

    if (length > 0) {
        //rv = variant->SetAsArray(nsIDataType::VTYPE_INTERFACE_IS,
        //        &NS_GET_IID(nsIVariant), length, array);

        JSObject * args = JS_NewArrayObject(ctx, length, vector);
        *val = OBJECT_TO_JSVAL(args);
    }
    return JS_TRUE;
}


JSBool
DBusMarshalling::unMarshallDict(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val)
{
    JSBool ret;
    DBusMessageIter subiter;
    int length=0;
    int current_type;
    jsval* vector;
    cerr << "dict entry" << endl;
    dbus_message_iter_recurse(iter, &subiter);
    while (dbus_message_iter_next(&subiter)) {
        length++;
    }
    vector = new jsval[length]; //TODO: memleak?

    cerr << "unMarsch len = " << length << endl;

    JSObject *dict = JS_NewObject(ctx,
            &DBusResult_jsClass,
            //JS_GetClass(JS_GetGlobalObject(ctx)),
            NULL, NULL);

    dbus_message_iter_recurse(iter, &subiter);
    while ((current_type = dbus_message_iter_get_arg_type(iter))
            != DBUS_TYPE_INVALID && ret == JS_TRUE) {
        jsval val;
        // first come the key - must be a basic type. ATM we recognize strings only.
        cerr << "--- name " << endl;
        char *name;
        if (current_type != DBUS_TYPE_STRING) {
            cerr << "Whaaaa! Don't know about non-string keys! skip!" << endl;
            dbus_message_iter_next(&subiter);
            dbus_message_iter_next(&subiter);
            current_type = dbus_message_iter_get_arg_type(iter);
            continue;
        }
        dbus_message_iter_get_basic(&subiter, &name);

        cerr << "--- value " << endl;
        dbus_message_iter_next(&subiter);
        current_type = dbus_message_iter_get_arg_type(iter);

        ret = unMarshallIter(ctx, current_type, &subiter, &val);

        JS_SetProperty(ctx, dict, name, &val);

        dbus_message_iter_next(&subiter);
        current_type = dbus_message_iter_get_arg_type(iter);
    }
    
    *val = OBJECT_TO_JSVAL(dict);


       return JS_TRUE;
}