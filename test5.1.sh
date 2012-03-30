#!/bin/bash

export RPC_COUNT=1
export LAB=4

make clean
make -e

./start.sh
echo ""
echo "---------- BEGINNING LAB 4 TEST ----------"
echo ""
./test-lab-3-c ./yfs1 ./yfs1
echo ""
echo "----------- ENDING LAB 4 TEST ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh

mv lock_server.log lock_server4.log
mv extent_server.log extent_server4.log

export LAB=5

make clean
make -e

./start.sh
echo ""
echo "---------- BEGINNING LAB 5 TEST ----------"
echo ""
./test-lab-3-c ./yfs1 ./yfs1
echo ""
echo "----------- ENDING LAB 5 TEST ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh

mv lock_server.log lock_server5.log
mv extent_server.log extent_server5.log

echo ""
echo "RESULTS:"
echo ""

echo "Extent server lab 4:"
grep STATS extent_server4.log | tail -n1
echo "Extent server lab 5:"
grep STATS extent_server5.log | tail -n1

echo "Lock server lab 4:"
grep STATS lock_server4.log | tail -n1
echo "Lock server lab 5:"
grep STATS lock_server5.log | tail -n1

rm extent_server4.log
rm extent_server5.log
rm lock_server4.log
rm lock_server5.log

make clean

unset RPC_COUNT
unset LAB
