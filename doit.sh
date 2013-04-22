#!/bin/bash

rm gpspipe.o gpspipe server

gcc -o gpspipe.o -c -D_GNU_SOURCE -Wextra -Wall -Wno-uninitialized -Wno-missing-field-initializers -Wcast-align -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith -Wreturn-type -g gpspipe.c

gcc -o gpspipe -Wl,-rpath=//usr/local/lib gpspipe.o -L. -L/usr/local/lib -lrt -lcap -lbluetooth -lgps -lm -lyajl


gcc -g -o server server.c -static -lyajl_s -lpthread
