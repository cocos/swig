/* File : example.i */
%module(target_namespace="MyTargetNS") example

// %include <std_string.i>
// %include <std_vector.i>
%include <attribute.i>

// %import stlmodule.i

%{
// #include "example.h"
#include "example_wrap.h"
%}

%virtual_inherit(cc::InterfaceA);
%virtual_inherit(cc::InterfaceB);
%virtual_inherit(cc::NativeA);


namespace cc {

class VirtualInheritBase {
protected:
    VirtualInheritBase() = default;
public:
    virtual ~VirtualInheritBase() = default;
    void virtualBaseFoo();
};

class InterfaceA : virtual public VirtualInheritBase {
public:
    virtual void foo_a() = 0;
protected:
    InterfaceA();
};

class NativeA : public InterfaceA {
public:
    void foo_a() override;
};

class InterfaceB : virtual public InterfaceA {
public:
    virtual void foo_b() = 0;
protected:
    InterfaceB();
};

class Manager {
public:
    InterfaceA* createA() {};
    InterfaceB* createB() {};
};

}


//%rename(DDD) myns1::ns2::ns3::AAA;
//%rename(EEE) myns1::ns2::ns3::AAA::BBB;
// %rename(FFF) myns1::ns2::ns3::AAA::BBB::CCC;


// namespace myns1::ns2::ns3 {

// class AAA {
// public:
//     void fooA();

//     class BBB {
//     public:
//         void fooB();

//         class CCC {
//         public:
//             CCC(int a);
//             CCC(int a, bool);
//             CCC(int a, bool, float);
//             void fooC();
//         };
//     };

//     // struct ISubMesh {
//     //     std::vector<uint32_t> vertexBundelIndices;
//     //     int a;
//     // };
// };

// }


// static const uint32_t MY_GLOBAL = 4;

// %attribute(RenderEntity, uint32_t, myInt, getMyInt, setMyInt);


// struct RenderEntity final {
// public:
//     static const  uint32_t STATIC_DRAW_INFO_CAPACITY;
//     static const std::string hello = "hello";
//     static const std::string NAME;

//     // uint32_t getMyInt();
//     // void setMyInt(uint32_t i);

//     // void foo();
// };

// namespace cc  { namespace gfx {

//     class Color {
//     public:

//         // class InternalColor {
//         // public:
//         //     InternalColor(int x);
//         //     InternalColor(int x, int y);

//         //     void foo(int x);
//         //     void foo(int x, int y);

//         // private:
//         //     int x;
//         //     int y;
//         // };

//         Color(uint8_t r);
//         Color(uint8_t r, uint8_t g);
//         Color(uint8_t r, uint8_t g, uint16_t);

//         // void haha(int x);
//         // void haha(int x, int y);
//     private:
//         uint8_t r;
//         uint8_t g;
//         uint8_t b;
//         uint8_t a;
//     };
// }}


// namespace cc::gfx {

// #define EXPOSE_COPY_FN(type)      \
//     type &copy(const type &rhs) { \
//         *this = rhs;              \
//         return *this;             \
//     }

// struct Size {
//     public:
//     // Size(uint32_t x = 0, uint32_t y = 0) {}

//     uint32_t x{0};
//     uint32_t y{0};
//     uint32_t z{0};

//     EXPOSE_COPY_FN(Size)
// };

// }

// %define SWIG_INCLUDE(file) 
// %include file
// %enddef

// %define SWIG_MY_INCLUDE(file)
// %include file
// %{
// #include "file"
// %}
// %enddef

// %module_macro(USE_A_CLASS) A;
// class A {

// };

// %release_returned_cpp_object_in_gc(TestFeature::createA);

// %module_macro(USE_TEST_FEATURE) TestFeature;

// %module_macro(USE_A) TestFeature::createA;
// %module_macro(USE_B) TestFeature::fooB;

// %module_macro(USE_OVERLOAD) TestFeature::testOverload;

// %module_macro(USE_GETTER_SETTER) TestFeature::myattr;

// %attribute(TestFeature, int, myattr, get_attr, set_attr);
// %attribute(TestFeature, int, myattr2, get_attr2, set_attr2);



// class TestFeature {
// public:
//     // %feature("module_macro", "USE_A");
//     static A* createA();

//     static void fooB();
//     void foo(int a, int b, A*);

//     void testOverload(int a);
//     void testOverload(int a, int b);
//     void testOverload(int a, int b, bool c);

//     void set_attr(int v);
//     int get_attr() const;

//     void set_attr2(int v);
//     int get_attr2() const;
    
//     // float floatValue1;
//     // int intValue1;
// };

// class TestNoFeature {
//     public:
//     void hello();
// };

// static void hello_static_global();

// void hello_global();

// extern int aaa;
// static int staticAAA;

// // struct StaticPropTest {
// //     static float staticFloatProp;
// // };

// // struct MySize {
// //     float width;
// // // float height;
// // };

// extern int MY_MAX_VALUE;

// class Mesh {
// public:
//     static int MESH_AAA = 0;
//     struct ISubMesh {

//         int prop;
//         class SubSubClass {
//             public:
//             int ohno;
//         };
//     };
//     void foo();
//     static void staticFoo();
// };

// class Camera {
// public:
//     static constexpr int32_t SKYBOX_FLAG = 0;
// };

