# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
INCLUDES = -I./include
LIBS = -lz

# Executable names
TARGETS = findpng pnginfo catpng

# Source files
FINDPNG_SRC = src/findpng.c src/crc.c src/zutil.c src/lab_png.c
PNGINFO_SRC = src/pnginfo.c src/lab_png.c src/crc.c src/zutil.c
CATPNG_SRC  = src/catpng.c src/crc.c src/zutil.c src/lab_png.c

# Build rules
all: $(TARGETS)

findpng: $(FINDPNG_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

pnginfo: $(PNGINFO_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

catpng: $(CATPNG_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean findpng pnginfo catpng
