#!/bin/bash

./start.sh
echo ""
echo "---------- BEGINNING TEST A ----------"
echo ""
./test-lab-2-a.pl ./yfs1
echo ""
echo "----------- ENDING TEST A ------------"
echo ""
./stop.sh
./stop.sh
./stop.sh
