#ifndef loShapes_H
#define loShapes_H
#include <string>
#include "DE1SoCfpga.h"
#include "Shapes.h"

// ------------------- LIST OF SHAPES -------------------------

class loShapes {
  public:
    virtual void drawShapes() {}
    virtual loShapes * addShape(Shape *shape) {}
    virtual loShapes * moveShapes() {}
    virtual void checkCollisions() {}
    virtual void checkCollisionsHelp(Shape *shape) {}
    virtual int size() {}
    virtual Shape * get(int index) {}
    virtual loShapes * placeShape(Shape *shape) {}
    virtual bool placeHelp(Shape *shape) {}

} ;

class loConsShapes : public loShapes, public Shape {
  public:
    Shape *shape ;
    loShapes *next ;

    loConsShapes(Shape *shape, loShapes *next) ;

    void drawShapes() ;
    loShapes * addShape(Shape *shape) ;
    loShapes * moveShapes() ;
    void checkCollisions() ;
    void checkCollisionsHelp(Shape *shape) ;
    int size() ;
    Shape * get(int index) ;
    loShapes * placeShape(Shape *shape) ;
    bool placeHelp(Shape *shape) ;

} ;

class loMtShapes : public loShapes, public Shape {
  public:

    loMtShapes() ;

    void drawShapes() ;
    loShapes * addShape(Shape *shape) ;
    loShapes * moveShapes() ;
    void checkCollisions() ;
    void checkCollisionsHelp(Shape *shape) ;
    int size() ;
    Shape * get(int index) ;
    loShapes * placeShape(Shape *shape) ;
    bool placeHelp(Shape *shape) ;

} ;

#endif
