#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
using namespace std;

// Important Message from Prof. Marpaung
// Read the PDF to find the real ADDRESS of LEDR, SW and KEY.
// End Important Message

// Physical base address of FPGA Devices 
const unsigned int LW_BRIDGE_BASE 	= 0xFF200000;  // Base offset 

// Length of memory-mapped IO window 
const unsigned int LW_BRIDGE_SPAN 	= 0x00DEC700;  // Address map size

// Cyclone V FPGA device addresses
const unsigned int LEDR_OFFSET 		=  0xFF200000 - LW_BRIDGE_BASE ;//real ADDRESS of RED LED - LW_BRIDGE_BASE ;
const unsigned int SW_OFFSET 		=  0xFF200040 - LW_BRIDGE_BASE;//real ADDRESS of SWITCH - LW_BRIDGE_BASE ;
const unsigned int KEY_OFFSET 		=  0xFF200050 - LW_BRIDGE_BASE;//real ADDRESS of PUSH BUTTON - LW_BRIDGE_BASE ;

const unsigned int bit_values[16] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 121, 113};
const unsigned int HEX0_3= 0xFF200020 - LW_BRIDGE_BASE ;
const unsigned int HEX4_5= 0xFF200030 - LW_BRIDGE_BASE ;

const unsigned int MPCORE_PRIV_TIMER_LOAD_OFFSET = 0xDEC600 ;
const unsigned int MPCORE_PRIV_TIMER_COUNTER_OFFSET = 0xFFFEC604 - LW_BRIDGE_BASE ;
const unsigned int MPCORE_PRIV_TIMER_CONTROL_OFFSET = 0xFFFEC608 - LW_BRIDGE_BASE ;
const unsigned int MPCORE_PRIV_TIMER_INTERRUPT_OFFSET = 0xFFFEC60C - LW_BRIDGE_BASE ;

const unsigned int JP1_Data_Register = 0xFF200060 - LW_BRIDGE_BASE;
const unsigned int JP1_Direction_Register = 0xFF200064 - LW_BRIDGE_BASE;

class DE1SoCfpga {
  public:
    char *pBase ;
    int fd ;
    
    DE1SoCfpga() {
    
   	  // Open /dev/mem to give access to physical addresses
    	fd = open( "/dev/mem", (O_RDWR | O_SYNC));
	    if (fd == -1) {			//  check for errors in openning /dev/mem
        cout << "ERROR: could not open /dev/mem..." << endl;
        exit(1);
    	}
      
     // Get a mapping from physical addresses to virtual addresses
     char *virtual_base = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
     if (virtual_base == MAP_FAILED) {		// check for errors
        cout << "ERROR: mmap() failed..." << endl;
        close (fd);		// close memory before exiting
        exit(1);        // Returns 1 to the operating system;
     }
     pBase = virtual_base; 
   }
   
   ~DE1SoCfpga() {
   
     	if (munmap (pBase, LW_BRIDGE_SPAN) != 0) {
      cout << "ERROR: munmap() failed..." << endl;
      exit(1);
      
	    }
     close (fd); 	// close memory
   }
   
  void RegisterWrite(unsigned int reg_offset, int value) { 
	  * (volatile unsigned int *)(pBase + reg_offset) = value; 
  } 

  int RegisterRead(unsigned int reg_offset) { 
  	return * (volatile unsigned int *)(pBase + reg_offset); 
  } 

};

class SevenSegment : DE1SoCfpga {
  private: 
    unsigned int reg0_hexValue ;
    unsigned int reg1_hexValue ;
    unsigned int initialvalueLoadMPCore ;
    unsigned int initialvalueControlMPCore ;
    unsigned int initialvalueInterruptMPCore ;

  public:
        
    SevenSegment(){
      reg0_hexValue = RegisterRead(HEX0_3);
      reg1_hexValue = RegisterRead(HEX4_5);
      initialvalueLoadMPCore = RegisterRead(MPCORE_PRIV_TIMER_LOAD_OFFSET);
      initialvalueControlMPCore = RegisterRead(MPCORE_PRIV_TIMER_CONTROL_OFFSET);
      initialvalueInterruptMPCore = RegisterRead(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET);
      RegisterWrite(JP1_Direction_Register, 1) ;
    }
    
    ~SevenSegment(){
      Hex_ClearAll();
      RegisterWrite(MPCORE_PRIV_TIMER_LOAD_OFFSET, initialvalueLoadMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_CONTROL_OFFSET, initialvalueControlMPCore);
      RegisterWrite(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET, initialvalueInterruptMPCore);

    }
    
