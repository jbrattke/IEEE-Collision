#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <string>
#include "SevenSegment.h"
using namespace std;

SevenSegment::SevenSegment(){
      reg0_hexValue = RegisterRead(HEX3_HEX0_BASE);
      reg1_hexValue = RegisterRead(HEX5_HEX4_BASE);
      initialvalueLoadMPCore = RegisterRead(MPCORE_PRIV_TIMER_LOAD_OFFSET);
      initialvalueControlMPCore = RegisterRead(MPCORE_PRIV_TIMER_CONTROL_OFFSET);
      initialvalueInterruptMPCore = RegisterRead(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET);
      //RegisterWrite(JP1_Direction_Register, 1) ;
}
    
SevenSegment::~SevenSegment(){
      RegisterWrite(MPCORE_PRIV_TIMER_LOAD_OFFSET, initialvalueLoadMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_CONTROL_OFFSET, initialvalueControlMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET, initialvalueInterruptMPCore);
      Hex_ClearAll();
}

void SevenSegment::Hex_AllOn() {
      RegisterWrite(HEX3_HEX0_BASE, 0xFFFFFFFF);
      RegisterWrite(HEX5_HEX4_BASE, 0xFFFF) ;
}
    
void SevenSegment::Hex_ClearAll() {
      RegisterWrite(HEX3_HEX0_BASE, 0) ;
      RegisterWrite(HEX5_HEX4_BASE, 0) ;
}
    
void SevenSegment::Hex_ClearSpecific(int index) {
      reg0_hexValue = RegisterRead(HEX3_HEX0_BASE);
      reg1_hexValue = RegisterRead(HEX5_HEX4_BASE);
      if(index == 0) {
        RegisterWrite(HEX3_HEX0_BASE, 0xFFFFFF00 & reg0_hexValue) ;
      } else if (index == 1) {
        RegisterWrite(HEX3_HEX0_BASE, 0xFFFF00FF & reg0_hexValue) ;
      } else if (index == 2) {
        RegisterWrite(HEX3_HEX0_BASE, 0xFF00FFFF & reg0_hexValue) ;
      } else if (index == 3) {
        RegisterWrite(HEX3_HEX0_BASE, 0x00FFFFFF & reg0_hexValue) ;
      } else if (index == 4) {
        RegisterWrite(HEX5_HEX4_BASE, 0xFF00 & reg1_hexValue) ;
      } else if (index == 5) {
        RegisterWrite(HEX5_HEX4_BASE, 0x00FF & reg1_hexValue) ;
      } else {
        cout << "Not a valid index number!" ;
      }
}
  
void SevenSegment::Hex_WriteSpecific(int index, int value) {
      reg0_hexValue = RegisterRead(HEX3_HEX0_BASE);
      reg1_hexValue = RegisterRead(HEX5_HEX4_BASE);
      unsigned int decValue = bit_values[value] ;
      
      if(index == 0) {
        RegisterWrite(HEX3_HEX0_BASE, (0xFFFFFF00 & reg0_hexValue) ^ decValue) ;
        
      } else if (index == 1) {
        RegisterWrite(HEX3_HEX0_BASE, (0xFFFF00FF & reg0_hexValue) ^ (decValue << 8)) ;
        
      } else if (index == 2) {
        RegisterWrite(HEX3_HEX0_BASE, (0xFF00FFFF & reg0_hexValue) ^ (decValue << 16)) ;
        
      } else if (index == 3) {
        RegisterWrite(HEX3_HEX0_BASE, (0x00FFFFFF & reg0_hexValue) ^ (decValue << 24)) ;
        
      } else if (index == 4) {
        RegisterWrite(HEX5_HEX4_BASE, (0xFF00 & reg1_hexValue) ^ decValue) ;
        
      } else if (index == 5) {
        RegisterWrite(HEX5_HEX4_BASE, (0x00FF & reg1_hexValue) ^ (decValue << 8)) ;
        
      } else if ((index > 5) ^ (value > 15) ^ (value < 0)) {
        cout << "Not a valid index number!" ;
      }
}
  
void SevenSegment::Hex_WriteNumber(int number) {
      reg0_hexValue = RegisterRead(HEX3_HEX0_BASE);
      reg1_hexValue = RegisterRead(HEX5_HEX4_BASE);
      int value=0;
      for (int i=5; i>=0; i--) 
      {
        if(i==5)
        {
          value=number&0xF00000;
          value=value>>20;
          Hex_WriteSpecific(5, value);
        }
        else if(i==4)
        {
          value=number&0x0F0000;
          value=value>>16;
          Hex_WriteSpecific(4, value);
        }
        else if(i==3)
        {
          value=number&0x00F000;
          value=value>>12;
          Hex_WriteSpecific(3, value);
        }
        else if(i==2)
        {
          value=number&0x000F00;
          value=value>>8;
          Hex_WriteSpecific(2, value);
        }
        else if(i==1)
        {
          value=number&0x0000F0;
          value=value>>4;
          Hex_WriteSpecific(1, value);
        }
        else if(i==0)
        {
          value=number&0x00000F;
          Hex_WriteSpecific(0, value);
        }
      }
}

/*
int SevenSegment::Get_Count(double Hz) {
    return (200000000/Hz) / 2 ;
}

int SevenSegment::ReturnNote(string note) {
    if(note.compare("C5") == 0) {
      Hex_WriteSpecific(1, 12) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(523.25) ;
    } else if (note.compare("D5") == 0) {
      Hex_WriteSpecific(1, 13) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(587.33) ;
    } else if (note.compare("E5") == 0) {
      Hex_WriteSpecific(1, 14) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(659.26) ;
    } else if (note.compare("F5") == 0) {
      Hex_WriteSpecific(1, 15) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(698.46) ;
    } else if (note.compare("G5") == 0) {
      Hex_WriteSpecific(1, 16) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(783.99) ;
    } else if (note.compare("A5") == 0) {
      Hex_WriteSpecific(1, 10) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(880.00) ;
    } else if (note.compare("B5") == 0) {
      Hex_WriteSpecific(1, 11) ;
      Hex_WriteSpecific(0, 5) ;
      return Get_Count(987.77) ;
    } else if (note.compare("C6") == 0) {
      Hex_WriteSpecific(1, 12) ;
      Hex_WriteSpecific(0, 6) ;
      return Get_Count(1046.5) ;
    } else if (note.compare("D6") == 0) {
      Hex_WriteSpecific(1, 13) ;
      Hex_WriteSpecific(0, 6) ;
      return Get_Count(1174.7) ;
    } else if (note.compare("E6") == 0) {
      Hex_WriteSpecific(1, 14) ;
      Hex_WriteSpecific(0, 6) ;
      return Get_Count(1318.5) ;
    } else if (note.compare("F6") == 0) {
      Hex_WriteSpecific(1, 15) ;
      Hex_WriteSpecific(0, 6) ;
      return Get_Count(1396.9) ;
    } else if (note.compare("G6") == 0) {
      Hex_WriteSpecific(1, 16) ;
      Hex_WriteSpecific(0, 6) ;
      return Get_Count(1568) ;
    } else {
      cout << "Enter a valid note!" ;
    }
}
*/








