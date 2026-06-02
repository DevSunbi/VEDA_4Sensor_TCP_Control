CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I./lib -I.
LDFLAGS = -rdynamic -lwiringPi -lpthread -ldl

# Target files
TARGET_SERVER = server
LIBS = libled.so libbuzzor.so libphotoresistor.so libsegment.so

all: $(TARGET_SERVER) $(LIBS)

$(TARGET_SERVER): src/main.c lib/rpi_common.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

libled.so: lib/led.c
	$(CC) $(CFLAGS) -shared -o $@ $^ -lwiringPi

libbuzzor.so: lib/buzzor.c
	$(CC) $(CFLAGS) -shared -o $@ $^ -lwiringPi

libphotoresistor.so: lib/photoresistor.c
	$(CC) $(CFLAGS) -shared -o $@ $^ -lwiringPi

libsegment.so: lib/segment.c
	$(CC) $(CFLAGS) -shared -o $@ $^ -lwiringPi

clean:
	rm -f $(TARGET_SERVER) $(LIBS)
