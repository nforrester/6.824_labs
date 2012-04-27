#!/bin/bash

make

rm -f *.log
echo ""
echo "---------- BEGINNING TESTS 0-7 ----------"
echo ""
./rsm_tester.pl 0 1 2 3 4 5 6 7
echo ""
echo "----------- ENDING TESTS 0-7 ------------"
echo ""

killall lock_server
#rm -f *.log
