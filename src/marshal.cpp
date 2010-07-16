#include "marshal.h"
#include <string.h>
#include <iostream>
/*
#include <jsobj.h>
#include <jsprvtd.h>
#include <jsxml.h> // js xml support -> convert to string on return.
*/
using namespace std;

int indent = 0;

JSBool
DBusMarshalling::appendArgs(JSContext *cx, JSObject *obj,
        DBusMessage *message,
        DBusMessageIter *iter,
        uintN argc, jsval *argv) {
    JSBool ret = JS_TRUE;
    for (uintN i = 0; i < argc && ret == JS_TRUE; i++) {
        jsval* val = &argv[i];

        ret = marshallVariant(cx, obj, iter, val);
    }
    return JS_TRUE;
}

JSBool DBusMarshalling::marshallBasicValue(JSContext *cx, JSObject *obj, DBusMessageIter *iter, jsval * val) {
    dbus_int32_t ret;
    if (val == NULL) {
        fprintf(stderr, "NULL value detected - skipping!\n");
        return JS_TRUE;
    }

    dbg3_err("bas");
    int tag = JSVAL_TAG(*val);
    if (JSVAL_IS_INT(*val)) {
        jsint iv = JSVAL_TO_INT(*val);
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_INT32,
                &iv);
    } else if (JSVAL_IS_STRING(*val)) {
        JSString* sv = JSVAL_TO_STRING(*val);
        //jschar* jv = JS_GetStringChars(sv);
        //fprintf(stderr, ".. a string  %02x %02x %02x %02x!\n",jv[0],jv[1],jv[2],jv[3]);
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
                    dbg2_err("its an object!");

                //JSObject* ov = JSVAL_TO_OBJECT(*val);

                dbg2_err("...uuuund? was machma damit??? a object im basic. sollma den source marshallen? ");
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

JSBool DBusMarshalling::marshallVariant(JSContext *cx, JSObject *obj, /*DBusMessage *message,*/ DBusMessageIter *iter, jsval * val) {
    dbus_int32_t ret;

    if (val == NULL) {
        fprintf(stderr, "NULL value detected - skipping!\n");
        return JS_TRUE;
    }

    dbg3_err("var");
    int tag = JSVAL_TAG(*val);
    if (JSVAL_IS_INT(*val)) {
        jsint iv = JSVAL_TO_INT(*val);
        ret = dbus_message_iter_append_basic(iter,
                DBUS_TYPE_INT32,
                &iv);
    } else if (JSVAL_IS_STRING(*val)) {
        JSString* sv = JSVAL_TO_STRING(*val);
        jschar* jv = JS_GetStringChars(sv);
        fprintf(stderr, ".. a string  %02x %02x %02x %02x!\n", jv[0], jv[1], jv[2], jv[3]);
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
                    dbg2_err("its an object!");

                JSObject* ov = JSVAL_TO_OBJECT(*val);

                dbg2_err("...uuuund? " << hex << ov << dec);
                if (JS_IsArrayObject(cx, ov)) {
                    dbg2_err("...aha!");
                    ret = marshallJSArray(cx, ov, iter);
                } else if (isDictObject(cx, ov)) {
                    dbg2_err("...dudu!");
                    ret = marshallDictObject(cx, ov, iter);
                } else if (isTypeCastObject(cx, ov)) {
                    dbg2_err("type cast found!");
                    JSObject* ov = JSVAL_TO_OBJECT(*val);
                    jsval typeval;
                    jsval valval;
                    ret = JS_GetProperty(cx, ov, "type", &typeval);
                    ret = JS_GetProperty(cx, ov, "value", &valval);
                    if (! JSVAL_IS_STRING(typeval)) {
                        dbg_err("'type' must be a string!");
                        return JS_FALSE;
                    }
                    char*typstr = JS_GetStringBytes(JSVAL_TO_STRING(typeval));
                    if (strcmp("UInt32", typstr) == 0) {
                        jsint iv = JSVAL_TO_INT(valval);
                        unsigned int value = iv;
                        dbg3_err("setting uint32:" << value);
                        ret = dbus_message_iter_append_basic(iter,
                                DBUS_TYPE_UINT32,
                                &value);
                        return JS_TRUE;
                    } else if (strcmp("UInt64", typstr) == 0) {
                    } else {
                        dbg_err("unknown type cast " << typstr);
                        return JS_FALSE;
                    }
                    /*
                } else if (OBJECT_IS_XML(cx,ov)) {
                    JSClass* cls = JS_GET_CLASS(cx, ov);
                    dbg2_err("..xml! " << cls->name);
                    JSString *str = js_ValueToXMLString(cx, *val);
                    cerr << "xmlstr " << JS_GetStringBytes(str) << endl << endl;

                     * */
                } else {
                    JSClass* cls = JS_GET_CLASS(cx, ov);
                    dbg2_err("..oho! " << cls->name);
                    ret = marshallJSObject(cx, ov, iter);
                }
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
DBusMarshalling::marshallJSArray(JSContext* cx,
        JSObject* arrayObj,
        DBusMessageIter* iter) {
    DBusMessageIter container_iter;
    int rv;
    jsuint i;
    bool isDict = false;

    JSIdArray* values = JS_Enumerate(cx, arrayObj);
    dbg3_err("marshalling array l=" << values->length);

    JSBool hasVariantValues = JSObjectHasVariantValues(cx, arrayObj);

    if (values->length == 0) {
        enforce_notnull(
                dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                DBUS_TYPE_VARIANT_AS_STRING
                , &container_iter));
        dbus_message_iter_close_container(iter, &container_iter);
        JS_free(cx, values);
        return JS_TRUE;
    }

    /****** optional: create a{sv} signature if array contains dict entries.*/
    jsval propkey;
    JS_GetPropertyById(cx, arrayObj, values->vector[0], &propkey);
    JSObject* obj = JSVAL_TO_OBJECT(propkey);
    JSClass* cls = JS_GetClass(cx, obj);

    if (jsvalIsDictObject(cx, propkey)) {
        dbg2_err("ITS A DICT OBJECT!");
        isDict = true;
        enforce_notnull(
                dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                DBUS_TYPE_ARRAY_AS_STRING
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                DBUS_TYPE_STRING_AS_STRING
                DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING
                , &container_iter));
    } else {
        // generic variant array
        dbg2_err("ITS  A GENERIC ARRAY!");
        enforce_notnull(
                dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                DBUS_TYPE_VARIANT_AS_STRING
                , &container_iter));
    }
    for (i = 0; i < values->length; i++) {
        jsval propkey = 0;
        jsval propval = 0;

        if (!JS_IdToValue(cx, values->vector[i], &propkey))
            return JS_FALSE;
        if (JSVAL_IS_INT(propkey)) {
            //dbg3_cerr(" int propkey is  "  << JSVAL_TO_INT(propkey) );
        }

        JS_GetPropertyById(cx, arrayObj, values->vector[i], &propval);

        rv = marshallVariant(cx, arrayObj, &container_iter, &propval);
    }
    dbus_message_iter_close_container(iter, &container_iter);
    printf("marshalled array object\n");
    JS_free(cx, values);
    return JS_TRUE;
}



