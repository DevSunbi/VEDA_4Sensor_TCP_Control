CC = aarch64-linux-gnu-gcc
SYSROOT = $(HOME)/rpi-sysroot

CFLAGS = -Wall -Wextra -fPIC -I./lib -I. --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/local/include

CROSS_LDFLAGS = --sysroot=$(SYSROOT) \
                -L$(SYSROOT)/lib/aarch64-linux-gnu \
                -L$(SYSROOT)/usr/lib/aarch64-linux-gnu \
                -L$(SYSROOT)/usr/lib \
                -L$(SYSROOT)/usr/local/lib \
                -Wl,-rpath-link=$(SYSROOT)/lib/aarch64-linux-gnu \
                -Wl,-rpath-link=$(SYSROOT)/usr/lib/aarch64-linux-gnu \
                -Wl,-rpath-link=$(SYSROOT)/usr/lib \
                -Wl,-rpath-link=$(SYSROOT)/usr/local/lib

# Target files
TARGET_SERVER = server
LIBS = libdevice.so
#LIBS = libled.so libbuzzor.so libphotoresistor.so libsegment.so

all: $(TARGET_SERVER) $(LIBS)

$(TARGET_SERVER): src/main.c lib/rpi_common.c
	$(CC) $(CFLAGS) -o $@ $^ $(CROSS_LDFLAGS) -rdynamic -lwiringPi -lpthread -ldl

libdevice.so: lib/led.c lib/buzzor.c lib/photoresistor.c lib/segment.c

#libled.so: lib/led.c
#	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

#libbuzzor.so: lib/buzzor.c
#	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

#libphotoresistor.so: lib/photoresistor.c
#	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

#libsegment.so: lib/segment.c
#	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

clean:
	rm -f $(TARGET_SERVER) $(LIBS)
