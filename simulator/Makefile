TARGET=single_cycle

$(TARGET):main.o 
	gcc main.o -o $(TARGET)
main.o:main.c simulator.h
	gcc -c main.c
clean:
	rm -f $(TARGET) *.o *.bin *.rpt *.exe *.out
