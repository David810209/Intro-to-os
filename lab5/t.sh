#!/bin/bash
rm t
g++ -o t hw5_111550076.cpp

./t ./inceptionV3_tf_oshw5.txt
# ./t ./double.txt
echo "-------------------------------------------------------------"
