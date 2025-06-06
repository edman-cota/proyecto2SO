CC = gcc
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0` -Iinclude
LDFLAGS = `pkg-config --libs gtk+-3.0`
OBJ = src/main.o src/scheduler.o src/file_loader.o src/gui.o
TARGET = build/simulador

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf build src/*.o
