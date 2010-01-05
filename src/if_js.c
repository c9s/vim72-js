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
 
    synopsis: 

	var buffer = vim.buffer.find( 2 );

	buffer.set('nu').set('hls');   // setlocal nu hls

	vim.set('nu');    // set nu

	var lines = vim.buffer.find( '/path/to/file' ).lines( 1, 10);

	var win = vim.window.find( .. );
	var tab = vim.tab.find( ... );

	vim.tab.new( ...  ).edit_buffer( buf );

	vim.command( 'vsplit' );

	...
 */

#include "vim.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "jsapi.h"

#define JS_RUNTIME_MEMORY 8L

#define INSTANCE_BUFFER(cx,obj)   JS_GetInstancePrivate(cx, obj, &buffer_class, NULL);

#define CHECK_BUFFER_FIELD(buf,field)  if( !buf->field ) { \
	*rval = JSVAL_VOID ;\
	JS_ReportError(cx, "buffer field '" #field "' is empty. %s:%d", __FILE__ , __LINE__ ); \
	return JS_FALSE ; \
    } 

static JSClass vim_class;
static JSClass window_class;
static JSClass tab_class;
static JSClass buffer_class;
static JSFunctionSpec js_global_functions[]; 

/* JS API Functions */
void report_error( JSContext *cx , const char *message , JSErrorReport *report );

/* Helper Functions */
JSObject * js_buffer_new( JSContext * cx, buf_T * buf);

/* Interface Functions */
JSBool js_system(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);

/* Vim Interface Functions */
JSBool js_vim_message(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);

