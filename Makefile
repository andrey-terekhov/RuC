TARGET = ruc
SRC = RuC/codegen.c RuC/codes.c RuC/error.c RuC/extdecl.c RuC/import.c RuC/main.c RuC/preprocessor.c RuC/scaner.c RuC/threads.c
CFLAGS = -lm -lpthread -DROBOTS

.PHONY:	 all clean

all: clean $(TARGET)

$(TARGET): $(SRC)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(TARGET)
