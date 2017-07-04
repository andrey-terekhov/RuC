TARGET = ruc
SRC = RuC/codegen.c RuC/codes.c RuC/error.c RuC/extdecl.c RuC/import.c RuC/main.c RuC/scaner.c
CFLAGS = -lm

.PHONY:	 all clean

all: clean $(TARGET)

$(TARGET): $(SRC)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(TARGET)
