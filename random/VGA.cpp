#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include "VGA.h"
#include "address_map_arm.h"

using namespace std;

VGA::VGA() {

    //-- CONSTANTS ------
    video_resolution = RegisterRead(PIXEL_BUF_CTRL_BASE + 0x8)  ;
    screen_x = video_resolution & 0xFFFF;
    screen_y = (video_resolution >> 16) & 0xFFFF;
      
    rgb_status = RegisterRead(RGB_RESAMPLER_BASE - LW_BRIDGE_BASE);
    db = get_data_bits(rgb_status & 0x3F);
    
}

VGA::~VGA() {
    //clearing the screen
    clear_screen() ;
}

void VGA::video_box(int x1, int y1, int x2, int y2, short int color) { 
  int pixel_ptr, row, col;
  
    /* assume that the box coordinates are valid */
    for (row = y1; row <= y2; row++)
      for (col = x1; col <= x2; ++col) 
      {
        * (volatile short int *)(pBase_Pixel + (row << 10) + (col << 1)) = color ;
      }
}

void VGA::clear_screen() {
  video_box(0, 0, 319, 239, 0) ; 
}

void VGA::video_text(int x, int y, char * text_ptr) {
  /* assume that the text string fits on one line */
  while (*(text_ptr)) {
      write_char(x,y,*text_ptr);
      ++text_ptr;
      ++x;
  } 
}

int VGA::resample_rgb(int num_bits, int color) {

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

int VGA::get_data_bits(int mode) {
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



