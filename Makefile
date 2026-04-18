CC = cc
CFLAGS = -Wall -Wextra -std=c99 -pedantic

TARGET = game-of-life
OBJS = main.o math.o string.o memory.o screen.o keyboard.o life_patterns.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c keyboard.h life_patterns.h math.h memory.h screen.h string.h
math.o: math.c math.h
string.o: string.c string.h
memory.o: memory.c memory.h
screen.o: screen.c screen.h string.h
keyboard.o: keyboard.c keyboard.h
life_patterns.o: life_patterns.c life_patterns.h math.h string.h

clean:
	rm -f $(OBJS) $(TARGET) game_of_life

.PHONY: all clean
