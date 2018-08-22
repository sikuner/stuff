#common makefile header

DIR_INC = ../include
DIR_BIN = ./
DIR_LIB = ../libs

TARGET	= playtts
BIN_TARGET = $(DIR_BIN)/$(TARGET)

ifdef LINUX64
CROSS_COMPILE = 
LDFLAGS := -L$(DIR_LIB)/x64
else
CROSS_COMPILE = mips-linux-gnu-
LDFLAGS := -L$(DIR_LIB)/mips
endif

LDFLAGS += -lmsc -lm -lrt -ldl -lpthread -lcrypto -lubus -lubox -lblobmsg_json -lcurl -lmpdclient -ljson-c

CFLAGS = -g -Wall -I$(DIR_INC)

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

$(BIN_TARGET) : $(OBJECTS)
	$(CROSS_COMPILE)gcc $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@

clean:
	@rm -f *.o $(BIN_TARGET)

.PHONY:clean

#common makefile foot