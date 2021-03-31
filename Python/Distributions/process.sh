#!/bin/bash

datasetPath="/home/jefgrailet/Online repositories/SAGE/Dataset" # TODO: modify this !

if [ $# -ne 2 ] && [ $# -ne 3 ]; then
    echo "Usage: ./process.sh [ASes] [start date,end date] [[output file name]]"
    exit 1
fi

outputFileName=""
if [ $# -eq 3 ]; then
    outputFileName=${3}
fi

# Retrieves ASes
nbASes=$( cat ${1} | wc -l )
IFS=$'\r\n' GLOBIGNORE='*' command eval 'ASes=($(cat ${1}))'
IFS=$',' GLOBIGNORE='*' command eval 'limitDates=($(echo "${2}"))'

# Verifies dates, formats them and turns them into timestamps
if [ ${#limitDates[*]} -ne 2 ]; then
    echo "Please provide only two dates (start and end of a period) separated with a comma."
    echo "Usage: ./process.sh [ASes] [start and end dates as dd-mm-yyyy separated with a comma]"
    exit 1
fi

IFS=$'-' GLOBIGNORE='*' command eval 'startDate=($(echo "${limitDates[0]}"))'
IFS=$'-' GLOBIGNORE='*' command eval 'endDate=($(echo "${limitDates[1]}"))'

startDateStr=${startDate[2]}"-"${startDate[1]}"-"${startDate[0]}
endDateStr=${endDate[2]}"-"${endDate[1]}"-"${endDate[0]}

startTimestamp=$(date -u -d "$startDateStr 00:00" +%s)
endTimestamp=$(date -u -d "$endDateStr 00:00" +%s)

# Lists the dates for each AS between the provided dates
nbSnapshots=0
i=0
while [ $i -lt $nbASes ]; do
    # Checks there's a directory for this AS
    ASRoot=$datasetPath"/"${ASes[$i]}
    if ! [ -d "$ASRoot" ]; then
        echo "Error: there is no directory (and therefore no data) for ${ASes[$i]}."
        exit 1
    fi
    for year in "$ASRoot"/*/; do
        year=${year%*/} # Removes the / at the end
        year=${year##*/} # Removes the ./ at the beginning
        if [ $year -lt ${startDate[2]} ] || [ $year -gt ${endDate[2]} ]; then
            continue
        fi
        for month in "$ASRoot"/$year/*/; do
            month=${month%*/}
            month=${month##*/}
            for day in "$ASRoot"/$year/$month/*/; do
                day=${day%*/}
                day=${day##*/}
                curDateStr=$year"-"$month"-"$day
                curTime=$(date -u -d "$curDateStr 00:00" +%s)
                if [ $startTimestamp -lt $curTime ] && [ $curTime -lt $endTimestamp ]; then
                    snapshots[$nbSnapshots]=${ASes[$i]}"+"$day"-"$month"-"$year
                    nbSnapshots=`expr $nbSnapshots + 1`
                fi
            done
        done
    done
    i=`expr $i + 1`
done

if [ $nbSnapshots -eq 0 ]; then
    if [ ${#selectedDates} -gt 1 ]; then
        echo "Error: there are no snapshots for the provided period."
    else
        echo "Error: there are no snapshots for the selected ASes."
    fi
    exit 1
fi

# Generates the string to provide to feed to the Python script
snapshotsStr=""
i=0
while [ $i -lt $nbSnapshots ]; do
    if [ $i -gt 0 ]; then
        snapshotsStr=$snapshotsStr","
    fi
    snapshotsStr=$snapshotsStr${snapshots[$i]}
    i=`expr $i + 1`
done

# TODO: change the name of the script to get other distributions
python CyclesDistribution.py $snapshotsStr $outputFileName