JSBool DBusMarshalling::getDictValTypeSig(JSContext* cx, JSObject* dictObj, char* sig) {
    dbus_int32_t ret;
    jsval val;
    jsval cast;
    JS_GetProperty(cx, dictObj,"value",&val);

    JS_GetProperty(cx, dictObj,"type",&cast);
    if (JSVAL_IS_STRING(cast) ) {
        return JS_TRUE;
    }

    int tag = JSVAL_TAG(val);
    if (JSVAL_IS_INT(val)) {
        jsint iv = JSVAL_TO_INT(val);
        sig[0] = DBUS_TYPE_INT32 ;
    } else if (JSVAL_IS_STRING(val)) {
        JSString* sv = JSVAL_TO_STRING(val);
        sig[0] =  DBUS_TYPE_STRING;
    } else if (JSVAL_IS_BOOLEAN(val)) {
        sig[0] = DBUS_TYPE_BOOLEAN;
    } else if (JSVAL_IS_DOUBLE(val)) {
        sig[0] = DBUS_TYPE_DOUBLE;
    } else
        switch (tag) {
            case JSVAL_OBJECT:
            {
                // parse object....
                if (JSVAL_IS_OBJECT(val))
                    dbg2_err("its an object!");

                //JSObject* ov = JSVAL_TO_OBJECT(*val);

                dbg_err("...uuuund? was machma damit??? a object im basic. sollma den source marshallen? ");
            }
                break;
            default:
                fprintf(stderr, "illegal type (tag=%d) !\n", tag);
                if (JSVAL_IS_NUMBER(val))
                    fprintf(stderr, "its a number !\n");
                if (JSVAL_IS_DOUBLE(val))
                    fprintf(stderr, "its a double !\n");
                if (JSVAL_IS_INT(val))
                    fprintf(stderr, "its a int !\n");
                if (JSVAL_IS_OBJECT(val))
                    fprintf(stderr, "its a object !\n");
                if (JSVAL_IS_STRING(val))
                    fprintf(stderr, "its a string !\n");
                if (JSVAL_IS_BOOLEAN(val))
                    fprintf(stderr, "its a bool !\n");
                if (JSVAL_IS_NULL(val))
                    fprintf(stderr, "its a null !\n");
                if (JSVAL_IS_VOID(val))
                    fprintf(stderr, "its a void !\n");
                //ret = JS_FALSE;
        }
    return JS_TRUE;
}

