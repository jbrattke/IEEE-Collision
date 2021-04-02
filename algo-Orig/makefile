main: DE1SoCfpga.o loShapes.o Shapes.o main.o
	g++ DE1SoCfpga.o loShapes.o Shapes.o main.o -o makefile

main.o: main.cpp
	g++ -g -Wall -c main.cpp

SevenSegment.o: SevenSegment.cpp SevenSegment.h
	g++ -g -Wall -c SevenSegment.cpp

DE1SoCfpga.o: DE1SoCfpga.cpp DE1SoCfpga.h
	g++ -g -Wall -c DE1SoCfpga.cpp
 
loShapes.o: loShapes.cpp loShapes.h
	g++ -g -Wall -c loShapes.cpp
  
Shapes.o: Shapes.cpp Shapes.h
	g++ -g -Wall -c Shapes.cpp

clean:
	rm -f DE1SoCfpga.o SevenSegment.o Shapes.o main.o loShapes.o