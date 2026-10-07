CC = gcc

CFLAGS = -Wall -Wextra -pthread

TARGET = proxy

SOURCES = proxy.c http.c cache.c access_control.c logger.c

OBJECTS = $(SOURCES:.c=.o)


all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJECTS) $(TARGET)


run: $(TARGET)
	./$(TARGET)