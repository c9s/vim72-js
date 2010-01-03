/*
 * vim:ts=8:sw=4:noet:
 *
 * JavaScript interface by Cornelius Lin <cornelius.howl@gmail.com>
 *
 * See https://developer.mozilla.org/en/JavaScript_C_Engine_Embedder's_Guide
 * for more details.
 *
 * naming rule:
 *  struct name          : js_[A-Z][a-z]+
 *  js export functions  : js_[a-z]+
 *  non-export functions : [a-z]+
 */

#include "vim.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "jsapi.h"

#define js_runtime_memory 8L

/* JS global variables. */
typedef struct jsEnv
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *global;
} jsEnv;

typedef struct
{
    JSObject	    *jso;
    buf_T	    *buf;
} jsBufferObject;

jsEnv *js_env = NULL;

/* The class of the global object. */
static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};



/* The javascript error reporter callback. */
    void
report_error(cx, message, report)
    JSContext		*cx;
    const char		*message;
    JSErrorReport	*report;
{
    char *error_msg;
    char *p;

    error_msg = (char *) alloc(128 * sizeof(char));
    vim_memset(error_msg, 0, 128 * sizeof(char));
    sprintf(error_msg, "Javascript Error: %s:%u:%s\n",
	    report->filename ? report->filename : "<no filename>",
	    (unsigned int) report->lineno,
	    message);
    p = strchr(error_msg, '\n');
    if (p)
	*p = '\0';
    MSG(error_msg);

    vim_free( error_msg );
    return;
}

    JSBool
js_system(cx, obj, argc, argv, rval)
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    char *cmd;
    int rc;

    if (!JS_ConvertArguments(cx, argc, argv, "s", &cmd))
	return JS_FALSE;

    rc = system(cmd);
    if (rc != 0)
    {
	/* Throw a JavaScript exception. */
	JS_ReportError(cx, "Command failed with exit code %d", rc);
	return JS_FALSE;
    }
    *rval = JSVAL_VOID;		/* return undefined */

    return JS_TRUE;
}


    JSBool
js_buf_number( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    jsBufferObject * bufobj;
    jsint fnum;

    bufobj = (jsBufferObject *) JSVAL_TO_OBJECT( argv[0] );

    if( ! bufobj || ! bufobj->buf || ! bufobj->buf->b_fnum ) 
	return JS_FALSE;

    fnum = bufobj->buf->b_fnum;

    *rval = INT_TO_JSVAL( fnum );
    return JS_TRUE;
}

    JSBool
js_buf_ffname( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    char *buf_name;
    JSString * str;
    jsBufferObject * bufobj;

    bufobj = (jsBufferObject *) JSVAL_TO_OBJECT( argv[0] );

    if( ! bufobj || ! bufobj->buf || ! bufobj->buf->b_ffname ) 
	return JS_FALSE;

    buf_name = (char *) bufobj->buf->b_ffname;

    //XXX: unicode string
    //JSString *str = JS_NewUCString( js_env->cx , buf_name , strlen( buf_name ) );
    str = JS_NewString( cx , buf_name , strlen( buf_name ) );

    *rval = STRING_TO_JSVAL( str );
    return JS_TRUE;
}

    JSBool
js_buf_cnt( cx , obj , argc ,argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    buf_T  *b;
    int n = 0;

    for ( b = firstbuf ; b ; b = b->b_next)
	++n;

    return JS_NewNumberValue(cx, n , rval);
}

    static jsBufferObject *
js_new_buffer_object( cx , buf )
    JSContext *cx;
    buf_T     *buf;

{
    jsBufferObject *self = (jsBufferObject *) alloc( sizeof( jsBufferObject ) );
    vim_memset(self, 0, sizeof( jsBufferObject ));
    self->jso = JS_NewObject( cx, NULL, NULL, NULL);
    self->buf = buf;
    return self;
}

    void
js_free_buffer_object( cx , buf ) 
    JSContext *cx;
    buf_T     *buf;
{



}


    JSBool
