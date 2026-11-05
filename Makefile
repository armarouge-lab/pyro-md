CC     = cc
CFLAGS = -std=c17 -Wall -Wextra -O2 -g
TARGET = pyro
SRC    = src/pyro.c
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
clean:
	rm -f $(TARGET)
.PHONY: clean
