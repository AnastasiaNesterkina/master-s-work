#!/bin/bash
prefix=$(date +%Y-%m-%d)
suffix=$(date +%H:%M:%S)
folderName=$prefix.$suffix
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
mpirun -np 2 ./server.exe $folderName