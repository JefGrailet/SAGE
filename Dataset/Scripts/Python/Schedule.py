#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
from datetime import datetime

# From: https://stackoverflow.com/questions/8419564/difference-between-two-dates-in-python

def differenceInDays(d1, d2):
    d1 = datetime.strptime(d1, "%d/%m/%Y")
    d2 = datetime.strptime(d2, "%d/%m/%Y")
    return abs((d2 - d1).days)

if __name__ == "__main__":

    if len(sys.argv) < 6 or len(sys.argv) > 7:
        print("Usage: python Schedule.py [ASes] [Schedulable ASes] [Pods] [Nodes] [Today as dd/mm/YYYY] [[Debug ? (Yes)]]")
        print("Remark: all args must be paths to text files with the requested content, except for the date.")
        sys.exit()
    
    # Not supposed to change: this script is designed for it (TODO: change this !)
    logsPath = "/home/jefgrailet/PhD/Campaigns/SAGE/Logs/History/"
    
    # List of EdgeNet nodes known to be problematic (if existing)
    avoidPath = logsPath + "NodesToAvoid.txt"
    nodesToAvoid = set()
    if os.path.isfile(avoidPath):
        with open(avoidPath) as f:
            listOfNodes = f.read().splitlines()
        for i in range(0, len(listOfNodes)):
            nodesToAvoid.add(listOfNodes[i])
    
    ASesPath = str(sys.argv[1])
    schedulablePath = str(sys.argv[2])
    podsPath = str(sys.argv[3])
    nodesPath = str(sys.argv[4])
    today = str(sys.argv[5])
    debug = False
    if len(sys.argv) == 7 and sys.argv[6].lower() == 'yes':
        debug = True
    
    # Gets the full list of ASes (one AS per line in the file); stops if no AS
    if not os.path.isfile(ASesPath):
        print("Error: input file for ASes \"" + ASesPath + "\" is missing.")
        sys.exit()

    with open(ASesPath) as f:
        ASes = f.read().splitlines()
    
    if len(ASes) == 0:
        print("There is no AS at the moment.")
        sys.exit()
    
    # First step of the scheduling: review all logs to get history and detect nodes to avoid
    overloadedNodes = set()
    alreadyProbed = set()
    snapshotHistory = []
    for i in range(0, len(ASes)):
        logPath = logsPath + ASes[i] + ".txt"
        if not os.path.isfile(logPath): # No log yet: skips to next iteration
            snapshotHistory.append(dict()) # Empty dictionary
            continue
        
        with open(logPath) as f:
            logLines = f.read().splitlines()
        
        snapshots = dict() # Mapping: node => amount of snapshots from this node
        for j in range(1, len(logLines)):
            lineSplit = logLines[j].split(' - ')
            date = lineSplit[0]
            node = lineSplit[1]
            diagnostic = lineSplit[2]
            diffDays = differenceInDays(date, today)
            # Log advertises a snapshot: adds to history; checks also if snapshot is from today
            if diagnostic.startswith("Downloaded new snapshot"):
                if node not in snapshots:
                    snapshots[node] = 1
                else:
                    snapshots[node] += 1
                findStartDate = diagnostic.split("start date: ")
                startDateRaw = findStartDate[1][:-2]
                startDateComp = startDateRaw.split('-')
                endDateComp = date.split('/')
                startedToday = False
                if diffDays == 0:
                    if startDateComp[0] == endDateComp[0] and startDateComp[1] == endDateComp[1]:
                        startedToday = True
                if startedToday and ASes[i] not in alreadyProbed:
                    alreadyProbed.add(ASes[i])
            # A node is avoided if it got a ressource issue less than 3 days ago
            if diffDays <= 3:
                nodeOverloaded = False
                if diagnostic.startswith("SAGE has been killed while running"):
                    nodeOverloaded = True
                elif diagnostic.startswith("Pod could not be created"):
                    nodeOverloaded = True
                if nodeOverloaded and node not in overloadedNodes:
                    overloadedNodes.add(node)
        snapshotHistory.append(snapshots)
    
    if debug:
        print("--- Already probed ---\n" + str(alreadyProbed))
        print("--- Nodes to avoid ---\n" + str(overloadedNodes))
        print("--- Detailed history ---")
        for i in range(0, len(ASes)):
            print(ASes[i] + " -> " + str(snapshotHistory[i]))
    
    # Gets the schedulable ASes (same format as previously); stops if no schedulable AS
    if not os.path.isfile(schedulablePath):
        print("Error: input file for schedulable ASes \"" + schedulablePath + "\" is missing.")
        sys.exit()

    with open(schedulablePath) as f:
        schedulableLines = f.read().splitlines()
    
    # Produces a second list, which omits ASes that are in the "alreadyProbed" set
    schedulable = set()
    for schedulableAS in schedulableLines:
        if schedulableAS not in alreadyProbed:
            schedulable.add(schedulableAS)
    
    if len(schedulable) == 0:
        print("There is no schedulable AS at the moment.")
        sys.exit()
    
    # Parses the current pods
    if not os.path.isfile(podsPath):
        print("Error: input file for pods \"" + podsPath + "\" is missing.")
        sys.exit()
    
    with open(podsPath) as f:
        podsLines = f.read().splitlines()
    
    nodesInUse = set()
    pods = [] # List of tuples [AS, node] (N.B.: could be removed ?)
    if len(podsLines) > 1:
        for i in range(1, len(podsLines)):
            splitLine = podsLines[i].split()
            if len(splitLine) != 3: # Just in case
                continue
            targetAS = splitLine[0][14:].upper()
            node = splitLine[2]
            pods.append([targetAS, node])
            nodesInUse.add(node)
    
    # Parses the nodes and put in a set nodes that are: Ready, not used for now and not overloaded
    if not os.path.isfile(nodesPath):
        print("Error: input file for pods \"" + nodesPath + "\" is missing.")
        sys.exit()
    
    with open(nodesPath) as f:
        nodesLines = f.read().splitlines()
    
    if len(nodesLines) <= 1:
        print("Warning ! The nodes file appear to list no nodes. Please verify.")
        sys.exit()
    
    readyNodes = set()
    allNodes = []
    for i in range(1, len(nodesLines)):
        splitLine = nodesLines[i].split() # By default, separator is blank spaces
        nodeName = splitLine[0]
        allNodes.append(nodeName)
        status = splitLine[1]
        if "," not in status and status == "Ready":
            readyNodes.add(nodeName)
    if len(nodesToAvoid) > 0:
        readyNodes = readyNodes - nodesToAvoid
    readyNodes = readyNodes - overloadedNodes
    readyNodes = readyNodes - nodesInUse
    
    if len(readyNodes) == 0:
        print("There are currently no node that can be used.")
    
    # Finds a new node to probe each AS that isn't being probed at the moment
    newSchedule = []
    for i in range(0, len(ASes)):
        # No more Ready nodes: stops looping
        if len(readyNodes) == 0:
            break
        
        # AS is no schedulable at the moment
        if ASes[i] not in schedulable:
            continue
        
        # Finds the Ready node with the lowest count of snapshots yet for this specific AS
        snapshots = snapshotHistory[i]
        selectedNode = ""
        minSnapshotCount = sys.maxsize
        for readyNode in readyNodes:
            if readyNode not in snapshots:
                selectedNode = readyNode
                minSnapshotCount = 0
            else:
                snapshotCount = snapshots[readyNode]
                if snapshotCount < minSnapshotCount:
                    selectedNode = readyNode
                    minSnapshotCount = snapshotCount
        
        # Shouldn't happen, because this means there are no Ready nodes any more (already checked)
        if minSnapshotCount == sys.maxsize:
            break
        
        # Saves the new schedule, and removes the picked node from the Ready nodes
        readyNodes.remove(selectedNode)
        newSchedule.append([ASes[i], selectedNode])
    
    # Prints the final schedule
    finalStr = ""
    if len(newSchedule) > 0:
        finalStr = "Schedule:"
        if debug:
            print("--- Detailed schedule ---")
        for newPod in newSchedule:
            if len(finalStr) > 9:
                finalStr += ";"
            if debug:
                print(newPod[0] + " -> " + newPod[1])
            finalStr += newPod[0] + "+" + newPod[1]
    else:
        finalStr = "No new schedule."
    
    if debug:
        print("--- Final output ---")
    print(finalStr)
