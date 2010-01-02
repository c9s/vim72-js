/*
 * vim:ts=8:sw=4:noet:
 *
 * JavaScript interface by Cornelius Lin <cornelius.howl@gmail.com>
 *
 * See https://developer.mozilla.org/en/JavaScript_C_Engine_Embedder's_Guide
 * for more details.
 *
 */
#include "vim.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "jsapi.h"

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

/* JS global variables. */
JSRuntime *rt;
JSContext *cx;
JSObject *global;


/* The javascript error reporter callback. */
    void
reportError(cx, message, report)
    JSContext 		*cx;
    const char 		*message;
    JSErrorReport 	*report;
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
js_get_buffer_count( cx , obj , argc ,argv , rval )
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

    JSBool
js_get_buffer_count( cx , obj , argc ,argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{

    int	    fnum;
    buf_T   *buf;
    for (buf = firstbuf; buf; buf = buf->b_next)
	if (buf->b_fnum == fnum)
	    return buffer_new(buf);


}


    JSBool
js_vim_message( cx , obj , argc , argv , rval )
    JSContext	*cx;
    JSObject	*obj; 
    uintN	argc;
    jsval	*argv; 
    jsval	*rval;
{

    const char *message;
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
    JS_FS("alert", js_vim_message, 1, 0, 0),
    JS_FS("message", js_vim_message, 1, 0, 0),
    JS_FS("buf_cnt", js_get_buffer_count, 0, 0, 0),
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
    JSScript *script = JS_CompileFile(cx, global, filename);

    if (script) {
	if (!compileOnly)
	    (void) JS_ExecuteScript(cx, global, script, NULL);
	JS_DestroyScript(cx, script);
    }
}



void
vim_js_init(arg)
    char *arg;
{
    /* 3 things to init runtime , context and global object */
    rt = JS_NewRuntime(8L * 1024L * 1024L);
    if (rt == NULL)
	return;

    cx = JS_NewContext(rt, 8192);
    if (cx == NULL)
	return;
    JS_SetOptions(cx, JSOPTION_VAROBJFIX);
    JS_SetVersion(cx, JSVERSION_LATEST);
    JS_SetErrorReporter(cx, reportError);

    global = JS_NewObject(cx, &global_class, NULL, NULL);

    if (global == NULL)
	return;

    /*
     * Populate the global object with the standard globals, like Object
     * and Array.
     */
    if (!JS_InitStandardClasses(cx, global))
	return;

    if (!JS_DefineFunctions(cx, global, js_global_functions))
	return;
}

    void
js_end()
{

    /* Cleanup. */
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();

}
