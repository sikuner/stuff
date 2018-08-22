#common makefile header

DIR_INC = ./
DIR_BIN = ./
DIR_LIB = ./

TARGET	= sfvoice
BIN_TARGET = $(DIR_BIN)/$(TARGET)

CROSS_COMPILE = mips-linux-gnu-
CFLAGS = -g -Wall -I$(DIR_INC)

#可根据实际需要修改，选择所需平台SDK，默认为x64或x86平台。
LDFLAGS := -L$(DIR_LIB)/mips 

LDFLAGS += -lmsc -lrt -ldl -lpthread -lasound

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

$(BIN_TARGET) : $(OBJECTS)
	$(CROSS_COMPILE)gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@
clean:
	@rm -f *.o $(BIN_TARGET)

.PHONY:clean

#common makefile foot