JSBool
DBusMarshalling::marshallDictObject(JSContext* ctx,
        JSObject* dictObj,
        DBusMessageIter* iter) {
    DBusMessageIter dict_iter, variant;
    int rv;
    JSBool ret;
    dbg2_err("marsh DICT OBJECT");

    JSIdArray* values = JS_Enumerate(ctx, dictObj);
    dbg3_err("marshalling dict l=" << values->length);
    if (values->length<2) {
        dbg_err("not enough elements found! l=" << values->length);
    }

    cerr << "iter sig " << dbus_message_iter_get_signature(iter) << endl;
    enforce_notnull(
            dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY,
            //DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
            DBUS_TYPE_STRING_AS_STRING
            DBUS_TYPE_VARIANT_AS_STRING
            //DBUS_DICT_ENTRY_END_CHAR_AS_STRING
            , &dict_iter));
    cerr << "  dict_iter sig " << dbus_message_iter_get_signature(&dict_iter) << endl;

    jsval propkey = 0;
    jsval propval = 0;

    ret = JS_GetProperty(ctx, dictObj, "key", &propkey);
    ret = JS_GetProperty(ctx, dictObj, "value", &propkey);

    char*keystr = JS_GetStringBytes(JSVAL_TO_STRING(propkey));
    dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &keystr);

    char sig[8] = {0,0,0,0,0,0,0,0};
    getDictValTypeSig(ctx, dictObj, sig);
    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, sig, &variant);
    marshallBasicValue(ctx, dictObj, &variant, &propval);

    cerr << "  container_iter sig " << dbus_message_iter_get_signature(&dict_iter) << endl;
    printf("marshalled dict object\n");
    return JS_TRUE;
}




