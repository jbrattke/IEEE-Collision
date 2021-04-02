#ifndef SevenSegment_H
#define SevenSegment_H
#include <string>
#include "DE1SoCfpga.h"
#include "address_map_arm.h"
using namespace std;

const unsigned int bit_values[17] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113, 111};

class SevenSegment : public DE1SoCfpga {

  public:
    unsigned int reg0_hexValue ;
    unsigned int reg1_hexValue ;
    unsigned int initialvalueLoadMPCore ;
    unsigned int initialvalueControlMPCore ;
    unsigned int initialvalueInterruptMPCore ;
        
    SevenSegment() ;
    ~SevenSegment() ;
    void Hex_AllOn() ;
    void Hex_ClearAll() ;
    void Hex_ClearSpecific(int index) ;
    void Hex_WriteSpecific(int index, int value) ;
    void Hex_WriteNumber(int number) ;
    
    int Get_Count(double Hz) ;
    int ReturnNote(string note) ;
    
};

#endif