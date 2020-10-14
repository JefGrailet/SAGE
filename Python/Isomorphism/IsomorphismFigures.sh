#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: ./IsomorphismFigures.sh [ASes] [Dates]"
    exit 1
fi

datesFile=${2}

# Retrieves ASes
n_ASes=$( cat ${1} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval  'ASes=($(cat ${1}))'

# Computes name of the destination folder
n_dates=$( cat ${2} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval  'dates=($(cat ${2}))'
lastIndex=`expr $n_dates - 1`
firstDataset=${dates[0]}
firstDataset=${firstDataset//\//-}
lastDataset=${dates[$lastIndex]}
lastDataset=${lastDataset//\//-}
dstFolder=$firstDataset" to "$lastDataset

i=0
while [ $i -lt $n_ASes ]
do
    if [ $i -gt 0 ]; then
        echo " " # Adds a new line between the dumps of each run of CompareGraphs.py
    fi
    python CompareGraphs.py ${ASes[$i]} $datesFile
    mkdir -p ./Figures/"$dstFolder"
    mv ${ASes[$i]}.pdf ./Figures/"$dstFolder"
    i=`expr $i + 1`
done
