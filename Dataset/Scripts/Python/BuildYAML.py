#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Usage: python BuildYAML.py [Target AS] [Node] [Date]")
        sys.exit()
    
    # TODO: adapt the two following variables to your computer !
    ASFoldersRoot = "/home/jefgrailet/PhD/Campaigns/SAGE"
    templatePath = ASFoldersRoot + "/Pods/template.yaml"
    
    # Default pod resources
    defaultReqs = dict()
    defaultReqs["cpu"] = "250m"
    defaultReqs["memory"] = "256Mi"
    
    # Args aren't checked; normally provided by a shell script that uses strict naming conventions
    targetAS = str(sys.argv[1])
    node = str(sys.argv[2])
    date = str(sys.argv[3])
    
    # Checks for additional files (custom parameters as .cfn, custom pod requirements as .req)
    customConfig = False
    if os.path.isfile(ASFoldersRoot + "/" + targetAS + "/" + targetAS + ".cfn"):
        customConfig = True
    
    customReqs = False
    reqsFilePath = ASFoldersRoot + "/" + targetAS + "/" + targetAS + ".req"
    podReqs = dict() # tag -> value (e.g.: "cpu" => "250m")
    if os.path.isfile(reqsFilePath):
        with open(reqsFilePath) as f:
            reqsLines = f.read().splitlines()
        for i in range(0, len(reqsLines)):
            if ":" not in reqsLines[i]:
                continue
            splitLine = reqsLines[i].split(':')
            if len(splitLine) != 2:
                continue
            podReqs[splitLine[0]] = splitLine[1]
        if len(podReqs) > 0:
            customReqs = True
    
    # Gets the template
    if not os.path.isfile(templatePath):
        print("Error: missing template.yaml. Please add it to the same directory as this script.")
        sys.exit()

    with open(templatePath) as f:
        lines = f.read().splitlines()

    # Final lines are directly written in the console (will be flushed to a .yaml file)
    for i in range(0, len(lines)):
        if "TODO" in lines[i]:
            # Name of the pod ("sage-snapshot-" suffixed with the AS name)
            if "sage-snapshot" in lines[i]:
                print(lines[i][:-4] + targetAS.lower())
            # Node in use
            elif "nodeName" in lines[i]:
                print(lines[i][:-6] + "\"" + node + "\"")
            # Resources (N.B.: for now, only cpu/memory, and limits/requests are identical)
            elif "cpu:" in lines[i] or "memory:" in lines[i]:
                tag = "cpu"
                if "memory:" in lines[i]:
                    tag = "memory"
                finalLine = "        " # Indentation
                if customReqs:
                    finalLine += tag + ": \"" + podReqs[tag] + "\""
                else:
                    finalLine += tag + ": \"" + defaultReqs[tag] + "\""
                print(finalLine)
            # Commands to run in the pod (depend on both targetAS and date)
            elif lines[i].endswith("- TODO"):
                commands = []
                prefixArgs = "      - " # Indentation + prefix for multi-line args
                prefixLine = "        " # Indentation for subsequent lines
                prefixOutputFiles = targetAS + "_" + date # Prefix name of files produced by SAGE
                
                # Command to download the target file from the remote DB
                scpDown = "scp -o StrictHostKeyChecking=no -i /etc/secret-ssh-keys/ssh-privatekey"
                scpDown += " -r $(cat /etc/secret-db/login)@$(cat /etc/secret-db/IP):"
                scpDown += "$(cat /etc/secret-db/rootPath)" + targetAS + "/" + targetAS + ".*" # .txt, .cfn, .req 
                scpDown += " /home/SAGE/Release/"
                commands.append(scpDown)
                
                # Command to move to folder of the image containing the executable of SAGE
                commands.append("cd /home/SAGE/Release/")
                
                # Command to run SAGE
                runSAGE = "./sage -l " + prefixOutputFiles
                if customConfig:
                    runSAGE += " -c " + targetAS + ".cfn"
                runSAGE += " " + targetAS + ".txt"
                runSAGE += " > " + prefixOutputFiles + ".txt 2>&1"
                commands.append(runSAGE)
                
                # Command to upload the output files to the remote DB
                scpUp = "scp -i /etc/secret-ssh-keys/ssh-privatekey "
                scpUp += "/home/SAGE/Release/" + prefixOutputFiles + "* "
                scpUp += "$(cat /etc/secret-db/login)@$(cat /etc/secret-db/IP):"
                scpUp += "$(cat /etc/secret-db/rootPath)" + targetAS + "/"
                commands.append(scpUp)
                
                commandsStr = prefixArgs
                for j in range(0, len(commands)):
                    if j > 0:
                        commandsStr += prefixLine
                    commandsStr += commands[j]
                    if j < len(commands) - 1:
                        commandsStr += ";\n"
                print(commandsStr)
        else:
            print(lines[i])
