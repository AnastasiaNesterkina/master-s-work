qsub -v nConnect=$5 -l select=$1:ncpus=$2:mem=$3,walltime=$4,place=scatter:excl ./scripts/cpoisson.sh >> ./scripts/id_clients.txt