js_get_buffer_by_num( cx , obj , argc ,argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    int	    fnum;
    buf_T   *buf;
    jsBufferObject *bufobj;

    if (!JS_ConvertArguments(cx, argc, argv, "/j", &fnum)) {
	JS_ReportError(cx, "Can't convert buffer number");
	return JS_FALSE;
    }


    for (buf = firstbuf; buf; buf = buf->b_next) {
	if (buf->b_fnum == fnum) {
	    bufobj = js_new_buffer_object(cx,buf);
	    if ( bufobj->jso == NULL) {
		return JS_FALSE;
	    }
	    else {
		*rval = OBJECT_TO_JSVAL( (JSObject *) bufobj );
		return JS_TRUE;
	    }
	}
    }

    *rval = JSVAL_VOID;
    JS_ReportError(cx, "Can not found buffer %d", fnum);
    return JS_FALSE;
}



    JSBool
js_vim_message( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{

    char *message;
    char *p;

    if (!JS_ConvertArguments(cx, argc, argv, "s", &message))
	return JS_FALSE;

    p = strchr(message, '\n');
    if (p)
	*p = '\0';
    MSG(message);

    *rval = JSVAL_VOID;

    return JS_TRUE;
}


/*
 * Export C functions to javascript environment. 
 * 
 **/
static JSFunctionSpec js_global_functions[] = {
    JS_FS("system", js_system, 1, 0, 0),

    /* vim function interface here  */
    JS_FS("alert"      , js_vim_message       , 1 , 0 , 0) , 
    JS_FS("message"    , js_vim_message       , 1 , 0 , 0) , 
    JS_FS("buf_cnt"    , js_buf_cnt           , 0 , 0 , 0) , 
    JS_FS("buf_nr"     , js_get_buffer_by_num , 0 , 0 , 0) , 
    JS_FS("buf_ffname" , js_buf_ffname        , 0 , 0 , 0) , 
    JS_FS("buf_number" , js_buf_number	      , 0 , 0 , 0) ,
    JS_FS_END
};


/*
 * :jsfile
 *
 * @see:
 * https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference/JS_CompileFil
 * e
 */
    void
ex_jsfile(eap)
    exarg_T *eap;
{
    char *filename = (char *) eap->arg;
    int compileOnly = 0;
    JSScript *script = JS_CompileFile( js_env->cx, js_env->global, filename);

    if (script) {
	if (!compileOnly)
	    (void) JS_ExecuteScript( js_env->cx, js_env->global, script, NULL);
	JS_DestroyScript( js_env->cx, script);
    }
}

    void
vim_js_init(arg)
    char *arg;
{
    js_env = (jsEnv *) alloc(sizeof(jsEnv));
    vim_memset( js_env , 0 , sizeof(jsEnv) );
    
    //js_env = (jsEnv *) JS_malloc( sizeof(jsEnv) );
    //JS_malloc

    js_env->rt = JS_NewRuntime( js_runtime_memory * 1024L * 1024L);


    /* 3 things to init runtime , context and global object */
    if ( js_env->rt == NULL)
	return;

    js_env->cx = JS_NewContext( js_env->rt, 8192);
    if (js_env->cx == NULL)
	return;
    JS_SetOptions(js_env->cx, JSOPTION_VAROBJFIX);
    JS_SetVersion(js_env->cx, JSVERSION_LATEST);
    JS_SetErrorReporter( js_env->cx, report_error);

    js_env->global = JS_NewObject( js_env->cx, &global_class, NULL, NULL);

    if ( js_env->global == NULL)
	return;

    /*
     * Populate the global object with the standard globals, like Object
     * and Array.
     */
    if (!JS_InitStandardClasses( js_env->cx, js_env->global))
	return;

    if (!JS_DefineFunctions( js_env->cx, js_env->global, js_global_functions))
	return;
}

    void
js_end()
{

    /* Cleanup. */
    JS_DestroyContext( js_env->cx);
    JS_DestroyRuntime( js_env->rt);
    JS_ShutDown();

}
