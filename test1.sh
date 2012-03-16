#!/bin/bash

make
make lock_tester

./lock_server 3772 &
./lock_tester 3772

killall lock_server
killall lock_server
killall lock_server
