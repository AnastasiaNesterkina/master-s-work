qsub -l select=1:ncpus=5:mem=3000m,walltime=$1,place=scatter:excl cpoisson.sh >> id_clients.txt
