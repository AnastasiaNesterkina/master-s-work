prefix=$(date +%Y-%m-%d)
suffix=$(date +%H:%M:%S)

folderName=$prefix.$suffix-$1-$2
mkdir ./src/loading/$folderName
mkdir ./src/source/$folderName
mkdir ./src/source/$folderName/allReduce
mkdir ./src/source/$folderName/barrier
mkdir ./src/source/$folderName/dup
mkdir ./src/source/$folderName/events
mkdir ./src/source/$folderName/recv
mkdir ./src/source/$folderName/send
mkdir ./src/source/$folderName/wait
startTime=$prefix' '$suffix":: waiting..."
echo $startTime >> ./src/source/$folderName/globalTime.txt
qsub -v folderName=$folderName -l select=$1:ncpus=$2:mem=3000m,walltime=1:00:0,place=scatter:excl spoisson.sh