// %attribute(MyRect, MySize, size, getSize, setSize);
// %attribute(MyRect, ccstd::string&, name, getName, setName);
// %attribute(MyRect, %arg(ccstd::map<int, MySize>&), mymap, getMap, setMap);
// struct MyRect {
    // static int size;
    // static MySize size;
    // MySize size;
    // MySize *size;
    // cc::IntrusivePtr<MySize> size;
    // int size;
    // ccstd::string size;

    // void setSize(const MySize& size);
    // const MySize& getSize() const;

    // void setName(const ccstd::string& name);
    // const ccstd::string& getName() const;

    // void setMap(const ccstd::map<int, MySize>& map);
    // const ccstd::map<int, MySize>& getMap() const;

    // ccstd::map<int, MySize> prop_map;
// MySize* size2;
// };

// struct SharedPtrPropTest {
    // std::shared_ptr<MySize> aaa;
// };

// %rename(globalFooFloat) globalFoo(float);

// void globalFoo(int a);
// void globalFoo(float a);

// namespace MyNS { namespace subns { namespace ns3 {
//     %rename(memberFooInClassFloat) MyClassA::memberFooInClass(float);
//     %rename(staticFooInClassFloat) MyClassA::staticFooInClass(float);
//     %rename(globalFooInNSFloat) globalFooInNS(float);

//     void globalFooInNS(float a);
//     void globalFooInNS(int a);

//     class MyClassA {
//     public:
//         static void staticFooInClass(bool a);
//         static void staticFooInClass(float a);
//         void memberFooInClass(int a);
//         void memberFooInClass(float a);
//     };
// }}}

// %attribute_writeonly(MyClassTestWriteOnlyProp, int, myvar, setVar);
// %attribute(MyClassTestWriteOnlyProp, float, myvar2, getVar2, setVar2);
// %attribute(MyClassTestWriteOnlyProp, double, myvar3, getVar3);

// class MyClassTestWriteOnlyProp {
// public:
//     void setVar(int a);
//     void setVar2(float a);
//     float getVar2() const;

//     double getVar3() const;
// };




// /* Set the input argument to point to a temporary variable */ 
// %typemap(in, numinputs=0) int *out (int temp) {
// $1 = &temp;
// }

// %typemap(argout) int *out {
// // Append output value $1 to $result ...
//   *out = $1;
// }



// // Get a list of integers 
// %typemap(in) int *items {
// int nitems = Length($input);
// $1 = (int *) malloc(sizeof(int)*nitems); }

// // Free the list
// %typemap(freearg) int *items {
// free($1); }

// void fooVecorInt(const std::vector<int>& arr);

// /* Let's just grab the original header file here */
// // %include "example.h"

// %typemap(in) (const char *fmt, ...) {
//     $1 = "%s"; /* Fix format string to %s */
//     $2 = (void *) PyString_AsString($input); /* Get string argument */
// };



// %typemap(in) MyClass * %{
//     xlalalla
// %}

// namespace myns { namespace mysubns {

// int my_printf(const char *fmt, ...);

// }}

// int my_printf(const char *fmt, ...);

// %insert("header") %{
// #include <myheader111.h>
// %}



// SWIG_MY_INCLUDE(Shape.h)

// SWIG_INCLUDE(example.h)

// class MyClass {
// public:
//     void foo();
//     int m_a;
// };

// void myClassFoo(MyClass * aa);

// // %extend(MyClass) {
// //     %ignore(foo)
// // }

// // %insert("header") %{
// // #include <myheader222.h>
// // %}

// // %begin %{
// // // Hello World
// // %}

// // %inline %{
// // /* Create a new vector */ 
// // Vector *new_Vector() {
// //     return (Vector *) malloc(sizeof(Vector));
// // }
// // %}



// // A typemap defining how to return an argument by appending it to the result 
// %typemap(argout) double *outvalue {
//     Tcl_Obj *o = Tcl_NewDoubleObj($1);
//     Tcl_ListObjAppendElement(interp, $result, o);
// }
// // A typemap telling SWIG to ignore an argument for input
// // However, we still need to pass a pointer to the C function 

// %typemap(in, numinputs=0) double *outvalue (double temp) {
//     $1 = &temp; 
// }
// // Now a function returning two values
// int mypow(double a, double b, double *outvalue) {
//     if ((a < 0) || (b < 0)) return -1; *outvalue = pow(a, b);
//     return 0;
// };

// // %typemap(memberin) int myvalue[4] {
// //     //cjh my memberin
// //      memmove($1, $input, 4*sizeof(int));
// // }

// // class MyMemberInTestClass {
// //     public:
// //     void myMemberInTest(int myvalue[4]);


// // };

// typedef struct {
//     MY_UNICODE value[512];
// } TEST_STRUCTURE;

// %typemap(memberin) MY_UNICODE [512] {
//     if(UTF8ToMyUnicode($1, $input) != 0) {
//         return NULL;
//     }
// }

// %typemap(memberout) MY_UNICODE [512] {
//     if(MyUnicodeToUTF8($1, $input) != 0) {
//         return NULL;
//     }
// }  

// int UTF8ToMyUnicode(char *utf8, MY_UNICODE *unicode);
// int MyUnicodeToUTF8(MY_UNICODE *unicode, char *utf8);

// void myTestStructure(TEST_STRUCTURE* p);

// %include <arrays_javascript.i>
// %inline %{
//     extern int FiddleSticks[3];
// %}

