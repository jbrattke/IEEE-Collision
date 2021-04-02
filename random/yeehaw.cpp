#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "address_map_arm.h"
using namespace std;
/* function prototypes */

void video_text(int, int, char *);
void video_box(int, int, int, int, short);

int resample_rgb(int, int);
int get_data_bits(int);

#define STANDARD_X 320
#define STANDARD_Y 240
#define INTEL_BLUE 0x0071C5
#define RGB_RESAMPLER_BASE 0xFF203010

class DE1SoCfpga 
{
  public:
    char *pBase ;
    char *pBase_Pixel ;
    char *pBase_Char ;
    int fd ;
    int res_offset;
    int col_offset;
    int screen_x;
    int screen_y;
    
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
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);
    x1 = x1 / x_factor;
    x2 = x2 / x_factor;
    y1 = y1 / y_factor;
    y2 = y2 / y_factor;
  
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

};

/* global variables */
int screen_x;
int screen_y;


/*******************************************************************************
* This program demonstrates use of the video in the computer system.
* Draws a blue box on the video display, and places a text string inside the
* box
******************************************************************************/
int main(void) 
{
  DE1SoCfpga *main = new DE1SoCfpga ;
  
  int rgb_status = main->RegisterRead(RGB_RESAMPLER_BASE - LW_BRIDGE_BASE);
  int db = main->get_data_bits(rgb_status & 0x3F);
  
  /* create a message to be displayed on the video and LCD displays */
  char text_top_row[40] = "PeePee";
  char text_bottom_row[40] = "PooPoo";
  
  /* update color */
  short background_color = main->resample_rgb(db, INTEL_BLUE);
  int exit = 0;
  
  short blue = main->resample_rgb(16, 0x0000FF) ;
  short green = main->resample_rgb(16, 0x00FF00) ;
  

  
  while(exit != 1) {
    exit = main->RegisterRead(SW_BASE) ;
  
    main->video_text(35, 29, text_top_row);
    main->video_text(32, 30, text_bottom_row);
    //main->video_box(0, 0, STANDARD_X, STANDARD_Y, 0) ; // clear the screen
    main->video_box(31 * 4, 28 * 4, 49 * 4 - 1, 32 * 4 - 1, background_color);
    main->video_box(20, 20, 300, 220, blue) ;
  }
  
  main->clear_screen() ;
  
}








