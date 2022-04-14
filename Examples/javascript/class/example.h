
// SWIG_INCLUDE(Shape.h)
// SWIG_MY_INCLUDE(Shape.h)

/* File : example.h */
namespace cc { namespace gfx {

struct MyStruct {
  int a;
  std::vector<float> b;
  std::vector<std::list<int>> c;
  std::variant<float, std::vector<int>> d;
};

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

class MyAbstractClass {
  public:
  virtual ~MyAbstractClass() = default;
  virtual void myAbstractMethod1() = 0;
  virtual void myAbstractMethod2() = 0;
  virtual void myAbstractMethod3() = 0;
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
};

}}
