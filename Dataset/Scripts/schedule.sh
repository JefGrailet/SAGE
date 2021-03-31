#!/bin/bash
# schedule.sh: using a Python script, computes a scheduling and creates new pods on that basis.

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

# Lists each AS that can be scheduled (i.e. if Logs/Ongoing/[ASN].txt doesn't exist for now)
i=0
ASesStr=""
schedulableStr=""
while [ $i -lt $nbASes ]
do    
    ASesStr+=${ASes[$i]}$'\n'
    if ! [ -f "Logs/Ongoing/${ASes[$i]}.txt" ]; then
        schedulableStr+=${ASes[$i]}$'\n'
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

# Gets the schedule and makes sure there's something to schedule
schedule=$(python Python/Schedule.py tmp_ASes.txt tmp_schedulable.txt tmp_pods.txt tmp_nodes.txt $today)
prefix=${schedule:0:9}
if ! [ "$prefix" == "Schedule:" ]; then
    printf '%s\n' "$schedule"
    exit 0
fi

# Source: https://stackoverflow.com/questions/19505227/integer-expression-expected-error-in-shell-script)
schedule="${schedule//[$'\t\r\n ']}"

# Generates the YAML files and side files in the Logs/ directory.
schedule=${schedule:9}
IFS=';' read -ra newPods <<< "$schedule"
i=0
while [ $i -lt ${#newPods[@]} ]
do
    # Splits the string describing the schedule on "+" (format is ASN+node)
    IFS='+' read -ra splitSchedule <<< "${newPods[$i]}"
    
    # Checks if a history log exists for the target AS; creates one if missing.
    if ! [ -f "./Logs/History/"${splitSchedule[0]}".txt" ]; then
        printf 'Log started on %s.\n' `date +%d/%m/%Y` > Logs/History/${splitSchedule[0]}.txt
    fi
    
    # Builds the YAML file to create the pod (N.B.: BuildYAML.py uses side files, see comments)
    YAMLFileName="sage-snapshot-${splitSchedule[0],,}.yaml"
    YAMLFiles[$i]=$YAMLFileName
    python Python/BuildYAML.py ${splitSchedule[0]} ${splitSchedule[1]} $date > Pods/$YAMLFileName
    printf 'Created YAML file to probe %s from %s.\n' ${splitSchedule[0]} ${splitSchedule[1]}
    
    # Creates a file in Logs/Ongoing/ (beneficial to collect.sh; see this script for more details)
    printf '%s' ${splitSchedule[1]} > Logs/Ongoing/${splitSchedule[0]}.txt
    i=`expr $i + 1`
done

# Creates the pods
i=0
while [ $i -lt ${#YAMLFiles[@]} ]
do
    kubectl -n $namespace apply -f Pods/${YAMLFiles[$i]}
    i=`expr $i + 1`
done

# Deletes temporary files
rm tmp_*.txt
