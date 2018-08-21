OUT=./build
CC=gcc
TARGET=$@
LIB=-libusb
RM=rm

ifeq ($(OS),Windows_NT)
    #Windows
    TARGET=$@.exe
    LIB=libusb0.dll
    RM=del /Q
endif

all: bootlu1p

bootlu1p: main.c flashprog.c hexfile.c
	$(CC) $(LIB) -o $(OUT)/$(TARGET) $^

clean:
	$(RM) $(OUT)/*.o
	$(RM) $(OUT)/bootlu1p
	$(RM) $(OUT)/bootlu1p.exe