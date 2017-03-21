CC = gcc
CFLAGS = -g -m32

all: bmpFilter1 bmpFilter2

bmpFilter1: bmpBlackWhite.o
	mpicc bmpBlackWhite.o bmpFilterStatic.c -o bmpFilterStatic

bmpFilter2: bmpBlackWhite.o
	mpicc bmpBlackWhite.o bmpFilterDynamic.c -o bmpFilterDynamic

clean:
	rm -f bmpFilterStatic bmpFilterDynamic