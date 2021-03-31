#!/bin/bash
# evaluate.sh: computes a scheduling without applying it to verify that the schedule is sound.

# Namespace used with the kubectl command (TODO: change this !)
namespace=my-authority

# Date for the measurements that might get started right now
date=`date +%d-%m`

# Today's date (used by Schedule.py - see below - to avoid problematic nodes for a while)
today=`date +%d/%m/%Y`

# First makes sure the "Logs" directory exists (plus subdirectories); creates it otherwise
if ! [ -d "./Logs" ]; then
    mkdir ./Logs
    mkdir ./Logs/History
    mkdir ./Logs/Ongoing
else
    if ! [ -d "./Logs/History" ]; then
        mkdir ./Logs/History
    fi
    if ! [ -d "./Logs/Ongoing" ]; then
        mkdir ./Logs/Ongoing
    fi
fi

# Source: https://stackoverflow.com/questions/2107945/how-to-loop-over-directories-in-linux
nbASes=0
for dir in ./*/
do
    dir=${dir%*/} # Removes the / at the end
    dir=${dir##*/} # Removes the ./ at the beginning
    if [[ $dir =~ ^AS([0-9]{1,5})$ ]]; then # Checks it's a proper AS name
        ASes[$nbASes]=$dir
        nbASes=`expr $nbASes + 1`
    fi
done

i=0
ASesStr=""
schedulableStr=""
while [ $i -lt $nbASes ]
do
    # Lists each AS that can be scheduled (i.e. if Logs/Ongoing/[ASN].txt doesn't exist for now)
    ASesStr+=${ASes[$i]}$'\n'
    if ! [ -f "Logs/Ongoing/${ASes[$i]}.txt" ]; then
        schedulableStr+=${ASes[$i]}$'\n'
    fi
    
    # Creates history file if it doesn't exist yet
    if ! [ -f "Logs/History/${ASes[$i]}.txt" ]; then
        printf 'Log started on %s.\n' $today > ./Logs/History/${ASes[$i]}.txt
    fi
    
    i=`expr $i + 1`
done

if [ ${#ASesStr} == 0 ]; then
    echo "There is no AS to deal with. Please add some AS folders (+ target files)."
    exit 0
fi

if [ ${#schedulableStr} == 0 ]; then
    echo "There is no AS to schedule: all target ASes are being measured now."
    exit 0
fi

# Various temporary files to feed to a Python script
printf '%s' "$ASesStr" > tmp_ASes.txt
printf '%s' "$schedulableStr" > tmp_schedulable.txt
kubectl -n $namespace get pods -o=custom-columns=NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName > tmp_pods.txt
kubectl -n $namespace get nodes > tmp_nodes.txt

# The last arg ("Yes") allows to run the Schedule.py script in debug mode
schedule=$(python Python/Schedule.py tmp_ASes.txt tmp_schedulable.txt tmp_pods.txt tmp_nodes.txt $today Yes)

# Prints the schedule, deletes the temporary files and stops
printf '%s\n' "$schedule"
rm tmp_*.txt
