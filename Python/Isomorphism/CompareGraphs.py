#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Compares a collection of snapshots collected by SAGE for a same target network from different 
# vantage points to check how similar the graphs are. The first snapshot of the collection is 
# compared to all subsequent snapshots, i.e. it is used as the reference snapshot. The results of 
# the comparisons are plotted in a figure which acts as a visual summary.

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
            secondSplit = nLabel.split(" (")
            nLabel = secondSplit[0]
        if nLabel in res:
            print("WARNING! Duplicate neighborhood: " + nLabel + " (line " + str(i + 1) + ")")
            continue
        res[nLabel] = nID

    return res

def getAdjacencyMatrix(lines, vertices):
    matrix = np.zeros(shape=(len(vertices), len(vertices)))
    
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

def getDensity(matrix):
    dims = matrix.shape
    maxLinks = dims[0] * dims[1]
    
    nbLinks = 0
    for i in range(0, dims[0]):
        for j in range(0, dims[1]):
            nbLinks += matrix[i][j]
    
    return float(nbLinks) / float(maxLinks)

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
    
    # Path of the dataset
    datasetPrefix = "/home/jefgrailet/Online repositories/SAGE/Dataset/" + ASNumber + "/"
    
    # Path of the graph used as the reference for the comparison (first snapshot of the campaign)
    refGraphPath = datasetPrefix + dates[0][2] + "/" + dates[0][1] + "/" + dates[0][0] + "/"
    refGraphPath += ASNumber + "_" + dates[0][0] + "-" + dates[0][1] + ".graph"
    refDate = dates[0][0] + "/" + dates[0][1] + "/" + dates[0][2]
    
    # Checks the file for the reference graph exists
    if not os.path.isfile(refGraphPath):
        print("Reference graph file does not exist (path: " + refGraphPath + ").")
        sys.exit()
    
    # Parses the reference .graph file
    with open(refGraphPath) as f:
        refGraphLines = f.read().splitlines()
    refVertices = getNeighborhoods(refGraphLines)
    refGraph = getAdjacencyMatrix(refGraphLines, refVertices)
    
    # Set of vertices that are common to all snapshots
    commonToAll = set()
    for n in refVertices:
        commonToAll.add(n)
    
    # For each subsequent snapshot
    ratiosCommonVertices = []
    ratiosCommonEdges = []
    for i in range(1, len(dates)):
        
        # Path of the snapshot
        cmpGraphPath = datasetPrefix + dates[i][2] + "/" + dates[i][1] + "/" + dates[i][0] + "/"
        cmpGraphPath += ASNumber + "_" + dates[i][0] + "-" + dates[i][1] + ".graph"
        cmpDate = dates[i][0] + "/" + dates[i][1] + "/" + dates[i][2]
        
        # Checks the file exist
        if not os.path.isfile(cmpGraphPath):
            print("The " + str(i+1) + "th graph file does not exist (path: " + cmpGraphPath + ").")
            sys.exit()
        
        # Parses the .graph file
        with open(cmpGraphPath) as f:
            cmpGraphLines = f.read().splitlines()
        cmpVertices = getNeighborhoods(cmpGraphLines)
        cmpGraph = getAdjacencyMatrix(cmpGraphLines, cmpVertices)
        
        # Finds common vertices
        commonVertices = set()
        for n in refVertices:
            if n in cmpVertices:
                commonVertices.add(n)
        ratioVertices = (float(len(commonVertices)) / float(len(refVertices))) * 100
        
        print("--- Comparaison of snapshots for " + ASNumber + " (" + refDate + " and " + cmpDate + ") ---")
        print("Common vertices: " + str(len(commonVertices)) + " (" + str('%.2f' % ratioVertices) + '%)')
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
                # For each (in)direct link in the first graph
                if refGraph[ID1 - 1][j] == 1:
                    ID3 = j + 1
                    otherLabel = reverseDict["N" + str(ID3)]
                    # The next neighborhood in the topology also exists in the second graph
                    if otherLabel in cmpVertices:
                        totalEdges += 1
                        ID4 = int(cmpVertices[otherLabel][1:])
                        # The link present in the first graph also exists in the second graph
                        if cmpGraph[ID2 - 1][ID4 - 1] == 1:
                            commonEdge = "N" + str(ID1) + " -> N" + str(ID3)
                            commonEdge += " <=> N" + str(ID2) + " -> N" + str(ID4)
                            if commonEdge not in commonEdges: # Just in case
                                commonEdges.add(commonEdge)
                # If it's a remote link, continue (remote links are more "volatile")
                elif refGraph[ID1 - 1][j] == 2:
                    continue
        
        if totalEdges > 0:
            ratioEdges = (float(len(commonEdges)) / float(totalEdges)) * 100
            ratiosCommonEdges.append(ratioEdges)
            print("Total of (in)direct links that can exist in both graphs:\t" + str(totalEdges))
            print("Total of (in)direct links that exist in both graphs:\t\t" + str(len(commonEdges)) + "\t(" + str('%.2f' % ratioEdges) + "%)")
        else:
            ratiosCommonEdges.append(0)
            print("There is no common edge between both graphs.")
    
    print("--- Summary for " + ASNumber + " ---")
    ratioCommonToAll = (float(len(commonToAll)) / float(len(refVertices))) * 100
    print("Vertices found in all snapshots: " + str(len(commonToAll)) + " (" + str('%.2f' % ratioCommonToAll) + '%)')
    print("Ratios of common vertices (w.r.t. reference snapshot): " + str(ratiosCommonVertices))
    print("Ratios of common edges (w.r.t. reference snapshot): " + str(ratiosCommonEdges))
    
    # Sizes for the ticks and labels of the plot
    hfont = {'fontweight': 'bold', 'fontsize': 28}
    hfont2 = {'fontsize': 26}
    hfont3 = {'fontsize': 22}
    
    # Bar chart stuff
    ind = np.arange(len(ratiosCommonEdges))
    widthBar = 0.7

    # Plots result
    plt.figure(figsize=(13,9))
    plt.rcParams.update({'font.family': 'Times New Roman'})
    
    xAxis = range(0, len(ratiosCommonEdges), 1)
    plt.plot(xAxis, ratiosCommonEdges, color='#000000', linewidth=3, marker='o', label="Redundant edges")
    plt.bar(ind, ratiosCommonVertices, widthBar, color='#CCCCCC', edgecolor='#000000', linewidth=2, label="Common vertices")
    plt.axhline(y=ratioCommonToAll, linewidth=4, linestyle='--', color='#0000FF', label="Intersection")
    
    # Limits
    plt.ylim([0, 105])
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
    plt.yticks(np.arange(0, 110, 10), **hfont2)
    yaxis = axis.get_yaxis()
    yticks = yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    
    ticksForX = []
    for i in range(1, len(dates)):
        ticksForX.append(dates[i][0] + "/" + dates[i][1])
    plt.xticks(xAxis, ticksForX, **hfont3)
    
    plt.ylabel('Ratio (%)', **hfont)
    plt.xlabel('Snapshot (reference=' + dates[0][0] + '/' + dates[0][1] + ')', **hfont)
    
    plt.grid()
    
    plt.legend(bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=2,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.,
               fontsize=24)
    
    firstDataset = '-'.join(dates[0])
    lastDataset = '-'.join(dates[len(dates) - 1])
    figureFileName = ASNumber + ".pdf" # + "_" + firstDataset + "_to_" + lastDataset + ".pdf"
    plt.savefig(figureFileName)
    plt.clf()
    print("New figure saved in " + figureFileName + ".")
