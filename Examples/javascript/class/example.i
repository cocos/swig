/* File : example.i */
%module example

%{
#include "example.h"
%}

%define SWIG_INCLUDE(file) 
%include #file
%enddef

/* Let's just grab the original header file here */
// %include "example.h"

%typemap(in) (const char *fmt, ...) {
    $1 = "%s"; /* Fix format string to %s */
    $2 = (void *) PyString_AsString($input); /* Get string argument */
};

int my_printf(const char *fmt, ...);

%insert("header") %{
#include <myheader111.h>
%}

SWIG_INCLUDE(Shape.h)

SWIG_INCLUDE(example.h)

%insert("header") %{
#include <myheader222.h>
%}

%begin %{
// Hello World
%}

%inline %{
/* Create a new vector */ 
Vector *new_Vector() {
    return (Vector *) malloc(sizeof(Vector));
}
%}

%init %{ 
    init_variables222();
%}

%init %{ 
    init_variables();
%}