#if 0
JSBool
DBusMarshalling::marshallDictObject(JSContext* cx,
        JSObject* dictObj,
        DBusMessageIter* iter) {
    DBusMessageIter container_iter;
    int rv;
    dbg2_err("marsh DICT");

    cerr << "iter sig " << dbus_message_iter_get_signature(iter) << endl;
    enforce_notnull(
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
            DBUS_TYPE_ARRAY_AS_STRING
            DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
            DBUS_TYPE_STRING_AS_STRING
            //		DBUS_TYPE_STRING_AS_STRING
            DBUS_TYPE_VARIANT_AS_STRING
            DBUS_DICT_ENTRY_END_CHAR_AS_STRING
            , &container_iter));
    cerr << "  container_iter sig " << dbus_message_iter_get_signature(&container_iter) << endl;

    JSIdArray* values = JS_Enumerate(cx, dictObj);
    dbg3_err("marshalling dict l=" << values->length);

    for (int i = 0; i < values->length; i++) {
        jsval propkey = 0;
        jsval propval = 0;
        //#define INCONT
#ifdef INCONT
        DBusMessageIter &dict_iter = container_iter;
#else
        DBusMessageIter dict_iter;
        cerr << "---- -1 " << endl;
        if (false) dbus_message_iter_open_container(&container_iter, DBUS_DICT_ENTRY_BEGIN_CHAR,
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                DBUS_TYPE_STRING_AS_STRING
                //			DBUS_TYPE_STRING_AS_STRING
                DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                &dict_iter);

        dbus_message_iter_open_container(iter, DBUS_DICT_ENTRY_BEGIN_CHAR,
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                DBUS_TYPE_STRING_AS_STRING
                //			DBUS_TYPE_STRING_AS_STRING
                DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                &dict_iter);
#endif
        cerr << "      dict_iter sig " << dbus_message_iter_get_signature(&dict_iter) << endl;
        if (!JS_IdToValue(cx, values->vector[i], &propkey))
            return JS_FALSE;
        if (JSVAL_IS_INT(propkey)) {
            //dbg3_cerr(" int propkey is  "  << JSVAL_TO_INT(propkey) );
        }
        if (JSVAL_IS_STRING(propkey)) {
            char* name = NULL;
            JSString* snam = JS_ValueToString(cx, propkey);
            name = JS_GetStringBytes(snam);
            cerr << "[[[" << name << "]]]" << endl;

#if !defined(INCONT)
            cerr << "-----0 " << endl;

            dbus_message_iter_append_basic(&dict_iter,
                    DBUS_TYPE_STRING, &name);
#endif
        }
        cerr << "-----1 " << endl;
#ifdef INCONT
        rv = marshallBasicValue(cx, dictObj, &dict_iter, &propkey);
#endif       
        JS_GetPropertyById(cx, dictObj, values->vector[i], &propval);

        cerr << "-----2 " << endl;
        rv = marshallBasicValue(cx, dictObj, &dict_iter, &propval);

#ifdef INCONT
        dbus_message_iter_close_container(iter, &dict_iter);
#endif
    }
    JS_free(cx, values);

    cerr << "  container_iter sig " << dbus_message_iter_get_signature(&container_iter) << endl;
    //dbus_message_iter_close_container(iter, &container_iter);
    printf("marshalled dict object\n");
    return JS_TRUE;
}



#endif


JSBool
DBusMarshalling::marshallJSObject(JSContext* cx,
        JSObject* aObj,
        DBusMessageIter* iter) {
    DBusMessageIter container_iter;
    int rv;
    jsval oval = OBJECT_TO_JSVAL(aObj);

    JSVAL_LOCK(cx,oval);

    JSBool hasVariantValues = JSObjectHasVariantValues(cx, aObj);

    enforce_notnull(
            dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
            DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
            DBUS_TYPE_STRING_AS_STRING
            DBUS_TYPE_VARIANT_AS_STRING
            DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
            &container_iter));

    
    cerr << "propiter" << endl;

    JSObject* js_iter = JS_NewPropertyIterator(cx, aObj);
    jsid propid;
    if (!JS_NextProperty(cx, js_iter, &propid)) {
        goto fail;//return JS_FALSE;
    }
    cerr << "propiter2" << endl;

    while (propid != JSVAL_VOID) {
        jsval propval;
        JSString *snam = NULL;

        if (!JS_IdToValue(cx, propid, &propval))
            goto fail;//return JS_FALSE;
        cerr << "marshJSO" << endl;
        if (JSVAL_IS_STRING(propval)) {
            char* name = NULL;
            snam = JS_ValueToString(cx, propval);
            name = JS_GetStringBytes(snam);
            cerr << "[[[" << name << "]]]" << endl;
        } else if (JSVAL_IS_STRING(propval)) {

        }
        // key, value.
        rv = marshallJSProperty(cx, aObj, propval, &container_iter,
                hasVariantValues);

        if (!JS_NextProperty(cx, js_iter, &propid)) {
            goto fail;//return JS_FALSE;
        }
    }

    dbus_message_iter_close_container(iter, &container_iter);
    printf("marshalled object\n");

    JSVAL_UNLOCK(cx,oval);
    return JS_TRUE;
    fail:
    JSVAL_UNLOCK(cx,oval);
    return JS_FALSE;
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

    dbg3_err(" marshal typed o: tval " << tval);
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

    enforce_notnull(
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
                enforce_notnull(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_INT32_AS_STRING,
                        &variant_iter));
                enforce_notnull(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_INT32,
                        &intVal));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                enforce_notnull(
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
             */
            const char* propvalstringbuf = JS_GetStringBytes(propvaljsstring);
            printf("  VALUE: %s\n", propvalstringbuf);
            if (dbus_type == DBUS_TYPE_INVALID || isVariant) {
                enforce_notnull(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_STRING_AS_STRING,
                        &variant_iter));
                enforce_notnull(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_STRING,
                        &propvalstringbuf));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                if (dbus_type != DBUS_TYPE_INVALID) {
                    enforce_notnull(
                            dbus_message_iter_append_basic(&dictentry_iter,
                            dbus_type,
                            &propvalstringbuf));
                }
                enforce_notnull(
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
                enforce_notnull(
                        dbus_message_iter_open_container(
                        &dictentry_iter,
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_BOOLEAN_AS_STRING,
                        &variant_iter));
                enforce_notnull(
                        dbus_message_iter_append_basic(&variant_iter,
                        DBUS_TYPE_BOOLEAN,
                        &propbool));
                dbus_message_iter_close_container(&dictentry_iter,
                        &variant_iter);
            } else {
                enforce_notnull(
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
        //dbg3(cerr << "gVA " << len << " " << current_type << "  sig = ");
        //dbg3_err( dbus_message_iter_get_signature(iter) ); /// XXX need to free signature..
        ret = unMarshallIter(ctx, current_type, iter, &vector[len]);
        dbus_message_iter_next(iter);
        len++;
    }
    dbg2_err("gVA << end len " << len);
    *length = len;
    return ret;
}

