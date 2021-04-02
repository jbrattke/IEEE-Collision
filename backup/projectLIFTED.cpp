#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "address_map_arm.h"
using namespace std;

#define STANDARD_X 320
#define STANDARD_Y 240
#define INTEL_BLUE 0x0071C5
#define RGB_RESAMPLER_BASE 0xFF203010

const unsigned int JP1_Data_Register = 0xFF200060 - LW_BRIDGE_BASE;
const unsigned int JP1_Direction_Register = 0xFF200064 - LW_BRIDGE_BASE;

//TO-DO LIST
// - make classes for all other shapes(draw functions too)
// - get keypad working(placing shapes, growing, etc)
//   - 1 requests shape, 2 shrinks, 3 grows, 4 places(HAVE TO MAKE PLACE FUNCTION)
// - get sound working on collisions - TUESDAY DONE??
// - CPU timing
// - Algorithm improvement --> using sphere AABB? TREE?

class DE1SoCfpga 
{
  public:
    char *pBase ;
    char *pBase_Pixel ;
    char *pBase_Char ;
    int fd ;
    int screen_x;
    int screen_y;
    int video_resolution;
    int db;
    int rgb_status;
    
    DE1SoCfpga() 
    {
    
      // Open /dev/mem to give access to physical addresses
    	fd = open( "/dev/mem", (O_RDWR | O_SYNC));
	    if (fd == -1) 
      {			//  check for errors in openning /dev/mem
        cout << "ERROR: could not open /dev/mem..." << endl;
        exit(1);
    	}
      
      // Get a mapping from physical addresses to virtual addresses
      char *virtual_base = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
      if (virtual_base == MAP_FAILED) 
      {		// check for errors
       cout << "ERROR: mmap1() failed..." << endl;
       close (fd);		// close memory before exiting
       exit(1);        // Returns 1 to the operating system;
      }
      pBase = virtual_base; 


	    // === get VGA char addr =====================
	    // get virtual addr that maps to physical
	    char *vga_char_virtual_base = (char *)mmap( NULL, FPGA_CHAR_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, FPGA_CHAR_BASE );	
	    if(vga_char_virtual_base == MAP_FAILED) 
      {
		    printf( "ERROR: mmap2() failed...\n" );
		    close(fd);
		    exit(1);
	    }
      // Get the address that maps to the FPGA LED control 
	    pBase_Char = vga_char_virtual_base ;


	    // === get VGA pixel addr ====================
  	  // get virtual addr that maps to physical
	    char *vga_pixel_virtual_base = (char *)mmap( NULL, FPGA_ONCHIP_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, FPGA_ONCHIP_BASE);	
	    if(vga_pixel_virtual_base == MAP_FAILED ) 
      {
	  	  printf( "ERROR: mmap3() failed...\n" );
	  	  close(fd);
	  	  exit(1);
	    }  
      // Get the address that maps to the FPGA LED control 
      pBase_Pixel = vga_pixel_virtual_base ;
      
      //-- CONSTANTS ------
      video_resolution = RegisterRead(PIXEL_BUF_CTRL_BASE + 0x8)  ;
      screen_x = video_resolution & 0xFFFF;
      screen_y = (video_resolution >> 16) & 0xFFFF;
      
      rgb_status = RegisterRead(RGB_RESAMPLER_BASE - LW_BRIDGE_BASE);
      db = get_data_bits(rgb_status & 0x3F);
    }
   
   ~DE1SoCfpga() 
   {
     	if (munmap (pBase, LW_BRIDGE_SPAN) != 0) 
      {
      cout << "ERROR: munmap() failed..." << endl;
      exit(1);
	    }
      
      if (munmap (pBase_Pixel, FPGA_ONCHIP_SPAN) != 0) 
      {
      cout << "ERROR: munmap() failed..." << endl;
      exit(1);
	    }
         
      if (munmap (pBase_Char, FPGA_CHAR_SPAN) != 0) 
      {
      cout << "ERROR: munmap() failed..." << endl;
      exit(1);
	    }
      
      close (fd); 	// close memory
   }
   
  void RegisterWrite(unsigned int reg_offset, int value) 
  { 
	  * (volatile unsigned int *)(pBase + reg_offset) = value; 
  } 

  int RegisterRead(unsigned int reg_offset) 
  { 
  	return * (volatile unsigned int *)(pBase + reg_offset); 
  } 

  void write_pixel(int x, int y, short int colour) 
  {
    * (volatile short int *)(pBase_Pixel + (y<<10) + (x<<1)) = colour ;
  }

  void video_box(int x1, int y1, int x2, int y2, short int pixel_color) 
  { 
    int pixel_ptr, row, col;
  
    /* assume that the box coordinates are valid */
    for (row = y1; row <= y2; row++)
      for (col = x1; col <= x2; ++col) 
      {
        * (volatile short int *)(pBase_Pixel + (row << 10) + (col << 1)) = pixel_color ;
      }
  }

