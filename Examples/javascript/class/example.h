/* File : example.h */
namespace cc {
class Shape {
public:
  Shape() {
    nshapes++;
  }
  Shape(int shapes) {
    nshapes = shapes;
  }
  Shape(float* p) {

  }

  Shape(Shape* p) {

  }
  virtual ~Shape() {
    nshapes--;
  }
  // double  x, y;   
  void    move(double dx, double dy);
  void move(bool doMove);
  // virtual double area() = 0;
  // virtual double perimeter() = 0;
  static  int nshapes;
};

// class Circle : public Shape {
// private:
//   double radius;
// public:
//   Circle(double r) : radius(r) { }
//   virtual double area();
//   virtual double perimeter();
// };



//   typedef double Matrix4[4][4];
// Matrix4 x; // type = 'Matrix4', ltype='p.a(4).double'
// typedef const char * Literal;
// Literal y; // type = 'Literal', ltype='p.char'

// class Square : public Shape {
// private:
//   double width;
// public:

//   int *axx[30];
//   %rename(area_renamed) area;

//   Square(double w) : width(w) { }
//   virtual double area();
//   virtual double perimeter();
//   void helloWorld(int aaa, bool isXXX, float    floatValue, MyType* myTypePtr);
//   void helloWorld(bool);
//   void testArr(vector<MyType>& myVec);
// };

}
