qsub -l select=1:ncpus=1:mem=2000m,walltime=0:01:00,place=free cpoisson.sh > kill.txt
