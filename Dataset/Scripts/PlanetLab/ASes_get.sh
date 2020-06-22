#!/bin/bash
# ASes_get.sh: gets measurements.

if [ $# -ne 3 ] && [ $# -ne 2 ]; then
    echo "Usage: ./ASes_get.sh [file w/ target ASes] [file w/ PlanetLab nodes] [[rotation number]]"
    exit 1
fi

rotationNumber=0
if [ $# -eq 3 ]; then
    rotationNumber=${3}
fi

# Retrieves ASes and nodes
n_ASes=$( cat ${1} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval  'targets=($(cat ${1}))'
n_nodes=$( cat ${2} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval  'nodes=($(cat ${2}))'

# Checks the amount of ASes and PlanetLab nodes match
if [ $n_ASes -ne $n_nodes ]; then
    echo "Amount of target ASes and amount of PlanetLab nodes don't match."
    exit 1
fi

# Checks that the rotation number is between 0 and n_nodes (excluded)
if [ $rotationNumber -gt $n_nodes ] || [ $rotationNumber -lt 0 ]; then
    echo "Rotation number must be comprised in [0, #nodes[."
    exit 1
fi

echo "Retrieving measurements"
echo ""

year=`date +%Y`

# To get the date dd-mm, quick check at the first remote host
echo "Finding date of measurements..."
echo ""

lengthASName=${#targets[0]}
lengthASName=`expr $lengthASName + 1`

command1="cd SAGE;"
command2="ls "${targets[0]}"_* > Date_ASes;"
command3="rm Date_ASes;"
commandsCreate=$command1" "$command2
commandsErase=$command1" "$command3

ssh ulgple_lisp@${nodes[$rotationNumber]} -i ~/.ssh/id_rsa -t $commandsCreate
scp -r ulgple_lisp@${nodes[$rotationNumber]}:/home/ulgple_lisp/SAGE/Date_ASes /home/jefgrailet/PhD/Campaigns/SAGE/
ssh ulgple_lisp@${nodes[$rotationNumber]} -i ~/.ssh/id_rsa -t $commandsErase

toParse=$(<./Date_ASes)
date=${toParse:$lengthASName:5}
rm ./Date_ASes

if [ -z "$date" ]; then
    echo "No date found. Stopping here."
    exit
fi

echo ""
echo "Date is "$date"."
echo ""

IFS='-' read -r -a date_split <<< "$date"
day=${date_split[0]}
month=${date_split[1]}

# Now actually retrieving measurements
i=0
sourceFolder=/home/ulgple_lisp/SAGE
while [ $i -lt $n_nodes ]
do
    j=$((($i+$rotationNumber)%$n_nodes))    
    echo "Getting and processing measurements of ${targets[$i]} from ${nodes[$j]}"
    mkdir -p ./${targets[$i]}/$year/$month/$day
    
    # TODO: don't forget to change this path to match your own filesystem
    destFolder=/home/jefgrailet/PhD/Campaigns/SAGE/${targets[$i]}/$year/$month/$day
    
    # Downloads the output files
    scp -r ulgple_lisp@${nodes[$j]}:$sourceFolder/${targets[$i]}_$date.* $destFolder/
    
    # Writes vantage point
    echo ${nodes[$j]} > VP.txt
    mv VP.txt $destFolder
    
    # Cleaning remote host
    echo "Cleaning remote PlanetLab node..."
    commands="cd SAGE; rm "${targets[$i]}"_*;"
    ssh ulgple_lisp@${nodes[$j]} -i ~/.ssh/id_rsa -t $commands
    
    i=`expr $i + 1`
done

