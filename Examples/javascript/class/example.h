
// SWIG_INCLUDE(Shape.h)
// SWIG_MY_INCLUDE(Shape.h)

namespace cc { namespace gfx {
  struct MyStruct {
    int a;
    std::vector<float> b;
    %immutable;
    std::vector<std::list<int>> c;
    %mutable;
    std::variant<float, std::vector<int>> d;
  };
}}

namespace std {
   %template(vectorMyStruct) std::vector<MyStruct>;
};

/* File : example.h */
namespace cc { namespace gfx {

%apply int *OUTPUT { int *result };
%apply int *INPUT { int *x, int *y};
void my_add(int x, int y, int *result); 
int my_sub(int *x, int *y);



#define UP 1
#define DOWN 2 
#define RIGHT 3 
#define LEFT 4

int check_direction(int x);

%aggregate_check(int, check_direction, UP, DOWN, RIGHT, LEFT);

%contract globalMove(SomeObject *, int, int) { 
require:
  check_direction(direction); 
}

void globalMove(SomeObject *, int direction, int distance);

double sqrtAAA(double posdouble);

int globalOverloadFunc(int a);
int globalOverloadFunc(int a, float b);
int globalOverloadFunc(int a, float b, bool c);
int globalOverloadFunc(double a, const char* b);

void setStdVector(const std::vector<MyStruct>& arg);

class MyAbstractClass {
  public:
  virtual ~MyAbstractClass() = default;
  virtual void myAbstractMethod1() = 0;
  virtual void myAbstractMethod2() = 0;
  virtual void myAbstractMethod3() = 0;

  MyStruct mMyStruct;
};

class MySubClass : public MyAbstractClass {
  public:
  void myAbstractMethod1() override;
  void myAbstractMethod2() override;
  void myAbstractMethod3() override;
};

class Circle : public Shape {
private:
  double radius;
public:
  Circle(double r) : radius(r) { }

  double area() override;
  double perimeter() override;
};



//   typedef double Matrix4[4][4];
// Matrix4 x; // type = 'Matrix4', ltype='p.a(4).double'
// typedef const char * Literal;
// Literal y; // type = 'Literal', ltype='p.char'

class Square : public Shape {
private:
  double width;
public:

  enum MySquareEnum {
    MySquareEnum_1,
    MySquareEnum_2,
    MySquareEnum_3,
    MySquareEnum_4,
  };

  friend void blah(Square *f);

  MySquareEnum updateEnum(bool a, MySquareEnum e, float b);
  MySquareEnum updateEnumConst(const char* a, const MySquareEnum e, bool b);
  MySquareEnum updateEnumConstRef(std::string bbb, const std::string& a, const MySquareEnum &e, short b);

  enum class MyEnumClass {
    ONE,
    TWO,
    THREE
  };
  MyEnumClass updateEnumClass(bool a, MyEnumClass e, float b = 0.1f);
  MyEnumClass updateEnumClassConst(const char* a, const MyEnumClass e, bool b);
  MyEnumClass updateEnumClassConstRef(const std::string& a, const MyEnumClass &e, short b);

  %apply (std::string* INOUT) {std::string* pResult};
  std::string* convertString(const int & a, std::string* pResult);

  using MyString = std::string;
  MyString convertTypedDefString(MyString);

  Square(double w) : width(w) { }
  double area() override;
  double perimeter() override;
  void helloWorld(int aaa, bool isXXX, float    floatValue, MyType* myTypePtr);
  void helloWorld(bool);

  
  void testArr(vector<MyType>& myVec);

  // %serialize(mPublicInt)
  int mPublicInt;

  static float sPublicFloat;

  static void staticFoo();

  std::string myStdStringProperty;
  MyString myTypedDefStringProperty;
};

}}
