#ifndef DE1SoCfpga_H
#define DE1SoCfpga_H
#include <string>

//include all the addresses
#include "address_map_arm.h"

#define RGB_RESAMPLER_BASE 0xFF203010
const unsigned int MPCORE_PRIV_TIMER_LOAD_OFFSET = 0xDEC600 ;
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
    
    DE1SoCfpga();
    ~DE1SoCfpga();
     
    void RegisterWrite(unsigned int reg_offset, int value);
    int RegisterRead(unsigned int reg_offset);
    
    void write_pixel(int x, int y, short int color) ;
    void write_char(int x, int y, char c) ;
    
    void init_keypad() ;
    char get_key() ;

};

#endif