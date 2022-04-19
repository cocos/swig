%insert(wrapper) %{

SWIGINTERN bool js_veto_set_variable(se::State& s)
{
    SE_REPORT_ERROR("%s, Tried to write read-only variable.", __FUNCTION__);
    return false;
}
SE_BIND_PROP_SET(js_veto_set_variable)
%}
