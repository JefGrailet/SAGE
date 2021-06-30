#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Compares a collection of snapshots collected by SAGE for a same target network from different 
# vantage points to check how similar the graphs are. The first snapshot of the collection is 
# compared to all subsequent snapshots, i.e. it is used as the reference snapshot. The results of 
# the comparisons are plotted in a figure that acts as a visual summary.

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

def getNeighborhoods(lines):
    res = dict()
    
    for i in range(0, len(lines)):
        if not lines[i]:
            break
        
        split = lines[i].split(" - ")
        nID = split[0]
        nLabel = split[1]
        if "not among targets" in nLabel:
            secondSplit = nLabel.rsplit(" (", 1) # In case there are tags like "(B)"
            nLabel = secondSplit[0]
        if nLabel in res:
            print("WARNING! Duplicate neighborhood: " + nLabel + " (line " + str(i + 1) + ")")
            continue
        res[nLabel] = nID

    return res

def getAdjacencyMatrix(lines, vertices):
    matrix = np.zeros(shape=(len(vertices), len(vertices)))
    
    # The "len(vertices) + 1" is to skip the part of the .graph listing vertices
    for i in range(len(vertices) + 1, len(lines)):
        if not lines[i]:
            break
        
        split = lines[i].split(' -> ')
        firstNode = split[0]
        secondNode = ""
        linkType = 1 # 1 = (in)direct link, 2 = remote link
        # Any line that doesn't contain " via " in the second part corresponds to a remote link
        if " via " in split[1]:
            secondSplit = split[1].split(' via ')
            secondNode = secondSplit[0]
        else:
            secondSplit = split[1].split()
            secondNode = secondSplit[0]
            linkType = 2
        
        firstNodeInt = int(firstNode[1:]) - 1
        secondNodeInt = int(secondNode[1:]) - 1
        matrix[firstNodeInt][secondNodeInt] = linkType
    
    return matrix

def getConvergencePointIPs(convPoint):
    convPointIPs = set()
    
    IPs = convPoint[1:]
    if "cluster" in convPoint:
        IPs = IPs[:-11]
    else:
        IPs = IPs[:-1]
    convIPs = IPs.split(", ")
    for convIP in convIPs:
        convPointIPs.add(convIP)
    
    return convPointIPs