    void Hex_AllOn() {
      RegisterWrite(HEX0_3, 0xFFFFFFFF);
      RegisterWrite(HEX4_5, 0xFFFF) ;
    }
    
    void Hex_ClearAll() {
      RegisterWrite(HEX0_3, 0) ;
      RegisterWrite(HEX4_5, 0) ;
    }
    
    void Hex_ClearSpecific(int index) {
      reg0_hexValue = RegisterRead(HEX0_3);
      reg1_hexValue = RegisterRead(HEX4_5);
      int test = 1;
      if(index == 0) {
        RegisterWrite(HEX0_3, 0xFFFFFF00 & reg0_hexValue) ;
      } else if (index == 1) {
        RegisterWrite(HEX0_3, 0xFFFF00FF & reg0_hexValue) ;
      } else if (index == 2) {
        RegisterWrite(HEX0_3, 0xFF00FFFF & reg0_hexValue) ;
      } else if (index == 3) {
        RegisterWrite(HEX0_3, 0x00FFFFFF & reg0_hexValue) ;
      } else if (index == 4) {
        RegisterWrite(HEX4_5, 0xFF00 & reg1_hexValue) ;
      } else if (index == 5) {
        RegisterWrite(HEX4_5, 0x00FF & reg1_hexValue) ;
      } else {
        cout << "Not a valid index number!" ;
      }
    }
  
  void Hex_WriteSpecific(int index, int value) {
      reg0_hexValue = RegisterRead(HEX0_3);
      reg1_hexValue = RegisterRead(HEX4_5);
      unsigned int decValue = bit_values[value] ;
      if(index == 0) {
        RegisterWrite(HEX0_3, (0xFFFFFF00 & reg0_hexValue) ^ decValue) ;
        
      } else if (index == 1) {
        RegisterWrite(HEX0_3, (0xFFFF00FF & reg0_hexValue) ^ (decValue << 8)) ;
        
      } else if (index == 2) {
        RegisterWrite(HEX0_3, (0xFF00FFFF & reg0_hexValue) ^ (decValue << 16)) ;
        
      } else if (index == 3) {
        RegisterWrite(HEX0_3, (0x00FFFFFF & reg0_hexValue) ^ (decValue << 24)) ;
        
      } else if (index == 4) {
        RegisterWrite(HEX4_5, (0xFF00 & reg1_hexValue) ^ decValue) ;
        
      } else if (index == 5) {
        RegisterWrite(HEX4_5, (0x00FF & reg1_hexValue) ^ (decValue << 8)) ;
        
      } else if (index > 5 ^ value > 15 ^ value < 0) {
        cout << "Not a valid index number!" ;
      }
  }
  
  void Hex_WriteNumber(int number) {
      reg0_hexValue = RegisterRead(HEX0_3);
      reg1_hexValue = RegisterRead(HEX4_5);
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
    
  void RegisterWrite(unsigned int reg_offset, int value) { 
	  * (volatile unsigned int *)(pBase + reg_offset) = value; 
  } 

  int RegisterRead(unsigned int reg_offset) { 
  	return * (volatile unsigned int *)(pBase + reg_offset); 
  } 
};

int main() {

  SevenSegment *display = new SevenSegment;
  
  int counter = 227272 ; 
  
  display->RegisterWrite(MPCORE_PRIV_TIMER_LOAD_OFFSET, counter) ;
  display->RegisterWrite(MPCORE_PRIV_TIMER_CONTROL_OFFSET, 3) ;
  int stop = 0 ;
  int counter2 = 5 ;
  
  display->RegisterWrite(JP1_Data_Register, 0) ;
  int switchtone = 0 ;
  
  while (stop != 1) {
    stop = display->RegisterRead(SW_OFFSET) ;
    if (display->RegisterRead(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET) != 0) {
    
      display->RegisterWrite(MPCORE_PRIV_TIMER_INTERRUPT_OFFSET, 1) ;
      // reset timer flag bit
      
      if(switchtone == 0) {
      
        display->RegisterWrite(JP1_Data_Register, 1) ;
        switchtone = 1 ;
        
      } else {
    
        display->RegisterWrite(JP1_Data_Register, 0) ;
        switchtone = 0 ;
      }
      
    } 
  }
  
  delete display;
  return 0 ;
  
}

















