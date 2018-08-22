
DIR_INC = ./
DIR_BIN = ./
DIR_LIB = ./

TARGET	= down 
BIN_TARGET = $(DIR_BIN)/$(TARGET)

CFLAGS += -g -Wall -I$(DIR_INC)

LDFLAGS := 

LDFLAGS += -lm -lcurl -lpthread -lsqlite3 -lssl -lcrypto -ljson-c -lblobmsg_json -lubus -lubox

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))

all: $(TARGET)

$(BIN_TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	@rm -f *.o $(BIN_TARGET)

.PHONY:clean

