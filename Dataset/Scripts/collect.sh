#!/bin/bash
# diagnostic.sh: checks the pods thoroughly without any further action (download, logging, etc.).

# Namespace used with the kubectl command (TODO: change this !)
namespace=my-authority

# Year of measurement (assumed to be the same as the start date of any measurement)
year=`date +%Y`

# Today's date (used for logging)
today=`date +%d/%m/%Y`

# Various variables used to access and handle the remote repository (TODO: change these !)
login="myusername"
repository="somerepository.accessible.with.ssh.org"
RSAKey="/path/to/a/RSA/key/to/log/to/repository/without/password/"
rootPath="/path/to/the/snapshots/folder/on/remote/repository/"
localRoot="/path/to/the/dataset/folder/in/your/computer/"

# Gets the ASes known to have been probed lately. Using text files created in Logs/Ongoing/ is 
# motivated by the fact that a pod can be deleted by Kubernetes while running; therefore it can't 
# appear in the console output of kubectl while listing running pods. This strategy allows this 
# script to detect unexpected deletions of pods and logs this to avoid re-using the nodes where a 
# problematic scenario happened for some time (because said node might be overloaded).

nbFiles=$(ls ./Logs/Ongoing/ | wc -l)
if [ $nbFiles -eq 0 ]; then
    printf 'There is no ongoing measurement. Use schedule.sh to start new measurements.\n';
    exit 0
fi

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

# Now, evaluates the situation of all current pods, i.e., pods that match the names of ASes that 
# are listed in ./Logs/Ongoing/ (see above). The code gathers many details to make sure all the 
# data that could be used in the future for better scheduling is recorded in ./Logs/History/. The 
# logs in said folder are read by Python/Schedule.py to renew the vantage points for each target 
# AS while avoiding nodes that have been overloaded lately.

