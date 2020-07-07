#!/bin/bash

####PBS -l walltime=00:01:00
####PBS -l select=$nNodes:ncpus=$nNodes:mem=2000m,place=scatter
#PBS -m n

cd $PBS_O_WORKDIR

MPI_NP=$(wc -l $PBS_NODEFILE | awk '{ print $1 }')
echo "Number of MPI process: $MPI_NP" >> ./src/source/$folderName/globalTime.txt

echo 'File $PBS_NODEFILE:'
cat  $PBS_NODEFILE
echo 
cat /proc/cpuinfo
echo
cat /proc/meminfo
echo
prefixBeg=$(date +%Y-%m-%d)
suffixBeg=$(date +%H:%M:%S)
echo $prefixBeg' '$suffixBeg > ./src/source/$folderName/date.txt

/mnt/storage/home/magorodnichev/nestyorkina/soft/bin/mpirun -hostfile $PBS_NODEFILE -np $MPI_NP ./server.exe $folderName $countOfConnect $time $nodes $n_cpus $memory

prefixEnd=$(date +%Y-%m-%d)
suffixEnd=$(date +%H:%M:%S)
resultBeg=$prefixBeg' '$suffixBeg":: server start work"
resultEnd=$prefixEnd' '$suffixEnd":: server finish work"
echo $resultBeg >> ./src/source/$folderName/globalTime.txt
echo $resultEnd >> ./src/source/$folderName/globalTime.txt
