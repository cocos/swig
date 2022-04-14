/* File : example.i */
%module example

%{
#include "example.h"
%}

%define SWIG_INCLUDE(file) 
%include file
%enddef

%define SWIG_MY_INCLUDE(file)
%include file
%{
#include "file"
%}
%enddef

// /* Let's just grab the original header file here */
// // %include "example.h"

%typemap(in) (const char *fmt, ...) {
    $1 = "%s"; /* Fix format string to %s */
    $2 = (void *) PyString_AsString($input); /* Get string argument */
};

%typemap(in) MyClass * %{
    xlalalla
%}

namespace myns { namespace mysubns {

int my_printf(const char *fmt, ...);

}}

int my_printf(const char *fmt, ...);

%insert("header") %{
#include <myheader111.h>
%}



SWIG_MY_INCLUDE(Shape.h)

SWIG_INCLUDE(example.h)

class MyClass {
public:
    void foo();
    int m_a;
};

void myClassFoo(MyClass * aa);

// %extend(MyClass) {
//     %ignore(foo)
// }

// %insert("header") %{
// #include <myheader222.h>
// %}

// %begin %{
// // Hello World
// %}

// %inline %{
// /* Create a new vector */ 
// Vector *new_Vector() {
//     return (Vector *) malloc(sizeof(Vector));
// }
// %}