i=0
while [ $i -lt $nbProbed ]
do
    # Lists the files stored for this target in the remote repository
    remoteCommands="cd "$rootPath"; ls -1 ./"${probedASes[$i]}";" # -1 for one item per line (each line ends with \n)
    remoteFiles=$(ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $remoteCommands) # No "connection closed" message
    IFS=$'\r\n' GLOBIGNORE='*' command eval  'filesList=($(echo "$remoteFiles"))'
    nbFiles=${#filesList[@]}
    
    # Finds length of the prefix
    lengthASName=${#probedASes[$i]}
    lengthTargetFile=`expr $lengthASName + 4` # ASN followed by ".txt"
    lengthSnapshotPrefix=`expr $lengthASName + 6` # '_' followed by date as dd-mm => 6 characters
    
    # Goes through the list of files, ignoring the target file
    j=0
    hasMetrics=false
    hasAliasesF=false
    while [ $j -lt $nbFiles ]
    do
        lengthFilename=${#filesList[$j]}
        if [ $lengthFilename -eq $lengthTargetFile ]; then
            j=`expr $j + 1`
            continue
        fi
        extension=${filesList[$j]:$lengthSnapshotPrefix}
        if [ $extension == ".metrics" ]; then
            hasMetrics=true
        elif [ $extension == ".aliases-f" ]; then
            hasAliasesF=true
        fi
        j=`expr $j + 1`
    done
    
    # If there are .metrics and .aliases-f output files, then the snapshot is complete.
    if $hasMetrics && $hasAliasesF ; then
        resultsPerAS[$i]="Success"
        podIssuePerAS[$i]="None"
    # Otherwise, will collect data from Kubernetes to evaluate the situation
    else
        columns="STATUS:.status.phase,REASON:.status.reason"
        podInfo=$(kubectl -n $namespace get pod sage-snapshot-${probedASes[$i],,} -o=custom-columns=$columns 2>&1)
        prefixPodInfo=${podInfo:0:5}
        # Error returned by Kubernetes: the pod has been deleted somehow
        if [ "$prefixPodInfo" == "Error" ]; then
            resultsPerAS[$i]="Killed"
            if [ $nbFiles -gt 1 ]; then
                podIssuePerAS[$i]=$nbFiles
            else
                podIssuePerAS[$i]="None"
            fi
        # Otherwise, inspects the status of the pod
        else
            IFS=$'\r\n' GLOBIGNORE='*' command eval  'splitTable=($(echo "$podInfo"))'
            secondLine=$(echo "${splitTable[1]}" | xargs echo -n)
            IFS=$' ' GLOBIGNORE='*' command eval  'diagnostic=($(echo "$secondLine"))'
            if [ ${diagnostic[0]} == "Running" ]; then
                resultsPerAS[$i]="Running"
                podIssuePerAS[$i]="None"
            elif [ ${diagnostic[0]} == "Pending" ]; then
                resultsPerAS[$i]="Pending"
                podIssuePerAS[$i]="None"
            elif [ ${diagnostic[0]} == "Succeeded" ]; then # Fake success (missing files)
                containerColumn="STATUS:.status.containerStatuses[0].state.terminated.reason"
                moreInfo=$(kubectl -n $namespace get pod sage-snapshot-${probedASes[$i],,} -o=custom-columns=$containerColumn)
                terminationReason=${moreInfo:7}
                if [ $terminationReason == "Completed" ] && [ $nbFiles -eq 2 ]; then # No IP replied.
                    resultsPerAS[$i]="NoReachability"
                    podIssuePerAS[$i]="None"
                else
                    resultsPerAS[$i]="KilledInAction"
                    if [ $terminationReason == "<none>" ]; then
                        nbOutputFiles=`expr $nbFiles - 1`
                        # Since we can't know what happened, we will log the amount of output files
                        podIssuePerAS[$i]="UnknownReason+"$nbOutputFiles
                    else
                        podIssuePerAS[$i]=$terminationReason
                    fi
                fi
            elif [ ${diagnostic[0]} == "Failed" ]; then
                resultsPerAS[$i]="Failure"
                podIssuePerAS[$i]=${diagnostic[1]}
            else
                resultsPerAS[$i]="Unknown"
                podIssuePerAS[$i]="None"
            fi
        fi
    fi
    
    i=`expr $i + 1`
done

# The code now actually collects the data (when available), cleans the remote repository as well 
# as the Logs/Ongoing/ folder, deletes the pods and logs the result. For pods that could not run 
# until completion, only the clean up work and the logging are performed.

i=0
nbEndedPods=0 # Amount of pods that were terminated, regardless of success
availableSnapshots=0 # Amount of snapshots that could be download
while [ $i -lt $nbProbed ]
do
    # Successful measurement: downloads the snapshot
    if [ ${resultsPerAS[$i]} == "Success" ]; then
        printf 'Downloading new snapshot for %s (from %s).\n' ${probedASes[$i]} ${VPs[$i]}
        
        # Reads the name of the .txt log to get back the date at which the measurement started
        commands="cd "$rootPath"/"${probedASes[$i]}"; ls "${probedASes[$i]}"_*.txt";
        snapshotName=$(ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t "$commands")
        # Source: https://stackoverflow.com/questions/19505227/integer-expression-expected-error-in-shell-script)
        snapshotName="${snapshotName//[$'\t\r\n ']}"
        lengthASName=${#probedASes[$i]}
        lengthASName=`expr $lengthASName + 1`
        date=${snapshotName:$lengthASName:5}
        IFS='-' read -r -a date_split <<< "$date"
        day=${date_split[0]}
        month=${date_split[1]}
        
        # Prepares the directory of the snapshot and downloads the output files
        mkdir -p ./${probedASes[$i]}/$year/$month/$day
        destFolder=$localRoot${probedASes[$i]}/$year/$month/$day
        scp -i $RSAKey -r $login@$repository:$rootPath/${probedASes[$i]}/${probedASes[$i]}_$date.* $destFolder/
        
        # Saves the vantage point of the snapshot
        printf '%s\n' ${VPs[$i]} > $destFolder/VP.txt
        
        # Cleans up the remote repository
        commands="cd "$rootPath"/"${probedASes[$i]}"; rm "${probedASes[$i]}"_*;"
        ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $commands
        
        # Deletes the pod
        finalStatus=$(kubectl -n $namespace get pod sage-snapshot-${probedASes[$i],,} -o=custom-columns=STATUS:.status.phase 2>&1)
        prefix=${finalStatus:0:5}
        if ! [ $prefix == "Error" ]; then
            finalStatus=${finalStatus:7:7} # Just to get "Running" or "Succeed"
            if [ $finalStatus == "Succeed" ]; then
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,}
            else
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,} --force --grace-period=0
            fi
        fi
        
        # Remark: it can happen a pod that successfully completed its measurements is still in a 
        # "Running" state at the end (this is a Kubernetes issue). This is why the script checks 
        # whether a measurement was completed or not based on the amount of files uploaded in the 
        # repository rather than by looking at the pod status, which is only consulted at the end 
        # to see whether it must be deleted by force or not (hence the code above).
        
        # Deletes the YAML used to create the pod as well as the file in the "ongoing" repository
        rm ./Pods/sage-snapshot-${probedASes[$i],,}.yaml
        rm ./Logs/Ongoing/${probedASes[$i]}.txt
        
        printf '%s - %s - Downloaded new snapshot (start date: %s).\n' $today ${VPs[$i]} $date >> Logs/History/${probedASes[$i]}.txt
        nbEndedPods=`expr $nbEndedPods + 1`
        availableSnapshots=`expr $availableSnapshots + 1`
    # Pod has been deleted; this is logged so the node can be avoided for some days
    elif [ ${resultsPerAS[$i]} == "Killed" ]; then
        if [ ${podIssuePerAS[$i]} == "None" ]; then
            printf '%s - %s - Pod has been deleted (unknown reason).\n' $today ${VPs[$i]} >> Logs/History/${probedASes[$i]}.txt
            printf 'Pod for %s is no longer there; presumably prematurely deleted.\n' ${probedASes[$i]}
        else
            printf '%s - %s - Pod has been deleted (unknown reason).\n' $today ${VPs[$i]} >> Logs/History/${probedASes[$i]}.txt
            printf 'Pod for %s is no longer there; presumably stopped during a measurement (output=%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
            
            # Cleans up the remote repository
            commands="cd "$rootPath"/"${probedASes[$i]}"; rm "${probedASes[$i]}"_*;"
            ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $commands
        fi
        rm ./Pods/sage-snapshot-${probedASes[$i],,}.yaml
        rm ./Logs/Ongoing/${probedASes[$i]}.txt
    # No message nor action for Running/Pending pods
    elif [ ${resultsPerAS[$i]} == "Running" ] || [ ${resultsPerAS[$i]} == "Pending" ]; then
        i=`expr $i + 1`
        continue
    # SAGE could not get a reply from any IP (reachability issue); some clean up is required
    elif [ ${resultsPerAS[$i]} == "NoReachability" ]; then
        printf '%s - %s - SAGE could not get a reply from any IP.\n' $today ${VPs[$i]} >> Logs/History/${probedASes[$i]}.txt
        printf 'Measurement of %s could not be performed (no IP replied during pre-scanning).\n' ${probedASes[$i]}
        
        # Cleans up the remote repository
        commands="cd "$rootPath"/"${probedASes[$i]}"; rm "${probedASes[$i]}"_*;"
        ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $commands
        
        # Deletes the pod (with or without forcing, depending on the status of the pod)
        finalStatus=$(kubectl -n $namespace get pod sage-snapshot-${probedASes[$i],,} -o=custom-columns=STATUS:.status.phase 2>&1)
        prefix=${finalStatus:0:5}
        if ! [ $prefix == "Error" ]; then
            finalStatus=${finalStatus:7:7} # Just to get "Running" or "Succeed"
            if [ $finalStatus == "Succeed" ]; then
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,}
            else
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,} --force --grace-period=0
            fi
        fi
        
        rm ./Pods/sage-snapshot-${probedASes[$i],,}.yaml
        rm ./Logs/Ongoing/${probedASes[$i]}.txt
        nbEndedPods=`expr $nbEndedPods + 1`
    # SAGE has been interrupted while running: in addition to logging, some clean up is required
    elif [ ${resultsPerAS[$i]} == "KilledInAction" ]; then
        issue=${podIssuePerAS[$i]:0:7}
        if [ $issue == "Unknown" ]; then
            nbOutputFiles=${podIssuePerAS[$i]:14}
            printf '%s - %s - SAGE has been interrupted while running (output: %s).\n' $today ${VPs[$i]} $nbOutputFiles >> Logs/History/${probedASes[$i]}.txt
            printf 'Measurement of %s has been prematurely terminated (unknown reason; output=%s).\n' ${probedASes[$i]} $nbOutputFiles
        else
            printf '%s - %s - SAGE has been interrupted while running (%s).\n' $today ${VPs[$i]} ${podIssuePerAS[$i]} >> Logs/History/${probedASes[$i]}.txt
            printf 'Measurement of %s has been prematurely terminated (%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
        fi
        
        # Cleans up the remote repository
        commands="cd "$rootPath"/"${probedASes[$i]}"; rm "${probedASes[$i]}"_*;"
        ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $commands
        
        # Deletes the pod (with or without forcing, depending on the status of the pod)
        finalStatus=$(kubectl -n $namespace get pod sage-snapshot-${probedASes[$i],,} -o=custom-columns=STATUS:.status.phase 2>&1)
        prefix=${finalStatus:0:5}
        if ! [ $prefix == "Error" ]; then
            finalStatus=${finalStatus:7:7} # Just to get "Running" or "Succeed"
            if [ $finalStatus == "Succeed" ]; then
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,}
            else
                kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,} --force --grace-period=0
            fi
        fi
        
        rm ./Pods/sage-snapshot-${probedASes[$i],,}.yaml
        rm ./Logs/Ongoing/${probedASes[$i]}.txt
        nbEndedPods=`expr $nbEndedPods + 1`
    # Failure while creating the pod (e.g.: OutOfMemory, OutOfCpu)
    elif [ ${resultsPerAS[$i]} == "Failure" ]; then
        printf '%s - %s - Pod could not be created (%s).\n' $today ${VPs[$i]} ${podIssuePerAS[$i]} >> Logs/History/${probedASes[$i]}.txt
        printf 'Measurement of %s has failed (%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
        
        # Deletes the pod (no need to check the status)
        kubectl -n $namespace delete pod sage-snapshot-${probedASes[$i],,}
        
        rm ./Pods/sage-snapshot-${probedASes[$i],,}.yaml
        rm ./Logs/Ongoing/${probedASes[$i]}.txt
        nbEndedPods=`expr $nbEndedPods + 1`
    # Unknown outcome. Script advertises it so user can check the pod manually.
    else
        printf 'The result of the pod measuring %s is unknown (may have been manually deleted).\n' ${probedASes[$i]}
    fi
    i=`expr $i + 1`
done

# Finally, gives a summary of the whole execution.

if [ $nbEndedPods -eq 0 ]; then
    printf 'No new data to download nor pods to terminate. Wait a bit.\n'
else
    if [ $availableSnapshots -gt 1 ]; then
        printf '%d new snapshots could be downloaded.\n' $availableSnapshots
    elif [ $availableSnapshots -eq 1 ]; then
        printf 'One new snapshot could be downloaded.\n'
    else
        printf 'No new snapshot could be downloaded.\n'
    fi

    if [ $nbEndedPods -eq 1 ]; then
        printf 'One pod has been terminated.\n'
    else
        printf '%d pods have been terminated.\n' $nbEndedPods
    fi
fi
