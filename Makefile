mention: lander.o
	gcc -Wall -std=c99 -o lander lander.o -lm -g -lcurses

mention.o: lander.c
        gcc -Wall -std=c99 lander.c -lm -g

clean:
	-rm -f *.o lander core
