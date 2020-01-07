#!/bin/bash
# ASes_start.sh: starts measurement of the target ASes.

if [ $# -ne 3 ] && [ $# -ne 2 ]; then
    echo "Usage: ./ASes_start.sh [file w/ target ASes] [file w/ PlanetLab nodes] [[rotation number]]"
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

echo "Starting measurements"
date=`date +%d-%m`

i=0
while [ $i -lt $n_nodes ]
do
    j=$((($i+$rotationNumber)%$n_nodes))
    echo "Contacting ${nodes[$j]} to start measurement of ${targets[$i]}..."
    command1="cd SAGE;"
    command2="sudo -S -b ./sage ${targets[$i]}.txt -l ${targets[$i]}_$date > ${targets[$i]}_$date.txt 2>&1 &"
    commands=$command1" "$command2
    ssh ulgple_lisp@${nodes[$j]} -i ~/.ssh/id_rsa -t $commands
    i=`expr $i + 1`
done

