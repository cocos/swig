%include "Shape.h"

/* File : example.h */
namespace cc {



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

  int mPublicInt;
  static float sPublicFloat;

  static void staticFoo();
};

}
