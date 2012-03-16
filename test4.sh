#!/bin/bash

export RPC_COUNT=25

make clean
make --makefile=GNUmakefileLab3Compare

./start.sh
echo ""
echo "---------- BEGINNING LAB 3 TEST ----------"
echo ""
./test-lab-3-c ./yfs1 ./yfs2
echo ""
echo "----------- ENDING LAB 3 TEST ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh

mv lock_server.log lock_server3.log
mv extent_server.log extent_server3.log

make clean
make

./start.sh
echo ""
echo "---------- BEGINNING LAB 4 TEST ----------"
echo ""
./test-lab-3-c ./yfs1 ./yfs2
echo ""
echo "----------- ENDING LAB 4 TEST ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh

mv lock_server.log lock_server4.log
mv extent_server.log extent_server4.log

echo "Extent server lab 3:"
grep STATS extent_server3.log | tail -n1
echo "Extent server lab 4:"
grep STATS extent_server4.log | tail -n1

echo "Lock server lab 3:"
grep STATS lock_server3.log | tail -n1
echo "Lock server lab 4:"
grep STATS lock_server4.log | tail -n1

rm extent_server3.log
rm extent_server4.log
rm lock_server3.log
rm lock_server4.log

make clean

unset RPC_COUNT
