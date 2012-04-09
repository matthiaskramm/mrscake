#include <stdbool.h>
#include <string.h>
#include "language_interpreter.h"

#ifdef _MSC_VER
# define XP_WIN
#else
# define XP_UNIX
#endif
#include "jsapi.h"

typedef struct _js_internal {
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    char*error;
} js_internal_t;

static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS };

static void error_callback(JSContext *cx, const char *message, JSErrorReport *report) {

    js_internal_t*js = JS_GetContextPrivate(cx);
    /*fprintf(stderr, "%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno, message);*/
    if(!js->error) {
        js->error = strdup(message);
    }
}

JSBool myjs_system(JSContext *cx, uintN argc, jsval *vp)
{
    JSString* str;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &str))
        return JS_FALSE;

    char*cmd = JS_EncodeString(cx, str);
    int rc = system(cmd);
    JS_free(cx, cmd);
    if (rc != 0) {
        // throw an exception
        JS_ReportError(cx, "Command failed with exit code %d", rc);
        return JS_FALSE;
    }
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

JSBool myjs_sqr(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble x;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "d", &x))
        return JS_FALSE;
    jsdouble result = x*x;
    JS_SET_RVAL(cx, vp, DOUBLE_TO_JSVAL(result));
    return JS_TRUE;
}

static JSFunctionSpec myjs_global_functions[] = {
    JS_FS("system", myjs_system, 1, 0),
    JS_FS("sqr", myjs_sqr, 1, 0),
    JS_FS_END
};

bool init_js(js_internal_t*js)
{
    js->rt = JS_NewRuntime(8L * 1024L * 1024L);
    if (js->rt == NULL)
        return false;
    js->cx = JS_NewContext(js->rt, 8192);
    if (js->cx == NULL)
        return false;

    JS_SetContextPrivate(js->cx, js);

    JS_SetOptions(js->cx, JSOPTION_VAROBJFIX | JSOPTION_JIT);
    JS_SetVersion(js->cx, JSVERSION_LATEST);
    JS_SetErrorReporter(js->cx, error_callback);

    //js->global = JS_NewObject(js->cx, &global_class, NULL, NULL);
    js->global = JS_NewCompartmentAndGlobalObject(js->cx, &global_class, NULL);
    if (js->global == NULL)
        return false;

    /* Populate the global object with the standard globals, like Object and Array. */
    if (!JS_InitStandardClasses(js->cx, js->global))
        return false;
    if (!JS_DefineFunctions(js->cx, js->global, myjs_global_functions))
        return false;
    return true;
}

static bool exec_js(language_interpreter_t*li, const char*script)
{
    js_internal_t*js = (js_internal_t*)li->internal;
    jsval rval;
    JSBool ok;
    ok = JS_EvaluateScript(js->cx, js->global, script, strlen(script), "__main__", 1, &rval);
    if(js->error)  {
        free(js->error);js->error=0;
    }
    return ok;
}

static int eval_js(language_interpreter_t*li, const char*script)
{
    js_internal_t*js = (js_internal_t*)li->internal;
    jsval rval;
    JSBool ok;
    ok = JS_EvaluateScript(js->cx, js->global, script, strlen(script), "__main__", 1, &rval);
    int i = -1;
    if (ok) {
        int32 d;
        ok = JS_ValueToInt32(js->cx, rval, &d);
        if(ok)
            i = d;
    }
    if(js->error)  {
        free(js->error);js->error=0;
    }
    return i;
}

void destroy_js(language_interpreter_t* li)
{
    js_internal_t*js = (js_internal_t*)li->internal;
    JS_DestroyContext(js->cx);
    JS_DestroyRuntime(js->rt);
    JS_ShutDown();
    free(js);
    free(li);
}

language_interpreter_t* javascript_interpreter_new()
{
    language_interpreter_t * li = calloc(1, sizeof(language_interpreter_t));
    li->name = "js";
    li->exec = exec_js;
    li->eval = eval_js;
    li->destroy = destroy_js;
    li->internal = calloc(1, sizeof(js_internal_t));
    js_internal_t*js = (js_internal_t*)li->internal;
    init_js(js);
    return li;
}

