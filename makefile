SOURCES=$(shell find . -name "*.c")
OBJECTS=$(SOURCES:%.c=%.o)
TARGET=fat32
CC = clang
CFLAGS=-Wall

all: $(TARGET)
build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@


clean:
	rm -f $(TARGET) $(OBJECTS)