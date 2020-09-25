.PHONY: all clean

PROJECT = ictoonmo
SRC = src/main.cpp src/font.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)
CFLAGS = -g -Iinc
LDFLAGS = $(shell pkg-config --libs sdl)
CC = g++

all: $(PROJECT)

$(PROJECT): $(OBJ)
	$(CC) -o $(PROJECT) $(OBJ) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

src/%.d: src/%.cpp
	@set -e; \
	rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,src/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(PROJECT) $(OBJ) $(DEP) src/*.d.*

-include $(DEP)
