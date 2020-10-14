#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Compares snapshots collected by SAGE for a same target network from different vantage points to 
# check how similar the graphs are. Contrary to CompareGraphs.py, this file only compares two 
# graphs produced on two given dates but prints all details about the redundant edges in the 
# terminal. No figure is produced here.

import os
import sys
import numpy as np

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

    if len(sys.argv) != 4:
        print("Usage: python CompareGraphPair.py [Target AS] [Date 1] [Date 2]")
        sys.exit()
    
    ASNumber = str(sys.argv[1])
    date1 = str(sys.argv[2]).split('/')
    date2 = str(sys.argv[3]).split('/')
    
    # Makes sure the dates have 3 components delimited by '/'
    if len(date1) != 3 or len(date2) != 3:
        print("Dates must be in dd/mm/yyyy format.")
        sys.exit()
    
    # TODO: change this !
    # Path of the dataset
    datasetPrefix = "/home/jefgrailet/Online repositories/SAGE/Dataset/" + ASNumber + "/"
    
    # Path of the graph used as the reference for the comparison (first snapshot of the campaign)
    refGraphPath = datasetPrefix + date1[2] + "/" + date1[1] + "/" + date1[0] + "/"
    refGraphPath += ASNumber + "_" + date1[0] + "-" + date1[1] + ".graph"
    refDate = date1[0] + "/" + date1[1] + "/" + date1[2]
    
    # Checks the file for the reference graph exists
    if not os.path.isfile(refGraphPath):
        print("Reference graph file does not exist (path: " + refGraphPath + ").")
        sys.exit()
    
    # Parses the reference .graph file
    with open(refGraphPath) as f:
        refGraphLines = f.read().splitlines()
    refVertices = getNeighborhoods(refGraphLines)
    refGraph = getAdjacencyMatrix(refGraphLines, refVertices)
    
    # Path of the snapshot which will be compared to the reference
    cmpGraphPath = datasetPrefix + date2[2] + "/" + date2[1] + "/" + date2[0] + "/"
    cmpGraphPath += ASNumber + "_" + date2[0] + "-" + date2[1] + ".graph"
    cmpDate = date2[0] + "/" + date2[1] + "/" + date2[2]
        
    # Checks the file exist
    if not os.path.isfile(cmpGraphPath):
        print("The second graph file does not exist (path: " + cmpGraphPath + ").")
        sys.exit()
    
    # Parses the .graph file
    with open(cmpGraphPath) as f:
        cmpGraphLines = f.read().splitlines()
    cmpVertices = getNeighborhoods(cmpGraphLines)
    cmpGraph = getAdjacencyMatrix(cmpGraphLines, cmpVertices)
    
    # Initializes the text of the summary of the comparison
    summary = "--- Comparaison between " + refDate + " and " + cmpDate + " ---\n"
    
    # Finds common vertices
    commonVertices = set()
    for n in refVertices:
        if n in cmpVertices:
            commonVertices.add(n)
    ratioVertices = (float(len(commonVertices)) / float(len(refVertices))) * 100
    summary += "Common vertices: " + str(len(commonVertices)) + " (" + str('%.2f' % ratioVertices) + '%)\n'
    
    # Builds a reverse dict. of the vertices
    reverseDict1 = dict()
    for n in refVertices:
        reverseDict1[refVertices[n]] = n
    reverseDict2 = dict()
    for n in cmpVertices:
        reverseDict2[cmpVertices[n]] = n
    
    commonEdges = set() # As strings, to simplify
    totalEdges = 0 # Direct/indirect links that can exist in both graphs
    for v in commonVertices:
        ID1 = int(refVertices[v][1:])
        ID2 = int(cmpVertices[v][1:])
        for j in range(0, refGraph.shape[1]):
            # For each (in)direct link in the first graph
            if refGraph[ID1 - 1][j] == 1:
                ID3 = j + 1
                otherLabel = reverseDict1["N" + str(ID3)]
                # The next neighborhood in the topology also exists in the second graph
                if otherLabel in cmpVertices:
                    totalEdges += 1
                    ID4 = int(cmpVertices[otherLabel][1:])
                    # The link present in the first graph also exists in the second graph
                    if cmpGraph[ID2 - 1][ID4 - 1] == 1:
                        # More detailed description of the edge than in CompareGraphs.py (to dump)
                        commonEdge = "N" + str(ID1) + " (" + reverseDict1["N" + str(ID1)] + ")"
                        commonEdge += " -> N" + str(ID3) + " (" + reverseDict1["N" + str(ID3)] + ")"
                        commonEdge += " <=> N" + str(ID2) + " (" + reverseDict2["N" + str(ID2)] + ")"
                        commonEdge += " -> N" + str(ID4) + " (" + reverseDict2["N" + str(ID4)] + ")"
                        if commonEdge not in commonEdges: # Just in case
                            commonEdges.add(commonEdge)
            # If it's a remote link, continue (remote links are more "volatile")
            elif refGraph[ID1 - 1][j] == 2:
                continue
    
    if totalEdges > 0:
        ratioEdges = (float(len(commonEdges)) / float(totalEdges)) * 100
        summary += "Total of (in)direct links that can exist in both graphs:\t" + str(totalEdges) + "\n"
        summary += "Total of (in)direct links that exist in both graphs:\t\t" + str(len(commonEdges)) + "\t(" + str('%.2f' % ratioEdges) + "%)\n"
    else:
        summary += "There is no common edge between both graphs.\n"
    
    commonEdgesList = list(commonEdges)
    if len(commonEdgesList) > 0:
        commonEdgesList.sort()
        summary += "--- Identical edges ---\n"
        for i in range(0, len(commonEdgesList)):
            summary += commonEdgesList[i] + "\n"
    
    summary = summary[:-1] # Removes last \n
    print(summary)
