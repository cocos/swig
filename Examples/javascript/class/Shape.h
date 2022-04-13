struct SomeObject { 
  float value[100];
};

namespace  cc { namespace gfx {

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

  int hello(int a);

  Shape* addChild(Shape* child);
  Shape* const & getChild() const;

  void setName(const char* name, char* prettyName);
  void setNameHaha2(char* name);

  void setInt(int positive);

  void testOut(int* out);
  void testMallocFree(int *items);
   void doSomeObject(SomeObject);

  void doDouble(double posdouble);


  static bool staticHello(float a);
  virtual double area() = 0;
  virtual double perimeter() = 0;
  static  int nshapes;
};

}}