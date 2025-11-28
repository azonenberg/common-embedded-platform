#!/bin/bash

# Extract the sections of interest to a binary file
arm-none-eabi-objcopy -O binary \
	--only-section=.text --only-section=.rodata --only-section=.tcmtext --only-section=.data \
	$1 $1.bin
arm-none-eabi-strip -s -o $1-stripped.elf $1

LINE=`ls -lh $1.bin | cut -d ' ' -f 5`
echo "Image size:         $LINE";

STACKSTART=$(arm-none-eabi-objdump -t $1  | grep __end | cut -d ' ' -f 1);
STACKEND=$(arm-none-eabi-objdump -t $1  | grep __stack | cut -d ' ' -f 1);
DSTACKSTART=$(echo "obase=10;ibase=16;${STACKSTART^^}" | bc);
DSTACKEND=$(echo "obase=10;ibase=16;${STACKEND^^}" | bc);
STACKSIZE=$(expr $DSTACKEND - $DSTACKSTART);
#STACKKB=$(expr $STACKSIZE / 1024);
echo "Stack size:         $STACKSIZE bytes";

DATASTART=$(arm-none-eabi-objdump -t $1  | grep __data_start | cut -d ' ' -f 1);
DATAEND=$(arm-none-eabi-objdump -t $1  | grep __data_end | cut -d ' ' -f 1);
DDATASTART=$(echo "obase=10;ibase=16;${DATASTART^^}" | bc);
DDATAEND=$(echo "obase=10;ibase=16;${DATAEND^^}" | bc);
DATASIZE=$(expr $DDATAEND - $DDATASTART);
DATAKB=$(expr $DATASIZE / 1024);
echo "Global data size:   $DATASIZE bytes";

BSSSTART=$(arm-none-eabi-objdump -t $1  | grep __bss_start | cut -d ' ' -f 1);
BSSEND=$(arm-none-eabi-objdump -t $1  | grep __bss_end | cut -d ' ' -f 1);
DBSSSTART=$(echo "obase=10;ibase=16;${BSSSTART^^}" | bc);
DBSSEND=$(echo "obase=10;ibase=16;${BSSEND^^}" | bc);
BSSSIZE=$(expr $DBSSEND - $DBSSSTART);
BSSKB=$(expr $BSSSIZE / 1024);
echo "Global BSS size:    $BSSSIZE bytes";

GLOBALSIZE=$(expr $DATASIZE + $BSSSIZE);
GLOBALKB=$(expr $GLOBALSIZE / 1024);
echo "Total global size:  $GLOBALSIZE bytes";

IPCBUFSIZE=$(arm-none-eabi-objdump -h $1 | grep ipcbuf | cut -c 19-26);
DIPCBUFSIZE=$(echo "obase=10;ibase=16;${IPCBUFSIZE^^}" | bc);
echo "IPC buffer size:    $DIPCBUFSIZE bytes";

# Run mkfsbl to build the FSBL-M image
DIR=$(dirname $1)
mkfsbl m $1.bin $DIR/fsbl.bin
