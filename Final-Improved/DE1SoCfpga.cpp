#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include "DE1SoCfpga.h"
using namespace std;

DE1SoCfpga::DE1SoCfpga() {

      // Open /dev/mem to give access to physical addresses
    	fd = open( "/dev/mem", (O_RDWR | O_SYNC));
	    if (fd == -1)
      {			//  check for errors in openning /dev/mem
        cout << "ERROR: could not open /dev/mem..." << endl;
        exit(1);
    	}

     cout << fd << endl ;

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

DE1SoCfpga::~DE1SoCfpga() {

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

       //clearing the screen
      clear_screen() ;

      close (fd); 	// close memory
}

void DE1SoCfpga::RegisterWrite(unsigned int reg_offset, int value) {
	  * (volatile unsigned int *)(pBase + reg_offset) = value;
}

int DE1SoCfpga::RegisterRead(unsigned int reg_offset) {
  	return * (volatile unsigned int *)(pBase + reg_offset);
}

// ------------------------- VGA ------------------------------------

void DE1SoCfpga::write_pixel(int x, int y, short int color) {
  * (volatile short int *)(pBase_Pixel + (y << 10) + (x << 1)) = color ;
}

void DE1SoCfpga::write_char(int x, int y, char c) {
  * (char *) (pBase_Char + (y<<7) + x) = c ;
}

void DE1SoCfpga::video_box(int x1, int y1, int x2, int y2, short int color) {
  int pixel_ptr, row, col;

    /* assume that the box coordinates are valid */
    for (row = y1; row <= y2; row++)
      for (col = x1; col <= x2; ++col)
      {
        * (volatile short int *)(pBase_Pixel + (row << 10) + (col << 1)) = color ;
      }
}

void DE1SoCfpga::clear_screen() {
  video_box(0, 0, 319, 239, 0) ;
}

void DE1SoCfpga::video_text(int x, int y, char * text_ptr) {
  /* assume that the text string fits on one line */
  while (*(text_ptr)) {
      write_char(x,y,*text_ptr);
      ++text_ptr;
      ++x;
  }
}

int DE1SoCfpga::resample_rgb(int num_bits, int color) {

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

int DE1SoCfpga::get_data_bits(int mode) {
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

// ----------------- KEYPAD ---------------------

void DE1SoCfpga::init_keypad() {

    unsigned int base_direc = 15 ;
    RegisterWrite(JP1_Direction_Register, 0b111100000) ;  //row input column output
  }

char DE1SoCfpga::get_key() {

    RegisterWrite(JP1_Data_Register, 0b011111110) ;

    if (RegisterRead(JP1_Data_Register) == -137 < 1) {
      return '1' ;
    } else if (RegisterRead(JP1_Data_Register) == -133 < 1) {
      return '2' ;
    } else if (RegisterRead(JP1_Data_Register) == -131 < 1) {
      return '3' ;
    } else if (RegisterRead(JP1_Data_Register) == -130 < 1) {
      return 'A' ;
    } else {
      return ' ' ;
    }
}

// ------ AUDIO ---------
int DE1SoCfpga::Get_Count(double Hz) {
  return (200000000/Hz) / 2 ;
}

void DE1SoCfpga::playNote() {
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
}
