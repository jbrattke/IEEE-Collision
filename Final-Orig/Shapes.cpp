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
      video_box(this->x, this->y, this->x + this->width, this->y + this->width, this->color) ;
}

void Square::clearShape() {
      if(this->direcx == 1 && this->direcy == 1) {
        for (int i = this->x ; i <= this->x + this->width ; i++) {
          write_pixel(i, this->y, resample_rgb(16, 0xFFFFFF)) ;
        }
        for (int i = this->y ; i <= this->y + this->height ; i++) {
          write_pixel(this->x, i, resample_rgb(16, 0xFFFFFF)) ;
        }

      } else if (this->direcx == -1 && this->direcy == -1) {
        for (int i = this->x ; i <= this->x + this->width ; i++) {
          write_pixel(i, this->y + this->height, resample_rgb(16, 0xFFFFFF)) ;
        }
        for (int i = this->y ; i <= this->y + this->height ; i++) {
          write_pixel(this->x + this->width, i, resample_rgb(16, 0xFFFFFF)) ;
        }

      } else if (this->direcx == 1 && this->direcy == -1) {
        for (int i = this->x ; i <= this->x + this->width ; i++) {
          write_pixel(i, this->y + this->height, resample_rgb(16, 0xFFFFFF)) ;
        }
        for (int i = this->y ; i <= this->y + this->height ; i++) {
          write_pixel(this->x, i, resample_rgb(16, 0xFFFFFF)) ;
        }

      } else if (this->direcx == -1 && this->direcy == 1) {
        for (int i = this->x ; i <= this->x + this->width ; i++) {
          write_pixel(i, this->y, resample_rgb(16, 0xFFFFFF)) ;
        }
        for (int i = this->y ; i <= this->y + this->height ; i++) {
          write_pixel(this->x + this->width, i, resample_rgb(16, 0xFFFFFF)) ;
        }
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
  video_box(this->x + this->width / 4, this->y, this->x + this->width - (this->width / 4), this->y + this->height, this->color) ;
  video_box(this->x, this->y + this->height / 4, this->x + this->width, this->y + this->height - (this->height / 4), this->color) ;
}

void Cross::clearShape() {
  video_box(this->x + this->width / 4, this->y, this->x + this->width - (this->width / 4), this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
  video_box(this->x, this->y + this->height / 4, this->x + this->width, this->y + this->height - (this->height / 4), resample_rgb(db, 0xFFFFFF)) ;
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
  static const double PI = 3.1415926535;
  double i, angle, xTemp, yTemp;

  for(i = 0; i < 360; i += 0.1)
  {
        angle = i;
        xTemp = this->width / 2 * cos(angle * PI / 180);
        yTemp = this->width / 2 * sin(angle * PI / 180);
        write_pixel(x + xTemp, y + yTemp, color);
  }
}

void Circle::clearShape() {
  static const double PI = 3.1415926535;
  double i, angle, xTemp, yTemp;

  for(i = 0; i < 360; i += 0.1)
  {
        angle = i;
        xTemp = this->width / 2 * cos(angle * PI / 180);
        yTemp = this->width / 2 * sin(angle * PI / 180);
        write_pixel(x + xTemp, y + yTemp, resample_rgb(db, 0xFFFFFF));
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
  for(int i = this->y; i <= this->y + this->height; ++i)
  {
    for(int j = this->x ; j <= i; ++j)
    {
        write_pixel(j, i, this->color) ;
    }
  }
}

void Triangle::clearShape() {
  for(int i = this->y; i <= this->y + this->height; ++i)
  {
    for(int j = this->x ; j <= i; ++j)
    {
        write_pixel(j, i, resample_rgb(db, 0xFFFFFF)) ;
    }
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
  video_box(this->x, this->y, this->x + this->width, this->y + this->height, this->color) ;
}

void Rectangle::clearShape() {
  video_box(this->x, this->y, this->x + this->width, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
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
