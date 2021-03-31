#!/bin/bash

if [ $# -lt 1 ] || [ $# -gt 3 ]; then
    echo "Usage: ./process.sh [AS number] [[--dates=[dd-mm-yyyy separated with commas]]] [[--replace]]"
    exit 1
fi

# Modes for INSIGHT; no raw graph (i.e. visual of the neighborhood-based DAG) generated for now
nbModes=2
modes[0]="Bip"
modes[1]="BipProjTop"
withTextDump[0]=true
withTextDump[1]=false

# Deals with arguments
ASNumber=${1}
selectedDates=""
replacing=false
if [ $# -gt 1 ] && [ $# -le 3 ]; then
    for i in "$@"; do
        curArg=$i
        if [ ${curArg:0:2} == "AS" ]; then
            continue
        elif [ $curArg == "--replace" ]; then
            replacing=true
        elif [ ${curArg:0:8} == "--dates=" ]; then
            selectedDates=${curArg:8}
        fi
    done
fi

# Checks there's a directory for this AS
datasetPath="/home/jefgrailet/Online repositories/SAGE/Dataset" # TODO: change this !
ASRoot=$datasetPath"/"$ASNumber
if ! [ -d "$ASRoot" ]; then
    echo "Error: there is no directory (and therefore no data) for $ASNumber."
    exit 1
fi

# Lists the dates for this AS, crawling the directories if no specific date has been selected
nbDates=0
if [ ${#selectedDates} -gt 1 ]; then
    IFS=$',' GLOBIGNORE='*' command eval 'parsedDates=($(echo "$selectedDates"))'
    i=0
    nbParsedDates=${#parsedDates[@]}
    while [ $i -lt $nbParsedDates ]; do
        IFS=$'-' GLOBIGNORE='*' command eval 'datesPart=($(echo "${parsedDates[$i]}"))'
        if [ ${#datesPart[@]} -ne 3 ]; then
            echo "Error: bad format for date ${parsedDates[$i]}."
            i=`expr $i + 1`
            continue
        fi
        datesD[$i]=${datesPart[0]}
        datesM[$i]=${datesPart[1]}
        datesY[$i]=${datesPart[2]}
        if ! [ -d "$ASRoot"/"${datesY[0]}"/"${datesM[0]}"/"${datesD[0]}"/ ]; then
            echo "Error: no snapshot for the date ${datesD[0]}-${datesM[0]}-${datesY[0]}."
            i=`expr $i + 1`
            continue
        fi
        nbDates=`expr $nbDates + 1`
        i=`expr $i + 1`
    done
else
    for year in "$ASRoot"/*/; do
        year=${year%*/} # Removes the / at the end
        year=${year##*/} # Removes the ./ at the beginning
        for month in "$ASRoot"/$year/*/; do
            month=${month%*/}
            month=${month##*/}
            for day in "$ASRoot"/$year/$month/*/; do
                day=${day%*/}
                day=${day##*/}
                datesD[$nbDates]=$day
                datesM[$nbDates]=$month
                datesY[$nbDates]=$year
                nbDates=`expr $nbDates + 1`
            done
        done
    done
fi

if [ $nbDates -eq 0 ]; then
    if [ ${#selectedDates} -gt 1 ]; then
        echo "Error: none of the selected dates correspond to snapshots of $ASNumber."
    else
        echo "Error: there are no snapshots for $ASNumber."
    fi
    exit 1
fi

# Generates the results of INSIGHT for each date and for each mode; skips if results already exist
i=0
while [ $i -lt $nbDates ]; do
    # Checks if a folder for the date already exists
    folderExists=false
    if [ -d ./Results/$ASNumber/${datesY[$i]}/${datesM[$i]}/${datesD[$i]} ]; then
        folderExists=true
    fi
    
    # Skips this iteration if a folder exists, unless user added "--replace" to the command
    if ! $replacing && $folderExists; then
        dateStr=${datesD[$i]}"-"${datesM[$i]}"-"${datesY[$i]}
        echo "There are already results for $dateStr. Re-run with --replace to update them."
        i=`expr $i + 1`
        continue
    fi
    
    # Creates the folder for the results (if needed) and the prefix for its files
    fileNamePrefix=$ASNumber"_"${datesD[$i]}"-"${datesM[$i]}"-"${datesY[$i]}
    if ! $folderExists; then
        mkdir -p ./Results/$ASNumber/${datesY[$i]}/${datesM[$i]}/${datesD[$i]}
    fi
    
    snapshotPrefix=$ASRoot"/"${datesY[$i]}"/"${datesM[$i]}"/"${datesD[$i]}
    snapshotPrefix=$snapshotPrefix"/"$ASNumber"_"${datesD[$i]}"-"${datesM[$i]}
    dateStr=${datesD[$i]}"-"${datesM[$i]}"-"${datesY[$i]}
    echo "Processing snapshot for $ASNumber collected on $dateStr..."
    
    # Runs the different modes of INSIGHT on the selected snapshot
    j=0
    while [ $j -lt $nbModes ]; do
        outputFilePrefix=$fileNamePrefix"_"${modes[$j]}
        if ${withTextDump[$j]} ; then
            python INSIGHT.py ${modes[$j]} "$snapshotPrefix" $outputFilePrefix > $outputFilePrefix.txt
        else
            python INSIGHT.py ${modes[$j]} "$snapshotPrefix" $outputFilePrefix
        fi
        mv $outputFilePrefix* ./Results/$ASNumber/${datesY[$i]}/${datesM[$i]}/${datesD[$i]}/
        j=`expr $j + 1`
    done
    
    i=`expr $i + 1`
done