if __name__ == "__main__":

    if len(sys.argv) != 3:
        print("Usage: python CompareGraphs.py [Target AS] [Dates]")
        sys.exit()
    
    ASNumber = str(sys.argv[1])
    datesFilePath = str(sys.argv[2])
    
    # Checks existence of the file providing dates
    if not os.path.isfile(datesFilePath):
        print(datesFilePath + " does not exist.")
        sys.exit()
    
    # Parses the dates file
    with open(datesFilePath) as f:
        datesRaw = f.read().splitlines()
    dates = []
    for i in range(0, len(datesRaw)):
        splitDate = datesRaw[i].split('/')
        dates.append(splitDate)
    
    # Path of the dataset (TODO: change this !)
    datasetPrefix = "/home/jefgrailet/Online repositories/SAGE/Dataset/" + ASNumber + "/"
    
    # Path of the graph used as the reference for the comparison (first snapshot of the campaign)
    refGraphPath = datasetPrefix + dates[0][2] + "/" + dates[0][1] + "/" + dates[0][0] + "/"
    refGraphPath += ASNumber + "_" + dates[0][0] + "-" + dates[0][1] + ".graph"
    refDate = dates[0][0] + "/" + dates[0][1] + "/" + dates[0][2]
    
    # Checks the file for the reference graph exists then parses it
    if not os.path.isfile(refGraphPath):
        print("Reference graph file does not exist (path: " + refGraphPath + ").")
        sys.exit()
    
    with open(refGraphPath) as f:
        refGraphLines = f.read().splitlines()
    refVertices = getNeighborhoods(refGraphLines)
    refGraph = getAdjacencyMatrix(refGraphLines, refVertices)
    
    # Set of vertices that are common to all snapshots (w/o best effort)
    commonToAll = set()
    refConvPoints = set()
    refConvPointIPs = set()
    for n in refVertices:
        if " | " in n or "echoing" in n:
            continue
        if "cluster" in n or ", " in n:
            refConvPoints.add(n)
            convIPs = getConvergencePointIPs(n)
            for convIP in convIPs:
                refConvPointIPs.add(convIP)
        commonToAll.add(n)
    totalNonBestEffort = len(commonToAll)
    
    # For each subsequent snapshot
    ratiosWithBestEffort = []
    ratiosCommonVertices = []
    ratiosCommonEdges = []
    for i in range(1, len(dates)):
        
        # Path of the snapshot
        cmpGraphPath = datasetPrefix + dates[i][2] + "/" + dates[i][1] + "/" + dates[i][0] + "/"
        cmpGraphPath += ASNumber + "_" + dates[i][0] + "-" + dates[i][1] + ".graph"
        cmpDate = dates[i][0] + "/" + dates[i][1] + "/" + dates[i][2]
        
        # Checks the file exist and parses it
        if not os.path.isfile(cmpGraphPath):
            print("The " + str(i+1) + "th graph file does not exist (path: " + cmpGraphPath + ").")
            sys.exit()
        
        with open(cmpGraphPath) as f:
            cmpGraphLines = f.read().splitlines()
        cmpVertices = getNeighborhoods(cmpGraphLines)
        cmpGraph = getAdjacencyMatrix(cmpGraphLines, cmpVertices)
        
        # Finds common/similar vertices
        commonWithBestEffort = set()
        commonVertices = set()
        totalNonBestEffort = 0
        for n in refVertices:
            if " | " not in n and "echoing" not in n: # Ignores "best effort" neighborhoods
                totalNonBestEffort += 1
                if n in cmpVertices:
                    commonVertices.add(n)
                    commonWithBestEffort.add(n)
            elif n in cmpVertices:
                commonWithBestEffort.add(n)
        
        commonConvPoints = 0
        commonConvPointIPs = 0
        for vertex in commonVertices:
            if ", " or "cluster" in vertex:
                if vertex in refConvPoints:
                    commonConvPoints += 1
                vIPs = getConvergencePointIPs(vertex)
                for vIP in vIPs:
                    if vIP in refConvPointIPs:
                        commonConvPointIPs += 1
        
        ratioWithBestEffort = float(len(commonWithBestEffort)) / float(len(refVertices))
        ratioVertices = float(len(commonVertices)) / float(totalNonBestEffort)
        ratioConvPoints = float(commonConvPoints) / float(len(refConvPoints))
        ratioConvPointIPs = float(commonConvPointIPs) / float(len(refConvPointIPs))
        
        print("--- Comparaison of snapshots for " + ASNumber + " (" + refDate + " and " + cmpDate + ") ---")
        print("Common vertices (w/ best effort): " + str(len(commonWithBestEffort)) + " (" + str('%.2f' % (ratioWithBestEffort * 100)) + '%)')
        print("Common vertices (w/o best effort): " + str(len(commonVertices)) + " (" + str('%.2f' % (ratioVertices * 100)) + '%)')
        print("Common convergence points: " + str(commonConvPoints) + " (" + str('%.2f' % (ratioConvPoints * 100)) + "%)")
        print("Common convergence point IPs: " + str(commonConvPointIPs) + " (" + str('%.2f' % (ratioConvPointIPs * 100)) + "%)")
        ratiosWithBestEffort.append(ratioWithBestEffort)
        ratiosCommonVertices.append(ratioVertices)
        
        # Removes from commonToAll any vertex that is NOT in cmpVertices
        toRemove = set()
        for n in commonToAll:
            if n not in cmpVertices:
                toRemove.add(n)
        for n in toRemove:
            commonToAll.remove(n)
        
        # Builds a reverse dict. of refVertices (to check easily if some N_X exists in the second graph)
        reverseDict = dict()
        for n in refVertices:
            reverseDict[refVertices[n]] = n
        
        commonEdges = set() # As strings, to simplify
        totalEdges = 0 # Direct/indirect links that can exist in both graphs
        for v in commonVertices:
            ID1 = int(refVertices[v][1:])
            ID2 = int(cmpVertices[v][1:])
            for j in range(0, refGraph.shape[1]):
                if refGraph[ID1 - 1][j] == 1: # For each (in)direct link in the first graph
                    ID3 = j + 1
                    otherLabel = reverseDict["N" + str(ID3)]
                    if otherLabel in cmpVertices: # The next vertex also exists in the second graph
                        totalEdges += 1
                        ID4 = int(cmpVertices[otherLabel][1:])
                        if cmpGraph[ID2 - 1][ID4 - 1] == 1: # The link also exists in the second graph
                            commonEdge = "N" + str(ID1) + " -> N" + str(ID3)
                            commonEdge += " <=> N" + str(ID2) + " -> N" + str(ID4)
                            if commonEdge not in commonEdges: # Just in case
                                commonEdges.add(commonEdge)
                elif refGraph[ID1 - 1][j] == 2: # Remote links are ignored (too many unknown hops)
                    continue
        
        if totalEdges > 0:
            ratioEdges = float(len(commonEdges)) / float(totalEdges)
            ratiosCommonEdges.append(ratioEdges)
            print("Total of (in)direct links that can exist in both graphs:\t" + str(totalEdges))
            print("Total of (in)direct links that exist in both graphs:\t\t" + str(len(commonEdges)) + "\t(" + str('%.2f' % (ratioEdges * 100)) + "%)")
        else:
            ratiosCommonEdges.append(0)
            print("There is no common edge between both graphs.")
    
    nbConvergencePoints = 0
    for vertex in commonToAll:
        if "(cluster)" in vertex or ", " in vertex:
            nbConvergencePoints += 1
    
    print("--- Summary for " + ASNumber + " ---")
    ratioCommonToAll = float(len(commonToAll)) / float(totalNonBestEffort)
    print("Vertices (w/o best effort) found in all snapshots: " + str(len(commonToAll)) + " (" + str('%.2f' % (ratioCommonToAll * 100)) + '%)')
    print("Ratios of common vertices (w.r.t. reference snapshot): " + str(ratiosCommonVertices))
    print("Ratios of common edges (w.r.t. reference snapshot): " + str(ratiosCommonEdges))
    print("Convergence points in intersection (all snapshots): " + str(nbConvergencePoints))
    
    # Sizes for the ticks and labels of the plot
    hfont = {'fontweight': 'bold', 'fontsize': 34}
    hfont2 = {'fontsize': 30}
    hfont3 = {'fontsize': 26}
    
    # Bar chart stuff
    ind = np.arange(len(ratiosCommonEdges))
    widthBar = 0.4
    padding = 0.2

    # Plots result
    plt.figure(figsize=(17,11))
    plt.rcParams.update({'font.family': 'Times New Roman'})
    
    xAxis = range(0, len(ratiosCommonEdges), 1)
    plt.plot(xAxis, ratiosCommonEdges, color='#000000', linewidth=3, marker='o', markersize=12, label="RER")
    plt.bar(ind - widthBar + padding, ratiosWithBestEffort, widthBar, color='#959595', label="RVR (w/ BE)")
    plt.bar(ind + padding, ratiosCommonVertices, widthBar, color='#555555', label="RVR")
    plt.axhline(y=ratioCommonToAll, linewidth=3, linestyle='--', color='#00CDFF', label="IR")
    
    # Limits
    plt.ylim([0, 1.05])
    plt.xlim([-0.5, len(ratiosCommonEdges) - 0.5])
    
    # Axes aesthetics
    axis = plt.gca()
    
    # Removes right and to axes
    axis.spines['right'].set_visible(False)
    axis.spines['top'].set_visible(False)
    
    # Ensures ticks on remaining axes
    axis.yaxis.set_ticks_position('left')
    axis.xaxis.set_ticks_position('bottom')
    
    # Slightly moves the axes (towards left and bottom)
    axis.spines['left'].set_position(('outward', 10))
    axis.spines['bottom'].set_position(('outward', 10))
    
    # Makes ticks larger and thicker
    axis.tick_params(direction='inout', length=6, width=3)
    
    # Ticks and labels
    plt.yticks(np.arange(0, 1.1, 0.1), **hfont2)
    yaxis = axis.get_yaxis()
    yticks = yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    
    ticksForX = []
    for i in range(1, len(dates)):
        ticksForX.append(dates[i][0] + "/" + dates[i][1])
    plt.xticks(xAxis, ticksForX, **hfont3)
    
    plt.ylabel('Redundancy ratio', **hfont)
    plt.xlabel('Snapshot (reference=' + dates[0][0] + '/' + dates[0][1] + ')', **hfont)
    
    plt.grid()
    
    plt.legend(bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=2,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.,
               fontsize=36)
    
    firstDataset = '-'.join(dates[0])
    lastDataset = '-'.join(dates[len(dates) - 1])
    figureFileName = ASNumber + ".pdf" # + "_" + firstDataset + "_to_" + lastDataset + ".pdf"
    plt.savefig(figureFileName)
    plt.clf()
    print("New figure saved in " + figureFileName + ".")
