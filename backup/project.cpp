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
    unsigned int initialvalueLoadMPCore ;
    unsigned int initialvalueControlMPCore ;
    unsigned int initialvalueInterruptMPCore ;
    
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
      
      //-- VIDEO CONSTANTS ------
      video_resolution = RegisterRead(PIXEL_BUF_CTRL_BASE + 0x8)  ;
      screen_x = video_resolution & 0xFFFF;
      screen_y = (video_resolution >> 16) & 0xFFFF;
      
      rgb_status = RegisterRead(RGB_RESAMPLER_BASE - LW_BRIDGE_BASE);
      db = get_data_bits(rgb_status & 0x3F);
      
      // -- AUDIO ----
      initialvalueLoadMPCore = RegisterRead(MPCORE_PRIV_TIMER_LOAD_OFFSET);
      initialvalueControlMPCore = RegisterRead(MPCORE_PRIV_TIMER_CONTROL_OFFSET);
      initialvalueInterruptMPCore = RegisterRead(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET);
      RegisterWrite(JP1_Direction_Register, 1) ;
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
         
      RegisterWrite(MPCORE_PRIV_TIMER_LOAD_OFFSET, initialvalueLoadMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_CONTROL_OFFSET, initialvalueControlMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET, initialvalueInterruptMPCore);
      
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
  
  int Get_Count(double Hz) {
    return (200000000/Hz) / 2 ;
  }
  
  int playNote() {
      double cycles = 100000000;
      int sum = 0 ;
      int switchtone = 0 ;
      int counter = Get_Count(1046.5) ;
      
      //int dataBase = RegisterRead(JP1_Data_Register) ;

      RegisterWrite(MPCORE_PRIV_TIMER_LOAD_OFFSET, counter) ;
      RegisterWrite(MPCORE_PRIV_TIMER_CONTROL_OFFSET, 3) ;
      
      while (sum < cycles)
      {
        if (RegisterRead(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET) != 0) 
        {
          sum = sum + counter ;
          RegisterWrite(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET, 1) ;
          // reset timer flag bit

          if (switchtone == 0)
          {
            RegisterWrite(JP1_Data_Register, 1);
            switchtone=1;
          }
          else
          {
            RegisterWrite(JP1_Data_Register, 0);
            switchtone=0;
          }
        }
      }
      
      return 0 ;
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
    
    Shape * move() {
      this->clearShape() ;
      this->edgeCheck() ;
      this->x = this->x + this->direcx ;
      this->y = this->y + this->direcy ;

      return this ;
    }
    
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
      for (int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y, this->color) ;
        write_pixel(i, this->y + this->height, this->color) ;
      }
      for (int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x, i, this->color) ;
        write_pixel(this->x + this->width, i, this->color) ;
      }
    }
    
    void clearShape() {
      for (int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y, resample_rgb(db, 0xFFFFFF)) ;
        write_pixel(i, this->y + this->height, resample_rgb(db, 0xFFFFFF)) ;
      }
      for (int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x, i, resample_rgb(db, 0xFFFFFF)) ;
        write_pixel(this->x + this->width, i, resample_rgb(db, 0xFFFFFF)) ;
      }
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
    
    void clearShape() {
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
    /*
      static const double PI = 3.1415926535;
      double i, angle, xTemp, yTemp;
      
      for(i = 0; i < 360; i += 0.1)
      {
            angle = i;
            xTemp = this->width / 2 * cos(angle * PI / 180);
            yTemp = this->width / 2 * sin(angle * PI / 180);
            write_pixel(x + xTemp, y + yTemp, color);
      }
      */
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
    
    void clearShape() {
    /*
      static const double PI = 3.1415926535;
      double i, angle, xTemp, yTemp;
      
      for(i = 0; i < 360; i += 0.1)
      {
            angle = i;
            xTemp = this->width / 2 * cos(angle * PI / 180);
            yTemp = this->width / 2 * sin(angle * PI / 180);
            write_pixel(x + xTemp, y + yTemp, resample_rgb(db, 0xFFFFFF));
      }
      */
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
} ;

class Triangle : public Shape, public DE1SoCfpga {
  public:
    Triangle(int x, int y, int width, short color, int direcx, int direcy, int type) {
      this->x = x ;
      this->y = y ;
      this->width = width ;
      this->height = width / 2;
      this->color = color ;
      this->direcx = direcx ;
      this->direcy = direcy ;
      this->type = type ;
    }
    
    void draw() {
      int xAcc = this->x ;
      int yAcc = this->y + this->height ;
      for (int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, yAcc, this->color) ;
        write_pixel(i, this->y + this->height, this->color) ;
        yAcc += (i <= this->x + this->width / 2) ?  -1 : 1 ;
      }
    }
    
    void clearShape() {
      short int color = resample_rgb(db, 0xFFFFFF) ;
      int xAcc = this->x ;
      int yAcc = this->y + this->height ;
      for (int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, yAcc, color) ;
        write_pixel(i, this->y + this->height, color) ;
        yAcc += (i <= this->x + this->width / 2) ?  -1 : 1 ;
      }
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
      for (int i = this->x ; i <= this->x + this->width ; i++) {
        write_pixel(i, this->y, this->color) ;
        write_pixel(i, this->y + this->height, this->color) ;
      }
      for (int i = this->y ; i <= this->y + this->height ; i++) {
        write_pixel(this->x, i, this->color) ;
        write_pixel(this->x + this->width, i, this->color) ;
      }
    }
    
    void clearShape() {
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

} ;