  /* use write_pixel to set entire screen to black (does not clear the character buffer) */
  void clear_screen() 
  {
    video_box(0,0, 319, 239, 0) ;
  }

  /* write a single character to the character buffer at x,y
  * x in [0,79], y in [0,59]
  */
  void write_char(int x, int y, char c) 
  {
    * (char *) (pBase_Char + (y<<7) + x) = c ;
  }
  
  void video_text(int x, int y, char * text_ptr) 
  {
    /* assume that the text string fits on one line */
    while (*(text_ptr)) 
    {
      write_char(x,y,*text_ptr);
      ++text_ptr;
      ++x;
    } 
  }
  
  int resample_rgb(int num_bits, int color) 
  {
    if (num_bits == 8) {
      color = (((color >> 16) & 0x000000E0) | ((color >> 11) & 0x0000001C) |
               ((color >> 6) & 0x00000003));
      color = (color << 8) | color;
    } else if (num_bits == 16) {
      color = (((color >> 8) & 0x0000F800) | ((color >> 5) & 0x000007E0) |
              ((color >> 3) & 0x0000001F));
    }
    return color;
  }

  int get_data_bits(int mode) 
  {
    switch (mode) 
    {
      case 0x0:
        return 1;
      case 0x7:
        return 8;
      case 0x11:
        return 8;
      case 0x12:
        return 9;
      case 0x14:
        return 16;
      case 0x17:
        return 24;
      case 0x19:
        return 30;
      case 0x31:
        return 8;
      case 0x32:
        return 12;
      case 0x33:
        return 16;
      case 0x37:
        return 32;
      case 0x39:
        return 40;
    }
  }
  
  
  void init_keypad() {
    RegisterWrite(JP1_Direction_Register, 0b11110000) ;  //row input column output
  }
  
  char get_key() {
    RegisterWrite(JP1_Data_Register, 0b01111111) ;
    
    if (RegisterRead(JP1_Data_Register) == -137) {
      return '1' ;
    } else if (RegisterRead(JP1_Data_Register) == -133) {
      return '2' ;
    } else if (RegisterRead(JP1_Data_Register) == -131) {
      return '3' ;
    } else if (RegisterRead(JP1_Data_Register) == -130) {
      return 'A' ;
    } else {
      return ' ' ;
    }   
  }

} ;

class Shape : public DE1SoCfpga {
  public:
    int width ;
    int height ;
    int x ;
    int y ;
    int direcx ;
    int direcy ;
    short color ;
    
    virtual void draw() {}
            
    void direcChange(Shape *b) {
      if ((this->x == b->x + b->width && this->y == b->y + b->height) || (b->x == this->x + this->width && b->y == this->y + this->height) ||
          (this->y == b->y + b->height && b->x == this->x + this->width) || (b->y == this->y + this->height && this->x == b->x + b->width)) {
        this->direcx *= -1 ;
        b->direcx *= -1 ;
        this->direcy *= -1 ;
        b->direcy *= -1 ;
        
        //this->move() ;
        //b->move() ;
        
      } else if (this->y == b->y + b->height || b->y == this->y + this->height) {
        this->direcy *= -1 ;
        b->direcy *= -1 ;
        
        //this->move() ;
        //b->move() ;
        
      } else if (this->x + this->width == b->x || b->x + b->width == this->x) {
        this->direcx *= -1 ;
        b->direcx *= -1 ;
        
        //this->move() ;
        //b->move() ;
        
      }
    }
            
    Shape * move() {
      int newX, newY ;
      newX = this->x + this->direcx ;
      newY = this->y + this->direcy ;
      
      if(newX < 10) {
        this->direcx = 1 ;
      } else if (newX > (310 - this->width)) {
        this->direcx = -1 ;
      }
      
      if(newY < 10) {
        this->direcy = 1 ;
      } else if (newY > (230 - this->height)) {
        this->direcy = -1 ;
      }
      
      newX = this->x + this->direcx ;
      newY = this->y + this->direcy ;
      
      this->clearShape() ;
      
      this->x = newX ;
      this->y = newY ;
      
      return this ;
    }
            
    void clearShape() {
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
            
    void isCollided(Shape *b) {
      if(((this->x + this->width) >= b->x) &&
          (this->x <= (b->x + b->width)) &&
          ((this->y + this->height) >= b->y) &&
          (this->y <= (b->y + b->height))) 
          {
            this->direcChange(b) ;
            //cout << "collision" << endl ;
          }
    }
} ;

class Square : public Shape {
  public:
    
    Square(int x, int y, int width, short color, int direcx, int direcy) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
    }
    
    void draw() {    
      video_box(this->x, this->y, this->x + this->width, this->y + this->width, this->color) ;
    }
    
} ;

class loShapes {
  public:
    virtual void drawShapes() {}
    virtual loShapes * addShape(Shape *shape) {}
    virtual loShapes * moveShapes() {}
    virtual void checkCollisions() {}
    virtual void checkCollisionsHelp(Shape *shape) {}
    virtual loShapes * reverse() {}
    virtual loShapes * reverseHelp(Shape *acc) {}
    virtual int size() {}

} ;

