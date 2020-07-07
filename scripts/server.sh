#!/bin/bash
prefixBeg=$(date +%Y-%m-%d)
suffixBeg=$(date +%H:%M:%S)
folderName=$prefixBeg.$suffixBeg
echo $folderName
mkdir ./src/loading/$folderName
mkdir ./src/source/$folderName
mkdir ./src/source/$folderName/allReduce
mkdir ./src/source/$folderName/barrier
mkdir ./src/source/$folderName/dup
mkdir ./src/source/$folderName/events
mkdir ./src/source/$folderName/recv
mkdir ./src/source/$folderName/send
mkdir ./src/source/$folderName/wait
mpirun -np 4 ./server.exe $folderName
prefixEnd=$(date +%Y-%m-%d)
suffixEnd=$(date +%H:%M:%S)
result=$prefixBeg' '$suffixBeg'\n'$prefixEnd' '$suffixEnd
echo $result > ./src/source/$folderName/globalTime.txt