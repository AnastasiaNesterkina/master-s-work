#!/bin/bash
#PBS -m n
cd $PBS_O_WORKDIR

MPI_NP=$(wc -l $PBS_NODEFILE | awk '{ print $1 }')
echo "Number of MPI process: $MPI_NP"

echo 'File $PBS_NODEFILE:'
cat  $PBS_NODEFILE
echo

/mnt/storage/home/magorodnichev/nestyorkina/soft/bin/mpirun -hostfile $PBS_NODEFILE -np $MPI_NP ./client.exe 
