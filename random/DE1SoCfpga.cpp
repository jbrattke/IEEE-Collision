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
      
      close (fd); 	// close memory
}
  
void DE1SoCfpga::RegisterWrite(unsigned int reg_offset, int value) { 
	  * (volatile unsigned int *)(pBase + reg_offset) = value; 
}

int DE1SoCfpga::RegisterRead(unsigned int reg_offset) { 
  	return * (volatile unsigned int *)(pBase + reg_offset); 
}

void DE1SoCfpga::write_pixel(int x, int y, short int color) {
  * (volatile short int *)(pBase_Pixel + (y << 10) + (x << 1)) = color ;
}

void DE1SoCfpga::write_char(int x, int y, char c) {
  * (char *) (pBase_Char + (y<<7) + x) = c ;
}


void DE1SoCfpga::init_keypad() {
    
    unsigned int base_direc = 15 ;
    RegisterWrite(JP1_Direction_Register, 0b11110000) ;  //row input column output
  }
  
char DE1SoCfpga::get_key() {

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


