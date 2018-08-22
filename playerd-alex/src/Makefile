PLAYERD = playerd
PLAYERD_FILES = mosquitto_client.c parse.c mpd.c base64.c msg.c queue.c

TARGET = $(PLAYERD)
PLAYERD_OBJS = $(PLAYERD_FILES:.c=.o)

all: $(TARGET) 

$(PLAYERD): $(PLAYERD_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lrt -lmpdclient -luci -o $@ $^ 

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
