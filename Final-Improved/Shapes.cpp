#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "DE1SoCfpga.h"
#include "Shapes.h"
using namespace std;

// --------------- SHAPE CLASS ----------------

bool Shape::isCollided(Shape *b) {
      return ((this->x + this->width) >= b->x) &&
             (this->x <= (b->x + b->width)) &&
             ((this->y + this->height) >= b->y) &&
             (this->y <= (b->y + b->height)) ;
}

void Shape::bounce(Shape *b) {
    playNote() ;
      if ((this->x == b->x + b->width && this->y == b->y + b->height) || (b->x == this->x + this->width && b->y == this->y + this->height) ||
          (this->y == b->y + b->height && b->x == this->x + this->width) || (b->y == this->y + this->height && this->x == b->x + b->width)) {
        this->direcx *= -1 ;
        b->direcx *= -1 ;
        this->direcy *= -1 ;
        b->direcy *= -1 ;
      } else if (this->y == b->y + b->height || b->y == this->y + this->height) {
        this->direcy *= -1 ;
        b->direcy *= -1 ;
      } else if (this->x + this->width == b->x || b->x + b->width == this->x) {
        this->direcx *= -1 ;
        b->direcx *= -1 ;
      }
}

void Shape::edgeCheck() {
      if(this->x < 10) {
        this->direcx = 1 ;
        playNote() ;
      } else if (this->x > (310 - this->width)) {
        this->direcx = -1 ;
        playNote() ;
      }

      if(this->y < 10) {
        this->direcy = 1 ;
        playNote() ;
      } else if (this->y > (230 - this->height)) {
        this->direcy = -1 ;
        playNote() ;
      }
}

Shape * Shape::move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
}

//-------------------------- INDIVIDUAL SHAPE CONSTRUCTORS AND DRAW/CLEAR FUNCTIONS

//----- SQUARE -----
Square::Square(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
}

void Square::draw() {
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, this->color) ;
    write_pixel(i, this->y + this->height, this->color) ;
  }
  for (int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, this->color) ;
    write_pixel(this->x + this->width, i, this->color) ;
  }
}

void Square::clearShape() {
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, resample_rgb(db, 0xFFFFFF)) ;
    write_pixel(i, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
  }
  for (int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, resample_rgb(db, 0xFFFFFF)) ;
    write_pixel(this->x + this->width, i, resample_rgb(db, 0xFFFFFF)) ;
  }
}

// ---- CROSS ----
Cross::Cross(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void Cross::draw() {
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    if(this->x + this->width / 4 < i && i < this->x + 3 * (this->width / 4)) {
      write_pixel(i, this->y, this->color) ;
      write_pixel(i, this->y + this->height, this->color) ;
    } else {
      write_pixel(i, this->y + this->height / 4, this->color) ;
      write_pixel(i, this->y + 3 * (this->height / 4), this->color) ;
    }
  }

  for (int i = this->y ; i <= this->y + this->height ; i++) {
    if(this->y + this->height / 4 < i && i < this->y + 3 * (this->height / 4)) {
      write_pixel(this->x, i, this->color) ;
      write_pixel(this->x + this->width, i, this->color) ;
    } else {
      write_pixel(this->x + this->width / 4, i, this->color) ;
      write_pixel(this->x + 3 * (this->width / 4), i, this->color) ;
    }
  }
}

void Cross::clearShape() {
  short int color = resample_rgb(db, 0xFFFFFF) ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    if(this->x + this->width / 4 < i && i < this->x + 3 * (this->width / 4)) {
      write_pixel(i, this->y, color) ;
      write_pixel(i, this->y + this->height, color) ;
    } else {
      write_pixel(i, this->y + this->height / 4, color) ;
      write_pixel(i, this->y + 3 * (this->height / 4), color) ;
    }
  }

  for (int i = this->y ; i <= this->y + this->height ; i++) {
    if(this->y + this->height / 4 < i && i < this->y + 3 * (this->height / 4)) {
      write_pixel(this->x, i, color) ;
      write_pixel(this->x + this->width, i, color) ;
    } else {
      write_pixel(this->x + this->width / 4, i, color) ;
      write_pixel(this->x + 3 * (this->width / 4), i, color) ;
    }
  }
}

// --- CIRCLE ----
Circle::Circle(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void Circle::draw() {
  int xAcc = this->x ;
  int yAcc = this->y + this->height / 2 ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, this->color) ;
    yAcc += (i <= this->x + this->width / 2) ?  1 : -1 ;
  }
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, this->color) ;
    yAcc += (i < this->x + this->width / 2) ?  -1 : 1 ;
  }
}

void Circle::clearShape() {
  short int color = resample_rgb(db, 0xFFFFFF) ;
  int xAcc = this->x ;
  int yAcc = this->y + this->height / 2 ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, color) ;
    yAcc += (i <= this->x + this->width / 2) ?  1 : -1 ;
  }
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, color) ;
    yAcc += (i < this->x + this->width / 2) ?  -1 : 1 ;
  }
}

//---- TRIANGLE ----
Triangle::Triangle(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void Triangle::draw() {
  int xAcc = this->x ;
  int yAcc = this->y + this->height ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, this->color) ;
    write_pixel(i, this->y + this->height, this->color) ;
    yAcc += (i <= this->x + this->width / 2) ?  -1 : 1 ;
  }
}

