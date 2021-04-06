#!/bin/bash


echo Bash version:
uname -a

# exit on error
set -e

mkdir -p build/
cd build
cmake ../
make 
