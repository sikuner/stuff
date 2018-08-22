#common makefile header

DIR_INC = ../include
DIR_BIN = ./
DIR_LIB = ../libs

TARGET	= resolvd
BIN_TARGET = $(DIR_BIN)/$(TARGET)

LDFLAGS += -lm -lrt -ldl -lresolv -lpthread -lcurl -ljson-c -luci

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