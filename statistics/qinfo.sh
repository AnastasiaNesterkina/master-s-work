#!/bin/bash
file="./qstatinfo.json"
last=2016
for((i=1; i <= "$last"; i++))
do 
echo "{" > $file
prefix=$(date +%d.%m.%Y)
suffix=$(date +%H:%M:%S)
d="\"date\": \""$prefix" "$suffix"\","
echo $d >> $file
echo "\"info\": " >> $file
qstat -f -F json >> $file
echo "}" >> $file
python ./parser.py
sleep 5m
done