int main() {
  //--------- CLOCK ----------
  double elapstedTime ;
  clock_t start = clock() ;

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
  
  DE1->playNote() ;
// ---------------- MAIN SECTION ----------------------
  
  //shapeList->addShape(b1) ;
  
  //shapeList->addShape(shape2) ;
  
  //shapeList->addShape(shape3) ;
  
  int exit = DE1->RegisterRead(SW_BASE) ;
  int count = 0 ;
  int idCount = 0 ;
  int keypadCount = 0 ;
  char last = ' ' ;
  char key = ' ' ;
  int button = DE1->RegisterRead(KEY_BASE) ;
  
  loShapes *lst = new loMtShapes() ;
  
  Shape *temp = new Square(0,0,0,background_color, 0, 0, 1) ;
  
  int dataRead = DE1->RegisterRead(JP1_Data_Register) ;
  
  bool flag = false ;
  shapeChanger *change = new shapeChanger ;
  
  //DE1->playNote() ;
  
  while(exit != 1) {
    dataRead = DE1->RegisterRead(JP1_Data_Register) ;
    button = DE1->RegisterRead(KEY_BASE) ;
    exit = DE1->RegisterRead(SW_BASE) ;
    
    /*
    DE1->init_keypad() ;
    key = DE1->get_key() ;
    
    if(key == '1' && key != last) {
      if (flag) {
        temp = change->changeShape(temp) ;
      } else {
        temp = new Square(150, 110, 30, 0, 1, 1, 1) ;
      }
      flag = true ;
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
    */
    
    if(button == 1 && count != 1) {
      count = 1 ;
      temp = flag ? change->changeShape(temp) : new Square(150, 110, 30, 0, 1, 1, 1) ;
      flag = true ;
    } else if (button == 2 && count != 2) {
      count = 2;
      temp->clearShape() ;
      temp->width -= 10 ;
      temp->height -= 10 ;
    } else if (button == 4 && count != 4) {
      count = 4 ;
      temp->clearShape() ;
      temp->width += 10 ;
      temp->height += 10 ;
    } else if (button == 8 && count != 8) {
      count = 8 ;
      lst = lst->placeShape(temp) ;
      temp = empty ;
      flag = false ;
    } else if (button == 0) {
      count = 0 ;
    }
    
    temp->draw() ;
    
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
    
  }
  
  //cin >> input ;
  
  DE1->clear_screen() ;
  delete DE1 ;
  
}








