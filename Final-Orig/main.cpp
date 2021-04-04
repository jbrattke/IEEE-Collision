#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "address_map_arm.h"
#include "DE1SoCfpga.h"
#include "Shapes.h"
#include "loShapes.h"
using namespace std;

int main() {

  // -------------- SETUP -------------
  DE1SoCfpga *DE1 = new DE1SoCfpga ;
  DE1->clear_screen() ;
  DE1->video_box(10, 10, 310, 230, background_color) ;
  DE1->init_keypad() ;

  //WORLD CONSTANTS SETUP
  int exit = DE1->RegisterRead(SW_BASE) ;
  short background_color = DE1->resample_rgb(16, 0xFFFFFF);
  char last ;
  char key ;
  loShapes *lst = new loMtShapes() ;
  Shape *temp = new Square(0,0,0,background_color, 0, 0, 1) ;
  bool flag = false ;
  shapeChanger *change = new shapeChanger ;

// ---------------- MAIN SECTION ---------------------

  while(exit != 1) {
    exit = DE1->RegisterRead(SW_BASE) ;

    // THIS DEALS WITH MOVING, COLLIDING, AND DRAWING SHAPES
    lst = lst->moveShapes() ;
    lst->checkCollisions() ;
    lst->drawShapes() ;

    //KEYPAD
    key = DE1->get_key() ;

    if(key == '1' && key != last) {
      temp = flag ? change->changeShape(temp) : new Square(150, 110, 20, 0, 1, 1, 1) ;
      flag = true ;
    } else if (key == '2' && key != last) {
      temp->clearShape() ;
      temp->width -= 10 ;
      temp->height -= 10 ;
    } else if (key == '3' && key != last) {
      temp->clearShape() ;
      temp->width += 10 ;
      temp->height += 10 ;
    } else if (key == 'A' && key != last) {
      lst = lst->placeShape(temp) ;
      temp = empty ;
      flag = false ;
    }

    temp->draw() ;
    last = key ;

  }

  // ----- CLOSING -----

  DE1->clear_screen() ;
  delete DE1 ;
}
