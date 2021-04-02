#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include <cmath>
#include <cstdio>
#include <ctime>
#include "address_map_arm.h"
using namespace std;

#define STANDARD_X 320
#define STANDARD_Y 240
#define INTEL_BLUE 0x0071C5
#define RGB_RESAMPLER_BASE 0xFF203010

const unsigned int MPCORE_PRIV_TIMER_LOAD_OFFSET = 0xFFFEC600 - LW_BRIDGE_BASE ;
const unsigned int MPCORE_PRIV_TIMER_COUNTER_OFFSET = 0xFFFEC604 - LW_BRIDGE_BASE ;
const unsigned int MPCORE_PRIV_TIMER_CONTROL_OFFSET = 0xFFFEC608 - LW_BRIDGE_BASE ;
const unsigned int MPCORE_PRIV_TIMER_INTERRUPT_OFFSET = 0xFFFEC60C - LW_BRIDGE_BASE ;

const unsigned int JP1_Data_Register = 0xFF200060 - LW_BRIDGE_BASE;
const unsigned int JP1_Direction_Register = 0xFF200064 - LW_BRIDGE_BASE;

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
    RegisterWrite(JP1_Data_Register, 0b01111111) ;
  }
  
  char get_key() {
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

class Shape {
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
    virtual Shape * move() {}
    virtual void clearShape() {}
     
    bool isCollided(Shape *b) {
      return ((this->x + this->width) >= b->x) &&
             (this->x <= (b->x + b->width)) &&
             ((this->y + this->height) >= b->y) &&
             (this->y <= (b->y + b->height)) ;
    }
    
    void bounce(Shape *b) {
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
    
    void edgeCheck() {
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

} ;

class Square : public Shape, public DE1SoCfpga {
  public:
    
    Square(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      video_box(this->x, this->y, this->x + this->width, this->y + this->width, this->color) ;
    }
    
    void clearShape() {
      video_box(this->x, this->y, this->x + this->width, this->y + this->width, resample_rgb(db, 0xFFFFFF)) ;
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
    
} ;

class Cross : public Shape, public DE1SoCfpga {
  public:
    Cross(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      video_box(this->x + this->width / 4, this->y, this->x + this->width - (this->width / 4), this->y + this->height, this->color) ;
      video_box(this->x, this->y + this->height / 4, this->x + this->width, this->y + this->height - (this->height / 4), this->color) ;
    }
    
    void clearShape() {
      video_box(this->x + this->width / 4, this->y, this->x + this->width - (this->width / 4), this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
      video_box(this->x, this->y + this->height / 4, this->x + this->width, this->y + this->height - (this->height / 4), resample_rgb(db, 0xFFFFFF)) ;
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class Circle : public Shape, public DE1SoCfpga {
  public:
    Circle(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
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
    
    void clearShape() {
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
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class Triangle : public Shape, public DE1SoCfpga {
  public:
    Triangle(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      for(int i = this->y; i <= this->y + this->height; ++i)
      {
        for(int j = this->x ; j <= i; ++j)
        {
            write_pixel(j, i, this->color) ;
        }
      }
    }
    
    void clearShape() {
      for(int i = this->y; i <= this->y + this->height; ++i)
      {
        for(int j = this->x ; j <= i; ++j)
        {
            write_pixel(j, i, resample_rgb(db, 0xFFFFFF)) ;
        }
      }
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class Rectangle : public Shape, public DE1SoCfpga {
  public:
    Rectangle(int x, int y, int width, int height, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = height ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      video_box(this->x, this->y, this->x + this->width, this->y + this->height, this->color) ;
    }
    
    void clearShape() {
      video_box(this->x, this->y, this->x + this->width, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class Star : public Shape, public DE1SoCfpga {
  public:
    Star(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
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
    
    void clearShape() {
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
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class shapeX : public Shape, public DE1SoCfpga {
  public:
    shapeX(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
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
    
    void clearShape() {
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
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class shapeL : public Shape, public DE1SoCfpga {
  public:
    shapeL(int x, int y, int height, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = height / 2 ;
      this->height = height ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      for(int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x, i, this->color) ;
      }
      for(int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y + this->height, this->color) ;
      }
    }
    
    void clearShape() {
      for(int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x, i, resample_rgb(db, 0xFFFFFF)) ;
      }
      for(int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
      }
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class shapeT : public Shape, public DE1SoCfpga {
  public:
    shapeT(int x, int y, int height, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = height / 2 ;
      this->height = height ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      for(int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y, this->color) ;
      }
      for(int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x + this->width / 2, i, this->color) ;
      }
    }
    
    void clearShape() {
      for(int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y, resample_rgb(db, 0xFFFFFF)) ;
      }
      for(int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x + this->width / 2, i, resample_rgb(db, 0xFFFFFF)) ;
      }
    }
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class shapeV : public Shape, public DE1SoCfpga {
  public:
    shapeV(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width / 2 ;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
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
    
    void clearShape() {
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
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
} ;

class shapeChanger {
  public:
  
    Shape * changeShape(Shape *shape) {
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
} ;


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
    virtual int collisionCount() {}

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
    }
    
    void checkCollisions() { 
      this->checkCollisionsHelp(this->shape) ;
      this->next->checkCollisions() ;
    }
    
    void checkCollisionsHelp(Shape *shape) {
        for(int i = this->size() - 1 ; i > 0 ; i--) {
          if (shape->isCollided(this->get(i))) { shape->bounce(this->get(i)) ; }
        }
    }
    
    int size() {
      return 1 + this->next->size() ;
    }
    
    Shape * get(int index) {
      if (this->size() == index) {
        return this->shape ;
      } else {
        this->next->get(index) ;
      }
    }
    
    loShapes * placeShape(Shape *shape) {
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
    
    bool placeHelp(Shape *shape) {
      bool flag = false ;
      for (int i = this->size() ; i > 0 ; i--) {
        flag = flag || shape->isCollided(this->get(i)) ;
      }
      return flag ;
    }
    
    int collisionCount() {
      int output = 0;
      for(int i = this->size() - 1 ; i > 0 ; i--) {
        output += this->shape->isCollided(this->get(i)) ? 1 : 0 ;
      }
      
      return output + this->next->collisionCount();
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
    
    int size() {
      return 0 ;
    }
    
    Shape * get(int index) {
      Shape *empty = new Square(0,0,0,0,0,0,1) ;
      return empty ;
    }
    
    loShapes * placeShape(Shape *shape) {
      return this->addShape(shape) ;
    }
    
    bool placeHelp(Shape *shape) {
      return false ;
    }
    
    int collisionCount() {
      return 0 ;
    }

} ;


int main() {

  // -------------- SETUP -------------
  DE1SoCfpga *DE1 = new DE1SoCfpga ;
  short background_color = DE1->resample_rgb(16, 0xFFFFFF);
  DE1->clear_screen() ;
  DE1->video_box(10, 10, 310, 230, background_color) ;
  
  // ---------------- MAIN SECTION ----------------------
  
  //SETUP
  int collisions = 0 ;
  double elapsedtime;
  
  loShapes *lst = new loMtShapes() ;
  Shape *sqr = new Square(10,10,30,0,1,1,1) ;
  Shape *crss = new Cross(50,10,30,0,1,1,2) ;
  Shape *crcl = new Cross(90,10,30,0,1,1,3) ;
  Shape *trg = new Triangle(130,10,30,0,1,1,4) ;
  Shape *rect = new Rectangle(170,10,30,40,0,1,1,5) ;
  Shape *str = new Star(10,150,30,0,1,1,6) ;
  Shape *x = new shapeX(50,150,30,0,1,1,7) ;
  Shape *l = new shapeL(90,150,30,0,1,1,8) ;
  Shape *t = new shapeT(130,150,30,0,1,1,9) ;
  Shape *v = new shapeV(170,150,30,0,1,1,10) ;
  
  lst = lst->addShape(sqr) ;
  lst = lst->addShape(crss) ;
  lst = lst->addShape(crcl) ;
  lst = lst->addShape(trg) ;
  lst = lst->addShape(str) ;
  lst = lst->addShape(x) ;
  lst = lst->addShape(l) ;
  lst = lst->addShape(t) ;
  lst = lst->addShape(v) ;
  
  //CLOCK
  clock_t start1 = clock();
  
  while(collisions < 1000) {
    lst = lst->moveShapes() ;
    lst->checkCollisions() ;
    lst->drawShapes() ;
    collisions += lst->collisionCount() ;
  }
  
  clock_t end1 = clock();
  elapsedtime = double(end1 - start1)/CLOCKS_PER_SEC;
  
  cout << "1,000 : " << elapsedtime << endl ;
  
  clock_t start2 = clock();
  
  while(collisions < 10000) {
    lst = lst->moveShapes() ;
    lst->checkCollisions() ;
    lst->drawShapes() ;
    collisions += lst->collisionCount() ;
  }
  
  clock_t end2 = clock();
  elapsedtime = double(end2 - start2)/CLOCKS_PER_SEC;
  
  cout << "10,000 : " << elapsedtime << endl ;
  
  clock_t start3 = clock();
  
  while(collisions < 100000) {
    lst = lst->moveShapes() ;
    lst->checkCollisions() ;
    lst->drawShapes() ;
    collisions += lst->collisionCount() ;
  }
  
  clock_t end3 = clock();
  elapsedtime = double(end3 - start3)/CLOCKS_PER_SEC;
  
  cout << "100,000 : " << elapsedtime << endl ;
  
  // -------------- CLOSING -----------------------
  DE1->clear_screen() ;
  delete DE1 ;
  
}








