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

SWIG_INCLUDE(example.h)