class loConsShapes : public loShapes, public Shape {
  public:
    Shape *shape ;
    loShapes *next ;

    loConsShapes(Shape *shape, loShapes *next) {
      this->shape = shape ;
      this->next = next ;
    }
    
    void drawShapes() {
      this->shape->draw() ;
      this->next->drawShapes() ;
    }
    
    loShapes * addShape(Shape *shape) {
      loShapes *output = new loConsShapes(shape, this) ;
      return output ;
    }
    
    loShapes * moveShapes() {
      loShapes *output = new loConsShapes(this->shape->move(), this->next->moveShapes()) ;
      return output ;
      //this->shape->move() ;
      //this->next->moveShapes() ;
    }
    
        
    Shape * moveShape(Shape *shape) {
      //check collidedhelp
      //move shape with new direcx direcy
    }
    
    void checkCollisions() { 
      this->next->checkCollisionsHelp(this->shape) ;
      this->next->checkCollisions() ;
    }
    
    void checkCollisionsHelp(Shape *shape) {
        shape->isCollided(this->shape) ;
        this->next->checkCollisionsHelp(shape) ; 
    }
    
    loShapes * reverse() {
      return this->next->reverse()->reverseHelp(this->shape) ;
    }
    
    loShapes * reverseHelp(Shape *acc) {
      loShapes *output = new loConsShapes(this->shape, this->next->reverseHelp(acc)) ;
      return output ;
    }
    
    int size() {
      return 1 + this->next->size() ;
    }
    
} ;

class loMtShapes : public loShapes, public Shape {
  public:
    
    loMtShapes() {}
    
    void drawShapes() {}
    
    loShapes * addShape(Shape *shape) {
      loShapes *output = new loConsShapes(shape, this) ;
      return output ; 
    }
    
    void checkCollisions() {}
    
    void checkCollisionsHelp(Shape *shape) {}
    
    loShapes * moveShapes() {
      return this ;
    }
    
    Shape * moveShape(Shape *shape) {
      
    }
    
    loShapes * reverse() {
      return this ;
    }
    
    loShapes * reverseHelp(Shape *acc) {
      loShapes *output = new loConsShapes(acc, this) ;
      return output ;
    }
    
    int size() {
      return 0 ;
    }

} ;


int main() {

  // -------------- SETUP -------------
  DE1SoCfpga *DE1 = new DE1SoCfpga ;
  short background_color = DE1->resample_rgb(16, 0xFFFFFF);
  DE1->clear_screen() ;
  DE1->video_box(10, 10, 310, 230, background_color) ;
  
  short blue = DE1->resample_rgb(DE1->db, 0x0000FF) ;
  short green = DE1->resample_rgb(DE1->db, 0x00FF00) ;
  short red = DE1->resample_rgb(DE1->db, 	0xFF0000) ;
  
  Shape *empty = new Square(0,0,0,background_color, 0, 0) ;
  Shape *b1 = new Square(100, 100, 20, blue, 1, 1) ;
  Shape *shape2 = new Square(200, 100, 50, blue, -1, 1) ;
  Shape *shape3 = new Square(50, 50, 50, red, 1, -1) ;
  
  loShapes *mtList = new loMtShapes() ;
  
  loShapes *lo1 = new loConsShapes(shape2, mtList) ;
  
  loShapes *lo2 = new loConsShapes(b1, lo1) ;
  
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
  
  Shape *temp = empty ;
  
  while(exit != 1) {
    button = DE1->RegisterRead(KEY_BASE) ;
    exit = DE1->RegisterRead(SW_BASE) ;
    
    if(button == 1 && count != 1) {
      count = 1 ;
      Shape *shapeNew = new Square(0, 0, 30, 0, 1, 1) ;
      lst = lst->addShape(shapeNew) ;
      idCount += 2 ;
    } else if (button == 2 && count != 2) {
      count = 2;
      Shape *shapeNew = new Square(0, 0, 30, 0, 1, 1) ;
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
    
    
    //shape3->move() ;
    
    //shape3->draw() ;
    //shape3->isCollided(shape2) ;  
    
    DE1->init_keypad() ;
    key = DE1->get_key() ;
    
    if(key == '1' && key != last) {
      temp = new Square(150, 110, 20, 0, 1, 1) ; 
    } else if (key == '2' && key != last) {
      temp = new Square(150, 110, temp->width + 10, 0, 1, 1) ;
    } else if (key == '3' && key != last) {
      temp = new Square(150, 110, temp->width - 10, 0, 1, 1) ;
    } else if (key == 'A' && key != last) {
      lst = lst->addShape(temp) ;
      temp = empty ;
    }
    
    temp->draw() ;
    last = key ;
  }
  
  //cin >> input ;
  
  DE1->clear_screen() ;
}








