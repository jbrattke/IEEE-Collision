#ifndef VGA_H
#define VGA_H
#include <string>
#include "address_map_arm.h"
#include "DE1SoCfpga.h"

class VGA : public DE1SoCfpga {
  public:
    int res_offset;
    int col_offset;
    int screen_x;
    int screen_y;
    int video_resolution;
    int db;
    int rgb_status;
    
    VGA() ;
    ~VGA() ;

    void video_box(int x1, int y1, int x2, int y2, short int color) ;
    void clear_screen() ;
    void video_text(int x, int y, char * text_ptr) ;
    int resample_rgb(int num_bits, int color) ;
    int get_data_bits(int mode) ;
} ;

#endif