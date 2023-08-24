CC = gcc 
cflags =
cflags += -Iinclude -I/usr/x86_64-w64-mingw32/include/SDL2 -D_REENTRANT
ldflags += -lstdc++ -lpthread -ldl -lSDL2main -lSDL2 -lSDL2_ttf -lpthread -I/usr/include/SDL2
name = GBEmuEsp32 

sources = $(wildcard src/*.c)
objects = $(patsubst src/%, object/%,$(sources:.c=.o))
deps = $(objects:.o=.d)

-include $(deps)
.PHONY: all clean

all: files $(name)

files:
	mkdir -p bin object

run: $(name)
#	bin/$(name) rom.gb
	bin/$(name) cpu_instrs.gb

$(name): $(objects)
	$(CC) -o bin/$(name) $^ $(ldflags)

object/%.o: src/%.c
	$(CC) -MMD -o $@ -c $< $(cflags)

clean:
	rm -rf bin/$(name) $(objects) object/*.d
