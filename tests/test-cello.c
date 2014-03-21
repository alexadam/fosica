#include "Cello.h"

// gcc -g -Wall -lefence -std=c99 -o test-cello test-cello.c -I/usr/local/include/ -L/usr/local/lib -lCello -Wl,-rpath -Wl,/usr/local/lib

int main(int argc, char** argv) {
  println("Cello World!");
}
