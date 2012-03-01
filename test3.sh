#!/bin/bash

./start.sh
echo ""
echo "---------- BEGINNING TEST A ----------"
echo ""
./test-lab-3-a.pl ./yfs1
echo ""
echo "----------- ENDING TEST A ------------"
echo ""
echo ""
echo "---------- BEGINNING TEST B ----------"
echo ""
./test-lab-3-b.pl ./yfs1 ./yfs2
echo ""
echo "----------- ENDING TEST B ------------"
echo ""
echo ""
echo "---------- BEGINNING TEST C ----------"
echo ""
./test-lab-3-c ./yfs1 ./yfs2
echo ""
echo "----------- ENDING TEST C ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh
