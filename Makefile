CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_DEFAULT_SOURCE -Iinclude
SRC    = src/main.c src/server.c src/http.c src/files.c src/mime.c
TARGET = minihttpd

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean