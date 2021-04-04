#ifndef Shapes_H
#define Shapes_H
#include <string>
#include "DE1SoCfpga.h"

//-------------------------- SHAPES -----------------------

class Shape : public DE1SoCfpga {
  public:
    int width ;
    int height ;
    int x ;
    int y ;
    int direcx ;
    int direcy ;
    short color ;
    int type ;

    virtual void draw() {}
    virtual void clearShape() {}
    Shape * move() ;
    bool isCollided(Shape *b) ;
    void bounce(Shape *b) ;
    void edgeCheck() ;
} ;

class Square : public Shape {
  public:
    Square(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class Cross : public Shape {
  public:
    Cross(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class Circle : public Shape {
  public:
    Circle(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class Triangle : public Shape {
  public:
    Triangle(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class Rectangle : public Shape {
  public:
    Rectangle(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class Star : public Shape {
  public:
    Star(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class shapeX : public Shape {
  public:
    shapeX(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class shapeL : public Shape {
  public:
    shapeL(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class shapeT : public Shape {
  public:
    shapeT(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

class shapeV : public Shape {
  public:
    shapeV(int x, int y, int width, short color, int direcx, int direcy, int id) ;

    void draw() ;
    void clearShape() ;
} ;

//----- SHAPE CHANGER -----
class shapeChanger {
  public:

    Shape * changeShape(Shape *shape) ;
} ;

#endif
