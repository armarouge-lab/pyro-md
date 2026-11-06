CC     = gcc
CFLAGS = -std=c17 -Wall -Wextra -Werror -O2 -g
TARGET = pyro
SRC    = src/pyro.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
