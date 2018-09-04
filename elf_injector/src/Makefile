TARGET = vita-elf-inject

OBJS = main.o sha256.o

LIBS = -lz 

CFLAGS = -s -static -Wall -Wextra -std=c99
all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Creating binary $(TARGET)"
	$(CXX) $(OBJS) -o $@ $(LIBS)  -static -static-libgcc

%.o: %.cpp
	@echo "Compiling $^"
	$(CXX) $(CFLAGS) -c $^ -o $@ -static -static-libgcc 

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)

install: all
	@cp $(TARGET) ../bin