JSBool DBusMarshalling::unMarshallIter(JSContext *ctx, int current_type, DBusMessageIter *iter, jsval* val) {
    JSBool ret = JS_TRUE;
    if (dbus_type_is_basic(current_type)) {
        dbg3(cerr << "basic type.. ");
        ret = unMarshallBasic(ctx, current_type, iter, val);
    } else if (dbus_type_is_container(current_type)) {
        dbg3_err("container type.. " << dbus_message_iter_get_signature(iter) );
        indent++;
        switch (current_type) {
            case DBUS_TYPE_ARRAY:
            {
                dbg3_err("array");
                indent++;
                ret = unMarshallArray(ctx, current_type, iter, val);
                indent--;
                dbg3_err("array done " << hex << *val << dec);
                break;
            }
            case DBUS_TYPE_DICT_ENTRY:
            {
                dbg3_err("dict.. " << dbus_message_iter_get_signature(iter) ); //@TODO implement dict type
                indent++;
                ret = unMarshallDict(ctx, current_type, iter, val);
                indent--;
                dbg3_err("dict done " << hex << *val << dec);
                break;
            }
            case DBUS_TYPE_VARIANT:
            {
                DBusMessageIter subiter;
                dbg_err("variant ");

                dbus_message_iter_recurse (iter, &subiter);
                char* sig = dbus_message_iter_get_signature (&subiter);
                int var_type = dbus_message_iter_get_arg_type(&subiter);
                indent++;
                ret = unMarshallIter(ctx,var_type,&subiter, val);
                indent--;
                break;
            }
            default:
            {
                dbg_err("unknown container type " << (char) current_type);
                ret = JS_FALSE;
            }
            indent--;
        }
    } else {
        dbg_err(  "Can't deal with signature " << dbus_message_iter_get_signature(iter) );
        ret = JS_FALSE;
    }
    return ret;
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
            dbg2_err("int16 [" << (*value) << "]");
            //variant->SetAsInt16(val);
            break;
        }
        case DBUS_TYPE_UINT16:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            dbg2_err("int16 [" << *value << "]");
            //variant->SetAsUint16(val);
            break;
        }
        case DBUS_TYPE_INT32:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            dbg2_err("int32 [" << val << "]");
            *value = INT_TO_JSVAL(val);
            //variant->SetAsInt32(val);
            break;
        }
        case DBUS_TYPE_UINT32:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            dbg2_err("uint32 [" << *value << "]");
            //variant->SetAsUint32(val);
            break;
        }
        case DBUS_TYPE_INT64:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsInt64(val);
            cerr << "INT64 POTENTIAL OVERFLOW \n";
            break;
        }
        case DBUS_TYPE_UINT64:
        {
            jsint val;
            dbus_message_iter_get_basic(iter, &val);
            *value = INT_TO_JSVAL(val);
            //variant->SetAsUint64(val);
            cerr << "UINT64 POTENTIAL OVERFLOW \n";
            break;
        }
        case DBUS_TYPE_DOUBLE:
        {
            double val;
            dbus_message_iter_get_basic(iter, &val);
            //variant->SetAsDouble(val);
            *value = DOUBLE_TO_JSVAL(&val);
            break;
        }
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_OBJECT_PATH:
        {
            char *val;
            dbus_message_iter_get_basic(iter, &val);
            dbg2_err("string [" << val << "]");
            JSString *str = JS_NewStringCopyZ(ctx, val);
            *value = STRING_TO_JSVAL(str);
            break;
        }
        default:
        {
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSBool
DBusMarshalling::unMarshallArray(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val) {
    JSBool ret;
    DBusMessageIter subiter;
    int length = 0;
    jsval* vector;
    int current_type;

    dbus_message_iter_recurse(iter, &subiter);
    length = dbus_iter_length(&subiter);
    dbus_message_iter_recurse(iter, &subiter);
    
    char* sig = dbus_message_iter_get_signature(&subiter);
    dbg3_err("unMarsh len1 = " << length <<
            " sig = " << sig);
    if (strcmp("{ss}", sig) == 0 || strcmp("{sv}", sig) == 0)
        dbg3_err("TODO: handle Dict intelligently... ");
    dbus_free(sig);

    if (length == 0) {
        //empty array
    }
    vector = new jsval[length]; //TODO: memleak?

    ret = getVariantArray(ctx, &subiter, &length, vector);
    dbg3_err("unMarsch len2 = " << length << endl);
    if (ret == JS_FALSE)
        return ret;
    //create array in any case...
    //    if (length > 0) {
    JSObject * args = JS_NewArrayObject(ctx, length, vector);
    *val = OBJECT_TO_JSVAL(args);
    //    }
    return JS_TRUE;
}

JSBool
DBusMarshalling::unMarshallDict(JSContext *ctx, int type, DBusMessageIter *iter, jsval* val) {
    JSBool ret = JS_TRUE;
    DBusMessageIter subiter;
    int length = 0;
    int current_type;
    dbg3_err("dict entry");

    JSObject *dict = JS_NewObject(ctx,
            &DBusDict_jsClass,
            NULL, NULL);

    dbus_message_iter_recurse(iter, &subiter);
    dbg3_err("recurse again"  );
    while ((current_type = dbus_message_iter_get_arg_type(&subiter))
            != DBUS_TYPE_INVALID && ret == JS_TRUE) {
        dbg3_err("get entry" );
        jsval val;
        // first come the key - must be a basic type. ATM we recognize strings only.
        char *name;
        if (current_type != DBUS_TYPE_STRING) {
            cerr << "Whaaaa! Don't know about non-string keys! skip!" << endl;
            dbus_message_iter_next(&subiter);
            dbus_message_iter_next(&subiter);
            current_type = dbus_message_iter_get_arg_type(iter);
            continue;
        }
        dbus_message_iter_get_basic(&subiter, &name);
        dbg3_err( "--- name " << name);

        dbus_message_iter_next(&subiter);
        current_type = dbus_message_iter_get_arg_type(&subiter);

        ret = unMarshallIter(ctx, current_type, &subiter, &val);
        dbg3_err("--- value " << hex << val << dec );
        dbg3_err("--- value is string? " << JSVAL_IS_STRING(val) );

        JS_SetProperty(ctx, dict, name, &val);
        {
            jsval key = STRING_TO_JSVAL( JS_NewStringCopyZ(ctx, name) );
            JS_SetProperty(ctx, dict, "key", &key);
        }
        JS_SetProperty(ctx, dict, "value", &val);

        dbus_message_iter_next(&subiter);
        current_type = dbus_message_iter_get_arg_type(&subiter);
    }
    dbg3_err("recurse done"  );

    *val = OBJECT_TO_JSVAL(dict);

    return JS_TRUE;
}
