#!/bin/bash
rm t
g++ -o t t.cpp

# ./t ./inceptionV3_tf_oshw5.txt
./t ./double.txt
echo "-------------------------------------------------------------"
