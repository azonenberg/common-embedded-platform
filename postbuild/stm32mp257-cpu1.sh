#!/bin/bash

# Extract the sections of interest to a binary file
aarch64-linux-gnu-objcopy -O binary \
	--only-section=.resetptr --only-section=.text --only-section=.rodata --only-section=.tcmtext --only-section=.data \
	$1 $1.bin
aarch64-linux-gnu-strip -s -o $1-stripped.elf $1

LINE=`ls -lh $1.bin | cut -d ' ' -f 5`
echo "Image size:         $LINE";

FREESTART=$(aarch64-linux-gnu-objdump -t $1  | grep __end | cut -d ' ' -f 1);
FREEEND=$(aarch64-linux-gnu-objdump -t $1  | grep __core0stackend | cut -d ' ' -f 1);
DFREESTART=$(echo "obase=10;ibase=16;${FREESTART^^}" | bc);
DFREEEND=$(echo "obase=10;ibase=16;${FREEEND^^}" | bc);
FREESIZE=$(expr $DFREEEND - $DFREESTART);
#FREEKB=$(expr $FREESIZE / 1024);
echo "Unallocated SYSRAM: $FREESIZE bytes";

STACKSTART=$(aarch64-linux-gnu-objdump -t $1  | grep __core0stackend | cut -d ' ' -f 1);
STACKEND=$(aarch64-linux-gnu-objdump -t $1  | grep __core0stackstart | cut -d ' ' -f 1);
DSTACKSTART=$(echo "obase=10;ibase=16;${STACKSTART^^}" | bc);
DSTACKEND=$(echo "obase=10;ibase=16;${STACKEND^^}" | bc);
STACKSIZE=$(expr $DSTACKEND - $DSTACKSTART);
#STACKKB=$(expr $STACKSIZE / 1024);
echo "Core 0 stack size:  $STACKSIZE bytes";

STACKSTART=$(aarch64-linux-gnu-objdump -t $1  | grep __core1stackend | cut -d ' ' -f 1);
STACKEND=$(aarch64-linux-gnu-objdump -t $1  | grep __core1stackstart | cut -d ' ' -f 1);
DSTACKSTART=$(echo "obase=10;ibase=16;${STACKSTART^^}" | bc);
DSTACKEND=$(echo "obase=10;ibase=16;${STACKEND^^}" | bc);
STACKSIZE=$(expr $DSTACKEND - $DSTACKSTART);
#STACKKB=$(expr $STACKSIZE / 1024);
echo "Core 1 stack size:  $STACKSIZE bytes";

DATASTART=$(aarch64-linux-gnu-objdump -t $1  | grep __data_start | cut -d ' ' -f 1);
DATAEND=$(aarch64-linux-gnu-objdump -t $1  | grep __data_end | cut -d ' ' -f 1);
DDATASTART=$(echo "obase=10;ibase=16;${DATASTART^^}" | bc);
DDATAEND=$(echo "obase=10;ibase=16;${DATAEND^^}" | bc);
DATASIZE=$(expr $DDATAEND - $DDATASTART);
DATAKB=$(expr $DATASIZE / 1024);
echo "Global data size:   $DATASIZE bytes";

BSSSTART=$(aarch64-linux-gnu-objdump -t $1  | grep __bss_start | cut -d ' ' -f 1);
BSSEND=$(aarch64-linux-gnu-objdump -t $1  | grep __bss_end | cut -d ' ' -f 1);
DBSSSTART=$(echo "obase=10;ibase=16;${BSSSTART^^}" | bc);
DBSSEND=$(echo "obase=10;ibase=16;${BSSEND^^}" | bc);
BSSSIZE=$(expr $DBSSEND - $DBSSSTART);
BSSKB=$(expr $BSSSIZE / 1024);
echo "Global BSS size:    $BSSSIZE bytes";

GLOBALSIZE=$(expr $DATASIZE + $BSSSIZE);
GLOBALKB=$(expr $GLOBALSIZE / 1024);
echo "Total global size:  $GLOBALSIZE bytes";

IPCBUFSIZE=$(aarch64-linux-gnu-objdump -h $1 | grep ipcbuf | cut -c 19-26);
DIPCBUFSIZE=$(echo "obase=10;ibase=16;${IPCBUFSIZE^^}" | bc);
echo "IPC buffer size:    $DIPCBUFSIZE bytes";

# Run mkfsbl to build the FSBL-A image
DIR=$(dirname $1)
mkfsbl a $1.bin $DIR/fsbl.bin
