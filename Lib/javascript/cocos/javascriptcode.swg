/* -----------------------------------------------------------------------------
 * js_ctor:  template for wrapping a ctor.
 *   - $jswrapper:        wrapper of called ctor
 *   - $jslocals:         locals part of wrapper
 *   - $jscode:           code part of wrapper
 *   - $jsargcount:       number of arguments
 *   - $jsmangledtype:    mangled type of class
 * ----------------------------------------------------------------------------- */
%fragment ("js_ctor", "templates")
%{
// js_ctor
static bool $jswrapper(se::State& s) // NOLINT(readability-identifier-naming)
{
    CC_UNUSED bool ok = true;
    const auto& args = s.args();
    size_t argc = args.size();
    if (argc != $jsargcount) {
      SE_REPORT_ERROR("$jswrapper wrong number of arguments: %d, was expecting %d", (int)argc, $jsargcount);
      return;
    }

    $jslocals
    $jscode
    auto *ptr = JSB_MAKE_PRIVATE_OBJECT_WITH_INSTANCE(result);
    s.thisObject()->setPrivateObject(ptr);
    return true;
}
SE_BIND_CTOR($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_veto_ctor:  a vetoing ctor for abstract classes
 *   - $jswrapper:        name of wrapper
 *   - $jsname:           class name
 * ----------------------------------------------------------------------------- */
%fragment ("js_veto_ctor", "templates")
%{
static bool $jswrapper(se::State& s) // NOLINT(readability-identifier-naming)
{
    // js_veto_ctor
    SE_REPORT_ERROR("Class $jsname can not be instantiated");
fail:
    return 0;
}
SE_BIND_CTOR($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_ctor_dispatcher:  dispatcher for overloaded constructors
 *   - $jswrapper:        name of wrapper
 *   - $jsname:           class name
 *   - $jsdispatchcases:  part containing code for dispatching
 * ----------------------------------------------------------------------------- */
%fragment ("js_ctor_dispatcher", "templates")
%{
static bool $jswrapper(se::State& s) // NOLINT(readability-identifier-naming)
{
    // js_ctor_dispatcher
    size_t argc = args.size();
    bool ret = false;

    // switch all cases by means of series of if-returns.
    $jsdispatchcases

    // default:
    SE_REPORT_ERROR("Illegal arguments for construction of $jsname");
    return false;
}
SE_BIND_CTOR($jswrapper, __jsb$jsmangledname_class, _wrap_delete_$jsname) %}

/* -----------------------------------------------------------------------------
 * js_overloaded_ctor:  template for wrapping a ctor.
 *   - $jswrapper:        wrapper of called ctor
 *   - $jslocals:         locals part of wrapper
 *   - $jscode:           code part of wrapper
 *   - $jsargcount:       number of arguments
 *   - $jsmangledtype:    mangled type of class
 * ----------------------------------------------------------------------------- */
%fragment ("js_overloaded_ctor", "templates")
%{
static bool $jswrapper(se::State& s) // NOLINT(readability-identifier-naming)
{
    // js_overloaded_ctor
    const auto& args = s.args();
    $jslocals
    $jscode
    auto *ptr = JSB_MAKE_PRIVATE_OBJECT_WITH_INSTANCE(result);
    s.thisObject()->setPrivateObject(ptr);
    return true;
}
%}

/* -----------------------------------------------------------------------------
 * js_ctor_dispatch_case:  template for a dispatch case for calling an overloaded ctor.
 *   - $jsargcount:       number of arguments of called ctor
 *   - $jswrapper:        wrapper of called ctor
 *
 *  Note: a try-catch-like mechanism is used to switch cases
 * ----------------------------------------------------------------------------- */
%fragment ("js_ctor_dispatch_case", "templates")
%{
  // js_ctor_dispatch_case
  if(argc == $jsargcount) {
    ret = $jswrapper(s);
    if (ret) { return ret; } /* reset exception and return */
  }
%}


/* -----------------------------------------------------------------------------
 * js_dtor:  template for a destructor wrapper
 *   - $jsmangledname:  mangled class name
 *   - $jstype:         class type
 * ----------------------------------------------------------------------------- */
%fragment ("js_dtor", "templates")
%{
// js_dtor
static bool $jswrapper(se::State& s) {
  return true;
}
SE_BIND_FINALIZE_FUNC($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_dtor:  template for a destructor wrapper
 *   - $jsmangledname:  mangled class name
 *   - $jstype:         class type
 *   - ${destructor_action}: The custom destructor action to invoke.
 * ----------------------------------------------------------------------------- */
%fragment ("js_dtoroverride", "templates")
%{
static void $jswrapper(JSObjectRef thisObject)
{// js_dtoroverride
  return true;
}
SE_BIND_FINALIZE_FUNC($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_getter:  template for getter function wrappers
 *   - $jswrapper:  wrapper function name
 *   - $jslocals:   locals part of wrapper
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_getter", "templates")
%{
static bool $jswrapper(se::State& s)
{
    bool ok = true;
    $jslocals
    $jscode
    return true;
}
SE_BIND_PROP_GET($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_setter:  template for setter function wrappers
 *   - $jswrapper:  wrapper function name
 *   - $jslocals:   locals part of wrapper
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_setter", "templates")
%{
static bool $jswrapper(se::State& s)
{
    bool ok = true;
    $jslocals
    $jscode
    return true;
}
SE_BIND_PROP_SET($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_function:  template for function wrappers
 *   - $jswrapper:  wrapper function name
 *   - $jslocals:   locals part of wrapper
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_function", "templates")
%{
static bool $jswrapper(se::State& s)
{
  // js_function
  CC_UNUSED bool ok = true;
  const auto& args = s.args();
  size_t argc = args.size();
  $jslocals
  if(argc != $jsargcount) {
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, $jsargcount);
    return false;
  }
  $jscode
  return true;
}
SE_BIND_FUNC($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_function_dispatcher:  template for a function dispatcher for overloaded functions
 *   - $jswrapper:  wrapper function name
 *   - $jsname:     name of the wrapped function
 *   - $jslocals:   locals part of wrapper
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_function_dispatcher", "templates")
%{
static bool $jswrapper(se::State& s)
{
  // js_function_dispatcher
  CC_UNUSED bool ok = true;
  const auto& args = s.args();
  size_t argc = args.size();
  $jslocals
  $jscode
  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, $jsargcount);
  return false;
}
SE_BIND_FUNC($jswrapper) %}

/* -----------------------------------------------------------------------------
 * js_overloaded_function:  template for a overloaded function
 *   - $jswrapper:  wrapper function name
 *   - $jslocals:   locals part of wrapper
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_overloaded_function", "templates")
%{
static bool $jswrapper(se::State& s)
{// js_overloaded_function
  $jslocals
  $jscode
  *p_result = jsresult;
  return true;
}%}

/* -----------------------------------------------------------------------------
 * js_function_dispatch_case:  template for a case used in the function dispatcher
 *   - $jswrapper:  wrapper function name
 *   - $jsargcount: number of arguments of overloaded function
 *   - $jscode:     code part of wrapper
 * ----------------------------------------------------------------------------- */
%fragment ("js_function_dispatch_case", "templates")
%{// js_function_dispatch_case
  if (argc == $jsargcount) {
     ok = $jswrapper(s);
     if (ok) { return true; }
  } %}

/* -----------------------------------------------------------------------------
 * jsc_variable_declaration:  template for a variable table entry
 *   - $jsname:       name of the variable
 *   - $jsgetter:     wrapper of getter function
 *   - $jssetter:     wrapper of setter function
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_variable_declaration", "templates")
%{ cls->defineProperty("$jsname", _SE($jsgetter), _SE($jssetter)); %}

%fragment ("jsc_static_variable_declaration", "templates")
%{ cls->defineStaticProperty("$jsname", _SE($jsgetter), _SE($jssetter)); %}

%fragment ("jsc_global_variable_declaration", "templates")
%{ ns->defineProperty("$jsname", _SE($jsgetter), _SE($jssetter)); %}

/* -----------------------------------------------------------------------------
 * jsc_function_declaration:  template for a function table entry
 *   - $jsname:       name of the variable
 *   - $jswrapper:    wrapper function
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_function_declaration", "templates")
%{ cls->defineFunction("$jsname", _SE($jswrapper)); %}

%fragment ("jsc_static_function_declaration", "templates")
%{ cls->defineStaticFunction("$jsname", _SE($jswrapper)); %}

%fragment ("jsc_global_function_declaration", "templates")
%{ ns->defineFunction("$jsname", _SE($jswrapper)); %}

/* -----------------------------------------------------------------------------
 * jsc_classtemplate_declaration:  template for a namespace declaration
 *   - $jsmangledname:      mangled class name
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_class_declaration", "templates")
%{
se::Class* __jsb_$jsmangledname_class;
se::Object* __jsb_$jsmangledname_proto;
SE_DECLARE_FINALIZE_FUNC(js_delete_$jsname) %}

/* -----------------------------------------------------------------------------
 * jsc_define_class_template:  template for defining a class template
 *   - $jsmangledname:            mangled class name
 *   - $jsmangledtype:            mangled class type
 *   - $jsctor:                   wrapper of ctor
 *   - $jsbaseclass:              mangled name of base class
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_class_definition", "templates")
%{
bool js_register_$jsmangledname(se::Object* obj) {
    $jsclass_inheritance
    $jsclassvariables
    $jsclassfunctions
    $jsstaticclassvariables
    $jsstaticclassfunctions
    cls->defineFinalizeFunction(_SE($jsdtor));
    cls->install();
    JSBClassType::registerClass<$jsclassname>(cls);

    __jsb_$jsmangledname_proto = cls->getProto();
    __jsb_$jsmangledname_class = cls;
    se::ScriptEngine::getInstance()->clearException();
    return true;
}
%}

%fragment ("jsc_class_inherit", templates)
%{ auto* cls = se::Class::create("$jsname", obj, __jsb_$jsbaseclassmangled_proto, $jsctor); %}

%fragment ("jsc_class_noinherit", templates)
%{ auto* cls = se::Class::create("$jsname", obj, nullptr, $jsctor); %}

/* -----------------------------------------------------------------------------
 * jsc_register_class:  template for registration of a class
 *   - $jsname:                   class name
 *   - $jsmangledname:            mangled class name
 *   - $jsnspace:                 mangled name of namespace
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_class_registration", "templates")
%{ js_register_$jsmangledname(ns); %}


/* -----------------------------------------------------------------------------
 * jsc_nspace_definition:  template for definition of a namespace object
 *   - $jsmangledname:            mangled name of namespace
 * ----------------------------------------------------------------------------- */
%fragment ("jsc_global_registration", "templates")
%{$jsglobalvariables
  $jsglobalfunctions %}

