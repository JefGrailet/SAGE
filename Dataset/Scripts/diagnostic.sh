#!/bin/bash
# diagnostic.sh: checks the pods thoroughly without any further action (download, logging, etc.).

# Namespace used with the kubectl command (TODO: change this !)
namespace=my-authority

# Year of measurement (assumed to be the same as the start date of any measurement)
year=`date +%Y`

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
# deletion happened for some time (because said node might be overloaded).

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
    remoteCommands="cd "$rootPath"; ls -1 ./"${probedASes[$i]}";"
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
    # Otherwise, will get data from Kubernetes to evaluate the situation
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

# Using the data collected above, the script now gives a diagnostic for all current pods (or 
# rather for all ASes for which a pod has been created lately). Since it's only a diagnostic, no 
# meaningful operation (e.g. downloading a snapshot, deleting pods that ran to completion or 
# failed, etc.) is performed here: this is the job of the collect.sh script. The whole purpose of 
# diagnostic.sh is to let the user evaluate the situation before changing the state of anything, 
# which can be useful to detect unexpected scenarii and study them before running collect.sh.

i=0
nbPodsToEnd=0 # Amount of pods that can be terminated, regardless of success
availableSnapshots=0 # Amount of snapshots that can be download
while [ $i -lt $nbProbed ]
do
    if [ ${resultsPerAS[$i]} == "Success" ]; then
        printf 'Success with %s!\n' ${probedASes[$i]}
        nbPodsToEnd=`expr $nbPodsToEnd + 1`
        availableSnapshots=`expr $availableSnapshots + 1`
    elif [ ${resultsPerAS[$i]} == "Killed" ]; then
        if [ ${podIssuePerAS[$i]} == "None" ]; then
            printf 'Pod for %s is no longer there; presumably prematurely deleted.\n' ${probedASes[$i]}
        else
            printf 'Pod for %s is no longer there; presumably deleted during a measurement (output=%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
        fi
    elif [ ${resultsPerAS[$i]} == "Running" ]; then
        printf 'Measurement of %s is ongoing.\n' ${probedASes[$i]}
    elif [ ${resultsPerAS[$i]} == "Pending" ]; then
        printf 'Measurement of %s is pending.\n' ${probedASes[$i]}
    elif [ ${resultsPerAS[$i]} == "NoReachability" ]; then
        printf 'Measurement of %s could not be performed (no IP replied during pre-scanning).\n' ${probedASes[$i]}
        nbPodsToEnd=`expr $nbPodsToEnd + 1`
    elif [ ${resultsPerAS[$i]} == "KilledInAction" ]; then
        issue=${podIssuePerAS[$i]:0:7}
        if [ $issue == "Unknown" ]; then
            nbOutputFiles=${podIssuePerAS[$i]:14}
            printf 'Measurement of %s has been prematurely terminated (unknown reason; output=%s).\n' ${probedASes[$i]} $nbOutputFiles
        else
            printf 'Measurement of %s has been prematurely terminated (%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
        fi
        nbPodsToEnd=`expr $nbPodsToEnd + 1`
    elif [ ${resultsPerAS[$i]} == "Failure" ]; then
        printf 'Measurement of %s has failed (%s).\n' ${probedASes[$i]} ${podIssuePerAS[$i]}
        nbPodsToEnd=`expr $nbPodsToEnd + 1`
    else
        printf 'The fate of the pod measuring %s is unknown. Check manually.\n' ${probedASes[$i]}
    fi
    i=`expr $i + 1`
done

# Finally, gives a summary of the whole execution.

if [ $nbPodsToEnd -eq 0 ]; then
    printf 'No new data to download nor pods to terminate. Wait a bit.\n'
else
    if [ $availableSnapshots -gt 1 ]; then
        printf '%d new snapshots can be downloaded.\n' $availableSnapshots
    elif [ $availableSnapshots -eq 1 ]; then
        printf 'One new snapshot can be downloaded.\n'
    else
        printf 'No new snapshot to download.\n'
    fi

    if [ $nbPodsToEnd -eq 1 ]; then
        printf 'One pod can be terminated.\n'
    else
        printf '%d pods can be terminated.\n' $nbPodsToEnd
    fi
fi