/* Buffer interface functions */
JSBool js_buffer_number(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_line(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_lines(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_ffname(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_sfname(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_fname(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_window_number(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buf_cnt(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_get_buffer_by_num(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_next(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);
JSBool js_buffer_prev(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval);

/* Window interface functions */

/* Tab interface functions */

/* JS global variables. */
typedef struct jsEnv
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *global;
} jsEnv;

jsEnv *js_env = NULL;

/* The class of the global object. */

/* Buffer Class */
static JSClass buffer_class = {
    "Buffer",
    JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
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

static JSFunctionSpec buffer_methods[] = {
    JS_FS("number", js_buffer_number, 1, 0, 0),
    JS_FS("window_number", js_buffer_window_number, 1, 0, 0),
    JS_FS("ffname", js_buffer_ffname, 1, 0, 0),
    JS_FS("sfname", js_buffer_sfname, 1, 0, 0),
    JS_FS("fname", js_buffer_fname, 1, 0, 0),
    JS_FS("line", js_buffer_line, 1, 0, 0),
    JS_FS("lines", js_buffer_lines, 1, 0, 0),
    JS_FS("next", js_buffer_next, 1, 0, 0),
    JS_FS("prev", js_buffer_prev, 1, 0, 0),
    JS_FS_END
};


/* global vim class */
// XXX: define this object when init.
static JSClass vim_class = {
    "vim",
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

/* global vim class functions */
static JSFunctionSpec vim_class_functions[] = {
    // JS_FS("system", js_system, 1, 0, 0),
    // JS_FS("alert"      , js_vim_message       , 1 , 0 , 0) , 
    // JS_FS("message"    , js_vim_message       , 1 , 0 , 0) , 
    // JS_FS("buf_cnt"    , js_buf_cnt           , 0 , 0 , 0) , 
    // JS_FS("buf_nr"     , js_get_buffer_by_num , 0 , 0 , 0) , 
    JS_FS_END
};

/* global class */
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

/* global functions */
static JSFunctionSpec js_global_functions[] = {
    JS_FS("system", js_system, 1, 0, 0),

    /* vim function interface here  */
    JS_FS("alert"      , js_vim_message       , 1 , 0 , 0) , 
    JS_FS("message"    , js_vim_message       , 1 , 0 , 0) , 

// XXX: mv buf_cnt , buf_nr to vim.buffer.count()  and vim.buffer.find( [num] );
    JS_FS("buf_cnt"    , js_buf_cnt           , 0 , 0 , 0) , 
    JS_FS("buf_nr"     , js_get_buffer_by_num , 0 , 0 , 0) , 
    JS_FS_END
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

    return;
}


/* --------------------- Helper Functions ----------------------- */

JSObject *
js_buffer_new( cx , buf ) 
    JSContext * cx;
    buf_T     * buf;
{
    if (buf->b_js_ref)
	return buf->b_js_ref;

    JSObject * jsobj;
    jsobj = JS_NewObject( cx , &buffer_class , NULL , NULL );
    if (!JS_SetPrivate(cx, jsobj, buf ) ) {
	return NULL;
    }
    JS_DefineFunctions(cx , jsobj , buffer_methods);

    buf->b_js_ref = jsobj;
    return jsobj;
}

/* --------------------- Javascript Interface ----------------------- */

    JSBool
js_system(cx, obj, argc, argv, rval)
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    const char *cmd;
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
js_buffer_number( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    int fnum;
    //buf_T *buf = JS_GetInstancePrivate(cx, obj, &buffer_class, NULL);
    buf_T *buf = INSTANCE_BUFFER(cx,obj);
    fnum = buf->b_fnum;
    *rval = INT_TO_JSVAL( fnum );
    return JS_TRUE;
}

    JSBool
js_buffer_window_number( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    int wnum;
    buf_T *buf = INSTANCE_BUFFER(cx,obj);
    wnum = buf->b_nwindows;
    *rval = INT_TO_JSVAL( wnum );
    return JS_TRUE;
}


    JSBool
js_buffer_ffname( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    char* ffname;
    JSString *str;
    buf_T *buf;

    buf = INSTANCE_BUFFER(cx,obj);
    CHECK_BUFFER_FIELD(buf,b_ffname);

    ffname = strdup( (char *) buf->b_ffname );
    str = JS_NewString( cx , ffname , strlen( ffname ) );
    *rval = STRING_TO_JSVAL( str );
    return JS_TRUE;
}



    JSBool
js_buffer_fname( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    char *fname;
    JSString *str;
    buf_T *buf;

    buf = INSTANCE_BUFFER(cx,obj);

    CHECK_BUFFER_FIELD(buf,b_fname);

    fname = strdup( (char *) buf->b_fname );
    str = JS_NewString( cx , fname , strlen( fname ) );
    *rval = STRING_TO_JSVAL( str );
    return JS_TRUE;
}


    JSBool
js_buffer_sfname( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    char* sfname;
    JSString *str;
    buf_T *buf;
    buf = INSTANCE_BUFFER(cx,obj);

    CHECK_BUFFER_FIELD(buf,b_sfname);

    sfname = strdup( (char *) buf->b_sfname );
    str = JS_NewString( cx , sfname , strlen( sfname ) );
    *rval = STRING_TO_JSVAL( str );
    return JS_TRUE;
}



    JSBool
js_buffer_line( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    JSString *str;
    char * jsstr;
    char_u* line;
    int linenr = JSVAL_TO_INT( argv[0] );
    buf_T *buf = INSTANCE_BUFFER(cx,obj);

    line = ml_get_buf( buf , (linenr_T)linenr , FALSE );

    jsstr = strdup( (char *) line );

    str = JS_NewString( cx , jsstr , strlen( jsstr ) );
    *rval = STRING_TO_JSVAL( str );
    return JS_TRUE;
}

    JSString *
js_make_string( JSContext * cx , char * str ) 
{
    char * _str = strdup( str );
    return JS_NewString( cx , _str , strlen( _str ) );
}

    JSBool
js_buffer_lines( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    int n,i,start,end;
    buf_T *buf;

    JSObject *array;

    buf = INSTANCE_BUFFER(cx, obj);
    start = JSVAL_TO_INT(argv[0]);
    end = JSVAL_TO_INT(argv[1]);

    start = start < 0 ? 0 : start;
    end = end < 0 ? 0 : end;
    end = end < start ? start : end;
    n = end - start;

    array = JS_NewArrayObject(cx, 0, NULL);
    if( array == NULL )
	return JS_FALSE;

    jsval v;
    for( i=n ; i >= 0; i-- ) {
	JSString *str = js_make_string( cx , (char *)
		    ml_get_buf( buf , (linenr_T)i , FALSE ));
	v = STRING_TO_JSVAL( str );
	if (!JS_SetElement(cx, array , i, &v ))
	    return JS_FALSE;
    }

    *rval = OBJECT_TO_JSVAL( array );
    /*
     *
    int linenr = JSVAL_TO_INT( argv[0] );
    buf_T *buf = INSTANCE_BUFFER(cx,obj);

    line = ml_get_buf( buf , (linenr_T)linenr , FALSE );

    jsstr = strdup( (char *) line );

    str = JS_NewString( cx , jsstr , strlen( jsstr ) );
    *rval = STRING_TO_JSVAL( str );
    */
    return JS_TRUE;
}




    JSBool
js_buffer_prev( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    buf_T *buf,*prev;
    JSObject *jsobj;

    buf = INSTANCE_BUFFER(cx,obj);
    prev = buf->b_prev;

    if( ! prev ) {
	*rval = JSVAL_VOID;
	return JS_TRUE;
    }

    jsobj = js_buffer_new(cx, prev);
    *rval = OBJECT_TO_JSVAL( jsobj );
    return JS_TRUE;
}


    JSBool
js_buffer_next( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{
    buf_T *buf,*next;
    JSObject *jsobj;

    buf = INSTANCE_BUFFER(cx,obj);
    next = buf->b_next;

    if( ! next ) {
	*rval = JSVAL_VOID;
	return JS_TRUE;
    }

    jsobj = js_buffer_new(cx, next);
    *rval = OBJECT_TO_JSVAL( jsobj );
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





/* XXX: from buffer class constructor
 *
 * var buf = Buffer.find( 10 );
 * var buflist = Buffer.list();
 *
 */
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

    JSObject * jsobj;

    if (!JS_ConvertArguments(cx, argc, argv, "/j", &fnum)) {
	JS_ReportError(cx, "Can't convert buffer number");
	*rval = JSVAL_VOID;
	return JS_FALSE;
    }

    for (buf = firstbuf; buf; buf = buf->b_next) {
	if (buf->b_fnum == fnum) {
	    jsobj = js_buffer_new( cx , buf );

	    if( ! jsobj ) 
		return JS_FALSE;

	    *rval = OBJECT_TO_JSVAL( jsobj );
	    return JS_TRUE;
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
    uintN i; 
    JSString *str;
    char *bytes;
    char *message;

    message = (char *) alloc( sizeof(char) * 128 );

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        bytes = JS_EncodeString(cx, str);
        if (!bytes)
            return JS_FALSE;
	sprintf( message , "%s%s" , i ? " " : "", bytes );
        JS_free(cx, bytes);
    }  

    char *p = strchr(message, '\n');
    p = strchr( message , '\n' );
    if(p) 
	*p = '\0';
    MSG( message );

    *rval = JSVAL_VOID;
    //JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}


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

    js_env->rt = JS_NewRuntime( JS_RUNTIME_MEMORY * 1024L * 1024L);


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
