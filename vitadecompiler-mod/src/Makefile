TARGET = vitadecompiler
OBJS = main.o vita.o utils.o asprintf.o import/vita-import.o import/vita-import-parse.o import/yamltree.o import/yamltreeutil.o relocate.o

LIBS = -lcapstone -lyaml -lz

CFLAGS = -g -s -static -Wall -Wextra -std=c99 -std=c++0x 
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Creating binary $(TARGET)"
	$(CXX) $(OBJS) -o $@ $(LIBS)  -static -static-libgcc -static-libstdc++ 

%.o: %.cpp
	@echo "Compiling $^"
	$(CXX) $(CFLAGS) -c $^ -o $@ -static -static-libgcc -static-libstdc++ 

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)

install: all
	@cp $(TARGET) ../bin
