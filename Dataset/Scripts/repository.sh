#!/bin/bash
# repository.sh: inspects the folders of the remote repository.

# Makes sure there's no more than one argument
if [ $# -gt 1 ]; then
    echo "Usage: ./repository.sh [[file w/ target ASes or ASN]]"
    exit 1
fi

# Checks the argument:
# -if no argument, the content of the repository for all ASes will be displayed.
# -if the argument is a file, will only print the content for the ASes listed in said file. 
#  Format of this file is assumed to be correct.
# -if the argument is a string, checks it's an ASN and will show the content for this specific AS.
# -an error message will be printed if the script cannot make sense of the argument.

nbASes=0
if [ $# -lt 1 ]; then
    # Based on https://stackoverflow.com/questions/2107945/how-to-loop-over-directories-in-linux
    nbASes=0
    for dir in ./*/
    do
        dir=${dir%*/} # Removes the / at the end
        dir=${dir##*/} # Removes the ./ at the beginning
        if [[ $dir =~ ^AS([0-9]{1,5})$ ]]; then # Checks it's a proper AS name
            targets[$nbASes]=$dir
            nbASes=`expr $nbASes + 1`
        fi
    done
elif [ -f "${1}" ]; then
    nbASes=$( cat ${1} | wc -l )
    IFS=$'\r\n' GLOBIGNORE='*' command eval  'targets=($(cat ${1}))'
elif [[ ${1} =~ ^AS([0-9]{1,5})$ ]]; then
    if ! [ -d "${1}" ]; then
        echo "${1} is not among the target ASes. Repository won't be checked."
        exit 1
    fi
    nbASes=1
    targets[0]=${1}
else
    echo "Argument is neither an existing file neither an AS number (prefix with \"AS\")."
    exit 1
fi

# Various variables used to access and handle the remote repository (TODO: change these !)
login="myusername"
repository="somerepository.accessible.with.ssh.org"
RSAKey="/path/to/a/RSA/key/to/log/to/repository/without/password/"
rootPath="/path/to/the/snapshots/folder/on/remote/repository/"
localRoot="/path/to/the/dataset/folder/in/your/computer/"

# Lists the content of each folder corresponding to the given ASes
i=0
commands="cd "$rootPath";"
while [ $i -lt $nbASes ]
do
    commands=$commands" ls -l ./"${targets[$i]}";"
    i=`expr $i + 1`
done
ssh $login@$repository -i $RSAKey -t $commands
