CC = cc
CFLAGS = -Wall -Wextra -std=c99 -pedantic

OBJS = main.o math.o string.o memory.o screen.o keyboard.o

game_of_life: $(OBJS)
	$(CC) $(CFLAGS) -o game_of_life $(OBJS)

main.o: main.c keyboard.h math.h memory.h screen.h string.h
math.o: math.c math.h
string.o: string.c string.h
memory.o: memory.c memory.h
screen.o: screen.c screen.h string.h
keyboard.o: keyboard.c keyboard.h

clean:
	rm -f $(OBJS) game_of_life

.PHONY: clean
