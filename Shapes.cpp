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
      } else if (this->x > (310 - this->width)) {
        this->direcx = -1 ;
      }
      
      if(this->y < 10) {
        this->direcy = 1 ;
      } else if (this->y > (230 - this->height)) {
        this->direcy = -1 ;
      }
}

//-------------------------- INDIVIDUAL SHAPE CONSTRUCTORS AND DRAW FUNCTIONS

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
    
Shape * Square::move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
}


