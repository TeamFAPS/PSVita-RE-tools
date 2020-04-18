TARGET = PSVita-error-code-resolver
OBJS = main.o
LIBS = 
CFLAGS = -static -Os -s -Wall -Wextra -std=c99 -static-libgcc

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Creating binary $(TARGET)"
	$(CXX) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: %.cpp
	@echo "Compiling $^"
	$(CXX) $(CFLAGS) -c $^ -o $@ $(LIBS)

clean:
	@echo "Removing all the .o files"
	@$(RM) $(OBJS)

mrproper: clean
	@echo "Removing binary"
	@$(RM) $(TARGET)

install: all
	@cp $(TARGET) ../bin
