#!/bin/bash
# prepare.sh: prepares the remote repository for measurements.

# Namespace used with the kubectl command (TODO: change this !)
namespace=my-authority

# Various variables used to access and handle the remote repository (TODO: change these !)
login="myusername"
repository="somerepository.accessible.with.ssh.org"
RSAKey="/path/to/a/RSA/key/to/log/to/repository/without/password/"
rootPath="/path/to/the/snapshots/folder/on/remote/repository/"
localRoot="/path/to/the/dataset/folder/in/your/computer/"

# List ASes on the basis of directories; uses a regex to distinguish AS folders from others.
# N.B.: based on https://stackoverflow.com/questions/2107945/how-to-loop-over-directories-in-linux
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

# Creates directories on remote repository if they don't exist yet (mkdir -p)
i=0
commandsStr="cd $rootPath "
while [ $i -lt $nbASes ]
do
    commandsStr+="; mkdir -p ${ASes[$i]}"
    i=`expr $i + 1`
done

ssh $login@$repository -o LogLevel=QUIET -i $RSAKey -t $commandsStr
echo "Created AS folders on remote repository (if they didn't exist yet)."

# Uploads the files "ASN.*" on the remote repository (.txt, .cfn, .req)
i=0
commandsStr="cd $rootPath "
while [ $i -lt $nbASes ]
do
    ASFilesPath=$localRoot"${ASes[$i]}/${ASes[$i]}.*"
    scp -i $RSAKey $ASFilesPath $login@$repository:$rootPath/${ASes[$i]}/
    i=`expr $i + 1`
done
