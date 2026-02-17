CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

TARGET = scope

SRC = main.c renderer.c signal.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

