#!/bin/bash
# ASes_check.sh: prints out advancement of every node probing an AS.

if [ $# -ne 2 ] && [ $# -ne 1 ]; then
    echo "Usage: ./ASes_check.sh [file w/ PlanetLab nodes] [[rotation number]]"
    exit 1
fi

rotationNumber=0
if [ $# -eq 2 ]; then
    rotationNumber=${2}
fi

# Retrieves nodes
n_nodes=$( cat ${1} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval  'nodes=($(cat ${1}))'

# Checks that the rotation number is between 0 and n_nodes (excluded)
if [ $rotationNumber -gt $n_nodes ] || [ $rotationNumber -lt 0 ]; then
    echo "Rotation number must be comprised in [0, #nodes[."
    exit 1
fi

echo "Checking state of remote PlanetLab Nodes"

i=0
while [ $i -lt $n_nodes ]
do
    j=$((($i+$rotationNumber)%$n_nodes))
    echo "State of ${nodes[$j]}"
    commands="cd SAGE; ls -l;"
    ssh ulgple_lisp@${nodes[$j]} -i ~/.ssh/id_rsa -T $commands
    i=`expr $i + 1`
done

