/*****
 * Gets a NON-DEBOUNCED key from the keypad. Note that this also shows the 
 * use of procedures and functions in C.
 *
 * This program detects a key pressed on the hexkeypad and displays it on the
 * red LEDs by lighting the corresponding LED
 *****/

#define ADDR_JP1PORT ((volatile long *) 0xFF200060)
#define HEXPAD_DIR ((volatile long *) 0xFF200064)
#define ADDR_LEDR ((volatile long *) 0xFF200000)
unsigned char colstims[]={0xe0,0xd0,0xb0,0x70}; //to run the hexpad


// start the hex keypad
// note that since this uses the GPIO interface you must set the
// directions of the individual bits. See the section on GPIO for
// more information.
void HexkeypadInit()
{
   *HEXPAD_DIR = 0xf0; // set directions . rows in, cols out
   *ADDR_JP1PORT = 0x0f; //set all rows to high
}

// return key found pressed, -1 if no key pressed
// note: if multiple keys pressed, returns the first key
// does not debounce key!
char HexkeypadIn()
{
  // Wait until a key is pressed
  int rowRead = (*ADDR_JP1PORT) & 0x0f;
  while (rowRead == 0x0f) {
    rowRead = (*ADDR_JP1PORT) & 0x0f;
  }
  
  // Key has been pressed!  Debounce!
  int d = 0;
  while (d < 100) {
    d++;
  }

  // Switch to read columns
  
  *HEXPAD_DIR = 0x0f; // rows out, cols in
  *ADDR_JP1PORT = 0xf0;  // set all cols to high
  
  // Process rows
  //rowRead = (*ADDR_JP1PORT) & 0x0f;
  rowRead = rowRead ^ 0x0f; // invert the number
  // Set it to first confirmed row
  if (rowRead & 0x01) {
    rowRead = 0;
  } else if (rowRead & 0x02) {
    rowRead = 1;
  } else if (rowRead & 0x04) {
    rowRead = 2;
  } else if (rowRead & 0x08) {
    rowRead = 3;
  } else {
    rowRead = -1;
  }
  

  // Read column
  int colRead = (*ADDR_JP1PORT) & 0xf0;
  // Process column
  colRead = colRead ^ 0xf0; // invert the number
  // Set it to first confirmed column
  if (colRead & 0x10) {
    colRead = 0;
  } else if (colRead & 0x20) {
    colRead = 1;
  } else if (colRead & 0x40) {
    colRead = 2;
  } else if (colRead & 0x80) {
    colRead = 3;
  } else {
    colRead = -1;
  }
  
  // Reset rows and columns
  *HEXPAD_DIR = 0xf0; // rows in, cols out
  *ADDR_JP1PORT = 0x0f;  // set all cols to high
  
  // Determine from rows and columns which key was pressed
  char numRead = -1;
  
  if ((rowRead != -1) && (colRead != -1)) {
    numRead = (char) (4*rowRead) + colRead;
  }
  return(numRead);

}  


int main()
{ 
   char inputkey;

   HexkeypadInit(); //this is a procedure to start the keypad

   while (1)
   {
      inputkey = HexkeypadIn(); //function, returns key pressed
      if (inputkey != -1) {
	    int i;
		int val = 1;
	    for (i = 0; i < inputkey; i++)
		{
		  val = val << 1;
		}
        *ADDR_LEDR = val;
      }
   }
}