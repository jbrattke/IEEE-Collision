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
  short background_color = DE1->resample_rgb(16, 0xFFFFFF);
  DE1->clear_screen() ;
  DE1->video_box(10, 10, 310, 230, background_color) ;
  
  short blue = DE1->resample_rgb(DE1->db, 0x0000FF) ;
  short green = DE1->resample_rgb(DE1->db, 0x00FF00) ;
  short red = DE1->resample_rgb(DE1->db, 	0xFF0000) ;
  
  Shape *empty = new Square(0,0,0,background_color, 0, 0, 1) ;
  Shape *b1 = new Square(100, 100, 20, blue, 1, 1, 1) ;
  Shape *shape2 = new Square(200, 100, 50, blue, -1, 1, 1) ;
  Shape *shape3 = new Square(50, 50, 50, red, 1, -1, 1) ;
  
  loShapes *mtList = new loMtShapes() ;
  
  loShapes *lo1 = new loConsShapes(shape2, mtList) ;
  
  loShapes *lo2 = new loConsShapes(b1, lo1) ;
  
  //Shape *test = lo2->get(2) ;
  //cout << test->width << endl ;
  
  loShapes *shapeList = mtList ;
  
// ---------------- MAIN SECTION ----------------------
  
  //shapeList->addShape(b1) ;
  
  //shapeList->addShape(shape2) ;
  
  //shapeList->addShape(shape3) ;
  
  int exit = DE1->RegisterRead(SW_BASE) ;
  int count = 0 ;
  int idCount = 0 ;
  int keypadCount = 0 ;
  char last ;
  char key ;
  int button = DE1->RegisterRead(KEY_BASE) ;
  
  loShapes *lst = new loMtShapes() ;
  
  Shape *temp = new Square(0,0,0,background_color, 0, 0, 1) ;
  
  int dataRead = DE1->RegisterRead(JP1_Data_Register) ;
  
  bool flag = false ;
  //shapeChanger *change = new shapeChanger ;
  
  DE1->init_keypad() ;
  
  while(exit != 1) {
    dataRead = DE1->RegisterRead(JP1_Data_Register) ;
    button = DE1->RegisterRead(KEY_BASE) ;
    exit = DE1->RegisterRead(SW_BASE) ;
    
    if(button == 1 && count != 1) {
      count = 1 ;
      Shape *shapeNew = new Square(0, 0, 50, 0, 1, 1, 1) ;
      lst = lst->addShape(shapeNew) ;
      idCount += 2 ;
    } else if (button == 2 && count != 2) {
      count = 2;
      Shape *shapeNew = new Square(0, 0, 50, 0, 1, 1, 1) ;
      lst = lst->addShape(shapeNew) ;
      idCount += 2 ;
    }
    
    
    if(exit != 4) {
      lst = lst->moveShapes() ;
    }
    
    lst->checkCollisions() ;
    lst->drawShapes() ;
    
    if(exit == 2) {
      lst = mtList ;
    }
    
    key = DE1->get_key() ;
    
    if(key == '1' && key != last) {
      //temp = flag ? change->changeShape(temp) : new Square(150, 110, 20, 0, 1, 1, 1) ;
      temp = new Square(150, 110, 20, 0, 1, 1, 1) ;
      //flag = true ;
    } else if (key == '2' && key != last) {
      temp->width -= 10 ;
      temp->height -= 10 ;
    } else if (key == '3' && key != last) {
      temp->width += 10 ;
      temp->height += 10 ;
    } else if (key == 'A' && key != last) {
      lst = lst->addShape(temp) ;
      temp = empty ;
      flag = false ;
    }
    
    temp->draw() ;
    last = key ;
    
    //shape3->move() ;
    
    //shape3->draw() ;
    //shape3->isCollided(shape2) ;  
    
  }
  
  //cin >> input ;
  
  DE1->clear_screen() ;
  delete DE1 ;
}

















