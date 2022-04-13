//cjh %insert(init) "swiginit.swg"

// Open the initializer function
%insert(init)
// Im init block
%{%}

/* -----------------------------------------------------------------------------
 * js_initializer:  template for the module initializer function
 *   - $jsname:                   module name
 *   - $jscreatenamespaces:       part with code for creating namespace objects
 *   - $jsregisternamespaces:     part with code for registration of namespaces
 * ----------------------------------------------------------------------------- */
%fragment ("js_initializer", "templates") %{
bool register_all_$jsname(se::Object* obj) {
    // Get the ns
    se::Value nsVal;
    if (!obj->getProperty("$jsname", &nsVal, true))
    {
        se::HandleObject jsobj(se::Object::createPlainObject());
        nsVal.setObject(jsobj);
        obj->setProperty("$jsname", nsVal);
    }
    se::Object* ns = nsVal.toObject();
    /* Register classes */
    $jsregisterclasses
    /* Register global variables & global functions */
    $jsregisternamespaces
    return true;
}
%}
