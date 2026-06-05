CC = aarch64-linux-gnu-gcc
HOST_CC = gcc
SYSROOT = $(HOME)/rpi-sysroot

CFLAGS = -Wall -Wextra -fPIC -I./lib -I./include -I. --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include -I$(SYSROOT)/usr/local/include

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
TARGET_CLIENT = client
#LIBS = libdevice.so
LIBS = libled.so libbuzzor.so libphotoresistor.so libsegment.so

all: $(TARGET_SERVER) $(TARGET_CLIENT) $(LIBS)

$(TARGET_SERVER): src/main.c lib/rpi_common.c
	$(CC) $(CFLAGS) -o $@ $^ $(CROSS_LDFLAGS) -rdynamic -lwiringPi -lpthread -ldl

$(TARGET_CLIENT): src/client.c
	$(HOST_CC) -o $@ $^

#libdevice.so: lib/led.c lib/buzzor.c lib/photoresistor.c lib/segment.c

libled.so: lib/led.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

libbuzzor.so: lib/buzzor.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

libphotoresistor.so: lib/photoresistor.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

libsegment.so: lib/segment.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT) $(LIBS)

# Pattern rule for single-file cross-compilation
# Usage: make blink (compiles blink.c -> blink)
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(CROSS_LDFLAGS) -lwiringPi -lpthread

# 배포 설정 변수 (사용자 환경에 맞게 오버라이드 가능)
# 예: make deploy RPI_IP=192.168.1.50 RPI_USR=pi RPI_DIR=/home/pi/myproject
RPI_IP ?= 192.168.0.100
RPI_USR ?= sunbi
RPI_DIR ?= /home/sunbi/prj

deploy: all
	ssh $(RPI_USR)@$(RPI_IP) "mkdir -p $(RPI_DIR)"
	scp $(TARGET_SERVER) $(LIBS) $(RPI_USR)@$(RPI_IP):$(RPI_DIR)/
	scp -r resources $(RPI_USR)@$(RPI_IP):$(RPI_DIR)/