void Triangle::clearShape() {
  short int color = resample_rgb(db, 0xFFFFFF) ;
  int xAcc = this->x ;
  int yAcc = this->y + this->height ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, yAcc, color) ;
    write_pixel(i, this->y + this->height, color) ;
    yAcc += (i <= this->x + this->width / 2) ?  -1 : 1 ;
  }
}

// ---- RECTANGLE ----
Rectangle::Rectangle(int x, int y, int width, int height, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = height ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void Rectangle::draw() {
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, this->color) ;
    write_pixel(i, this->y + this->height, this->color) ;
  }
  for (int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, this->color) ;
    write_pixel(this->x + this->width, i, this->color) ;
  }
}

void Rectangle::clearShape() {
  short int color = resample_rgb(db, 0xFFFFFF) ;
  for (int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, color) ;
    write_pixel(i, this->y + this->height, color) ;
  }
  for (int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, color) ;
    write_pixel(this->x + this->width, i, color) ;
  }
}

//----- STAR ----
Star::Star(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void Star::draw() {
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y + this->height / 2, this->color) ;
  }
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x + this->width / 2, i, this->color) ;
  }

  int xAcc = this->x + this->width / 4 ;
  int yAcc = this->y + this->height / 4;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x + this->width / 4 ;
  yAcc = this->y + this->height - this->height / 4;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc++ ;
    yAcc-- ;
  }
}

void Star::clearShape() {
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y + this->height / 2, resample_rgb(db, 0xFFFFFF)) ;
  }
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x + this->width / 2, i, resample_rgb(db, 0xFFFFFF)) ;
  }

  int xAcc = this->x + this->width / 4 ;
  int yAcc = this->y + this->height / 4;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x + this->width / 4 ;
  yAcc = this->y + this->height - this->height / 4;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc++ ;
    yAcc-- ;
  }
}

// ----- SHAPEX ------
shapeX::shapeX(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void shapeX::draw() {
  int xAcc = this->x ;
  int yAcc = this->y;

  for(int i = 0 ; i <= this->width ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x ;
  yAcc = this->y + this->height;
  for(int i = 0 ; i <= this->width ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc++ ;
    yAcc-- ;
  }
}

void shapeX::clearShape() {
  int xAcc = this->x ;
  int yAcc = this->y;

  for(int i = 0 ; i <= this->width ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x ;
  yAcc = this->y + this->height;
  for(int i = 0 ; i <= this->width ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc++ ;
    yAcc-- ;
  }
}

//----- SHAPEL ------
shapeL::shapeL(int x, int y, int height, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = height / 2 ;
  this->height = height ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void shapeL::draw() {
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, this->color) ;
  }
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y + this->height, this->color) ;
  }
}

void shapeL::clearShape() {
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x, i, resample_rgb(db, 0xFFFFFF)) ;
  }
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
  }
}

//----- SHAPET -----
shapeT::shapeT(int x, int y, int height, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = height / 2 ;
  this->height = height ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void shapeT::draw() {
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, this->color) ;
  }
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x + this->width / 2, i, this->color) ;
  }
}

void shapeT::clearShape() {
  for(int i = this->x ; i <= this->x + this->width ; i++) {
    write_pixel(i, this->y, resample_rgb(db, 0xFFFFFF)) ;
  }
  for(int i = this->y ; i <= this->y + this->height ; i++) {
    write_pixel(this->x + this->width / 2, i, resample_rgb(db, 0xFFFFFF)) ;
  }
}

//----- SHAPEV ------
shapeV::shapeV(int x, int y, int width, short color, int direcx, int direcy, int type) {
  this->x = x ;
  this->y = y ;
  this->width = width ;
  this->height = width / 2 ;
  this->color = color ;
  this->direcx = direcx ;
  this->direcy = direcy ;
  this->type = type ;
}

void shapeV::draw() {
  int xAcc = this->x ;
  int yAcc = this->y ;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x + this->width ;
  yAcc = this->y ;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, this->color) ;
    xAcc-- ;
    yAcc++ ;
  }
}

void shapeV::clearShape() {
  int xAcc = this->x ;
  int yAcc = this->y ;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc++ ;
    yAcc++ ;
  }

  xAcc = this->x + this->width ;
  yAcc = this->y ;
  for(int i = 0 ; i <= this->width / 2 ; i++) {
    write_pixel(xAcc, yAcc, resample_rgb(db, 0xFFFFFF)) ;
    xAcc-- ;
    yAcc++ ;
  }
}

//----- SHAPE CHANGER -----
Shape * shapeChanger::changeShape(Shape *shape) {
  shape->clearShape() ;

  Shape *output = shape ;

  if(shape->type == 1) { output = new Cross(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 2) ; }
  else if (shape->type == 2) { output = new Circle(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 3) ; }
  else if (shape->type == 3) { output = new Triangle(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 4) ; }
  else if (shape->type == 4) { output = new Rectangle(shape->x, shape->y, shape->width, shape->height, shape->color, shape->direcx, shape->direcy, 5) ; }
  else if (shape->type == 5) { output = new Star(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 6) ; }
  else if (shape->type == 6) { output = new shapeX(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 7) ; }
  else if (shape->type == 7) { output = new shapeL(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 8) ; }
  else if (shape->type == 8) { output = new shapeT(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 9) ; }
  else if (shape->type == 9) { output = new shapeV(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 10) ; }
  else if (shape->type == 10) { output = new Square(shape->x, shape->y, shape->width, shape->color, shape->direcx, shape->direcy, 1) ; }

  return output ;
}
