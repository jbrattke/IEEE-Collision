#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "DE1SoCfpga.h"
#include "Shapes.h"
#include "loShapes.h"
using namespace std;

// ---------------------- LIST OF SHAPES METHODS --------------------------------

//CONS CLASS
loConsShapes::loConsShapes(Shape *shape, loShapes *next) {
  this->shape = shape ;
  this->next = next ;
}

void loConsShapes::drawShapes() {
  this->shape->draw() ;
  this->next->drawShapes() ;
}

loShapes * loConsShapes::addShape(Shape *shape) {
  loShapes *output = new loConsShapes(shape, this) ;
  return output ;
}

loShapes * loConsShapes::moveShapes() {
  loShapes *output = new loConsShapes(this->shape->move(), this->next->moveShapes()) ;
  return output ;
}

void loConsShapes::checkCollisions() {
  this->checkCollisionsHelp(this->shape) ;
  this->next->checkCollisions() ;
}

void loConsShapes::checkCollisionsHelp(Shape *shape) {
    for(int i = this->size() - 1 ; i > 0 ; i--) {
      if (shape->isCollided(this->get(i))) { shape->bounce(this->get(i)) ; }
    }
}

int loConsShapes::size() {
  return 1 + this->next->size() ;
}

Shape * loConsShapes::get(int index) {
  if (this->size() == index) {
    return this->shape ;
  } else {
    this->next->get(index) ;
  }
}

loShapes * loConsShapes::placeShape(Shape *shape) {
  loShapes *output = this ;
  bool exit = false ;
  int xAcc = 0 ;
  int yAcc = 0 ;

  while(!exit) {
    shape->x = xAcc ;
    shape->y = yAcc ;

    if (!this->placeHelp(shape)) {
      output = this->addShape(shape) ;
      exit = true ;
    }
    xAcc++ ;
    yAcc++ ;
  }

  return output ;
}

bool loConsShapes::placeHelp(Shape *shape) {
  bool flag = false ;
  for (int i = this->size() ; i > 0 ; i--) {
    flag = flag || shape->isCollided(this->get(i)) ;
  }
  return flag ;
}

//MT CLASS
loMtShapes::loMtShapes() {}

void loMtShapes::drawShapes() {}

loShapes * loMtShapes::addShape(Shape *shape) {
  loShapes *output = new loConsShapes(shape, this) ;
  return output ;
}

void loMtShapes::checkCollisions() {}

void loMtShapes::checkCollisionsHelp(Shape *shape) {}

loShapes * loMtShapes::moveShapes() {
  return this ;
}

int loMtShapes::size() {
  return 0 ;
}

Shape * loMtShapes::get(int index) {
  Shape *empty = new Square(0,0,0,0,0,0,1) ;
  return empty ;
}

loShapes * loMtShapes::placeShape(Shape *shape) {
  return this->addShape(shape) ;
}

bool loMtShapes::placeHelp(Shape *shape) {
  return false ;
}
