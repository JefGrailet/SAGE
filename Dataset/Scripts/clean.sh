#!/bin/bash
# clean.sh: force deletes all pods or a specific one.

# Makes sure there's no more than one argument
if [ $# -gt 1 ]; then
    echo "Usage: ./clean.sh [[file w/ target ASes or ASN]]"
    exit 1
fi

# Namespace used with the kubectl command (TODO: change this !)
namespace=my-authority

# Today's date (used for logging)
today=`date +%d/%m/%Y`

# Checks there are ongoing measurements
nbFiles=$(ls ./Logs/Ongoing/ | wc -l)
if [ $nbFiles -eq 0 ]; then
    printf 'There is no ongoing measurement. Use schedule.sh to start new measurements.\n';
    exit 0
fi

# Gets the ASes known to have been probed lately. Using text files created in Logs/Ongoing/ is 
# motivated by the fact that a pod can be deleted by Kubernetes while running; therefore it can't 
# appear in the console output of kubectl while listing running pods. This strategy allows this 
# script to detect unexpected deletions of pods and logs this to avoid re-using the nodes where a 
# deletion happened for some time (because said node might be overloaded).

nbProbed=0
for filename in ./Logs/Ongoing/*.txt # Assumes, if directory not empty, that it contains .txt files
do
    probedAS=${filename:15:-4}
    if ! [[ $probedAS =~ ^AS([0-9]{1,5})$ ]]; then # Checks it's a proper AS name
        printf 'Warning: %s does not refer to a proper AS.\n' $filename
        continue
    elif ! [ -d "$probedAS" ]; then
        printf 'Warning: there is no %s among the target ASes (file: %s).\n' $probedAS $filename
        continue
    fi
    
    vantagePoint=$(cat "./Logs/Ongoing/"$probedAS".txt")
    vantagePoint="${vantagePoint//[$'\t\r\n ']}" # Removes miscellaneous characters
    VPs[$nbProbed]=$vantagePoint
    probedASes[$nbProbed]=$probedAS
    nbProbed=`expr $nbProbed + 1`
done

if [ $nbProbed -eq 0 ]; then
    printf 'There is no ongoing measurement. Use schedule.sh to start new measurements.\n';
    exit 0
fi

# Selects the pods to delete on the basis of the command-line argument:
# -if no argument, will delete all ongoing measurements (or logs their failure).
# -if the argument is a file, will only delete the listed ASes (format is assumed to be OK).
# -if the argument is a string, checks it's an ASN and will delete the associated pod (if any).
# -an error message will be printed if the script cannot make sense of the argument.

nbArgASes=0
if [ $# -eq 1 ]; then
    if [ -f "${1}" ]; then
        nbArgASes=$( cat ${1} | wc -l )
        IFS=$'\r\n' GLOBIGNORE='*' command eval  'argASes=($(cat ${1}))'
    elif [[ ${1} =~ ^AS([0-9]{1,5})$ ]]; then
        if ! [ -d "${1}" ]; then
            echo "${1} is not among the target ASes. Repository won't be checked."
            exit 1
        fi
        nbArgASes=1
        argASes[0]=${1}
    else
        echo "Argument is neither an existing file neither an AS number (prefix with \"AS\")."
        exit 1
    fi
fi

nbSelected=0
if [ $nbArgASes -eq 0 ]; then
    nbSelected=$nbProbed
    selectedASes=("${probedASes[@]}")
    fVPs=("${VPs[@]}")
else
    i=0
    while [ $i -lt $nbArgASes ]
    do
        curASN=${argASes[$i]}
        j=0
        while [ $j -lt $nbProbed ]
        do
            if [ ${probedASes[$j]} == $curASN ]; then
                selectedASes[$nbSelected]=${probedASes[$j]}
                fVPs[$nbSelected]=${VPs[$j]}
                nbSelected=`expr $nbSelected + 1`
                break
            fi
            j=`expr $j + 1`
        done
        i=`expr $i + 1`
    done
fi

if [ $nbSelected -eq 0 ]; then
    printf 'The selected AS(es) is/are not being mesured at the moment. Use schedule.sh to start new measurements.\n';
    exit 0
fi

# Force deletes (selected) pods when they still exist and logs it
i=0
while [ $i -lt $nbSelected ]
do
    podName="sage-snapshot-"${selectedASes[$i],,}
    
    columns="STATUS:.status.phase,REASON:.status.reason"
    podInfo=$(kubectl -n $namespace get pod $podName -o=custom-columns=$columns 2>&1)
    prefixPodInfo=${podInfo:0:5}
    if [ "$prefixPodInfo" == "Error" ]; then
        printf '%s - %s - Pod has been deleted (unknown reason).\n' $today ${fVPs[$i]} >> Logs/History/${selectedASes[$i]}.txt
        printf 'Pod for %s is no longer there; presumably prematurely deleted.\n' ${selectedASes[$i]}
        i=`expr $i + 1`
        continue
    fi
    
    kubectl -n $namespace delete pod $podName --grace-period=0 --force
    
    rm ./Pods/$podName.yaml
    rm ./Logs/Ongoing/${selectedASes[$i]}.txt
    
    printf '%s - %s - Running pod has been intentionally deleted.\n' $today ${fVPs[$i]} >> Logs/History/${selectedASes[$i]}.txt
    i=`expr $i + 1`
done
