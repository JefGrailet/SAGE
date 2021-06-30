#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Compares snapshots collected by SAGE for a same target network from different vantage points 
# (and different dates) to check how similar the graphs are. Contrary to CompareGraphs.py, this 
# file only compares two graphs produced on two given dates but prints all details about the 
# redundant edges in the terminal. No figure is created here.

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
            secondSplit = nLabel.rsplit(" (", 1) # In case there are tags like "(B)"
            nLabel = secondSplit[0]
        if nLabel in res:
            print("WARNING! Duplicate neighborhood: " + nLabel + " (line " + str(i + 1) + ")")
            continue
        res[nLabel] = nID

    return res

def listClusterIPs(clusterLabel):
    labelIPs = clusterLabel[1:]
    splitLine = labelIPs.split("} (cluster")
    labelIPs = splitLine[0]
    listOfIPs = labelIPs.split(", ")
    return listOfIPs

def getClusters(neighborhoods):
    clusters = set()
    clusterIPs = set()
    
    for n in neighborhoods:
        if "cluster" not in n:
            continue
        clusters.add(n)
        listOfIPs = listClusterIPs(n)
        for i in range(0, len(listOfIPs)):
            clusterIPs.add(listOfIPs[i]) # Clusters are not supposed to overlap by design
    
    return clusters, clusterIPs

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
    
    # Path of the dataset (TODO: change this !)
    datasetPrefix = "/home/jefgrailet/Online repositories/SAGE/Dataset/" + ASNumber + "/"
    
    # Path of the graph used as the reference for the comparison (first snapshot of the campaign)
    refGraphPath = datasetPrefix + date1[2] + "/" + date1[1] + "/" + date1[0] + "/"
    refGraphPath += ASNumber + "_" + date1[0] + "-" + date1[1] + ".graph"
    refDate = date1[0] + "/" + date1[1] + "/" + date1[2]
    
    # Checks the file for the reference graph exists then parses it
    if not os.path.isfile(refGraphPath):
        print("Reference graph file does not exist (path: " + refGraphPath + ").")
        sys.exit()
    
    with open(refGraphPath) as f:
        refGraphLines = f.read().splitlines()
    refVertices = getNeighborhoods(refGraphLines)
    refClusters, refClusterIPs = getClusters(refVertices)
    refGraph = getAdjacencyMatrix(refGraphLines, refVertices)
    
    # Counts total amount of edges in reference graph
    allEdges = 0
    for i in range(len(refVertices) + 1, len(refGraphLines)):
        if not refGraphLines[i]:
            break
        allEdges += 1
    
    # Path of the snapshot which will be compared to the reference
    cmpGraphPath = datasetPrefix + date2[2] + "/" + date2[1] + "/" + date2[0] + "/"
    cmpGraphPath += ASNumber + "_" + date2[0] + "-" + date2[1] + ".graph"
    cmpDate = date2[0] + "/" + date2[1] + "/" + date2[2]
        
    # Checks the file exist then parses it
    if not os.path.isfile(cmpGraphPath):
        print("The second graph file does not exist (path: " + cmpGraphPath + ").")
        sys.exit()
    
    with open(cmpGraphPath) as f:
        cmpGraphLines = f.read().splitlines()
    cmpVertices = getNeighborhoods(cmpGraphLines)
    cmpClusters, cmpClusterIPs = getClusters(cmpVertices)
    cmpGraph = getAdjacencyMatrix(cmpGraphLines, cmpVertices)
    
    # Initializes the text of the summary of the comparison
    summary = "--- Comparaison between " + refDate + " and " + cmpDate + " ---\n"
    
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
    
    similarVertices = set()
    for cluster in refClusters:
        if cluster in commonVertices:
            continue
        thisClusterIPs = listClusterIPs(cluster)
        for i in range(0, len(thisClusterIPs)):
            if thisClusterIPs[i] in cmpClusterIPs:
                similarVertices.add(cluster)
                break
    
    ratioWithBestEffort = float(len(commonWithBestEffort)) / float(len(refVertices))
    ratioVertices = float(len(commonVertices)) / float(totalNonBestEffort)
    ratioSimilar = float(len(similarVertices)) / float(totalNonBestEffort)
    summary += "Common vertices (w/ best effort): " + str(len(commonWithBestEffort)) + " (" + str('%.2f' % (ratioWithBestEffort * 100)) + '%)\n'
    summary += "Common vertices (w/o best effort): " + str(len(commonVertices)) + " (" + str('%.2f' % (ratioVertices * 100)) + '%)\n'
    summary += "Similar vertices (non-identical clusters): " + str(len(similarVertices)) + " (" + str('%.2f' % (ratioSimilar * 100)) + '%)\n'
    
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
            if refGraph[ID1 - 1][j] == 1: # For each (in)direct link in the first graph
                ID3 = j + 1
                otherLabel = reverseDict1["N" + str(ID3)]
                if otherLabel in cmpVertices: # The next vertex also exists in the second graph
                    totalEdges += 1
                    ID4 = int(cmpVertices[otherLabel][1:])
                    if cmpGraph[ID2 - 1][ID4 - 1] == 1: # The link also exists in the second graph
                        # More detailed description of the edge than in CompareGraphs.py (to dump)
                        commonEdge = "N" + str(ID1) + " (" + reverseDict1["N" + str(ID1)] + ")"
                        commonEdge += " -> N" + str(ID3) + " (" + reverseDict1["N" + str(ID3)] + ")"
                        commonEdge += " <=> N" + str(ID2) + " (" + reverseDict2["N" + str(ID2)] + ")"
                        commonEdge += " -> N" + str(ID4) + " (" + reverseDict2["N" + str(ID4)] + ")"
                        if commonEdge not in commonEdges: # Just in case
                            commonEdges.add(commonEdge)
            elif refGraph[ID1 - 1][j] == 2: # Remote links are ignored (too many unknown hops)
                continue
    
    if totalEdges > 0:
        ratioEdges = float(len(commonEdges)) / float(totalEdges)
        ratioAllEdges = float(len(commonEdges)) / float(allEdges)
        summary += "Total of (in)direct links that can exist in both graphs:\t" + str(totalEdges) + "\n"
        summary += "Total of (in)direct links that exist in both graphs:\t\t" + str(len(commonEdges)) + "\t(" + str('%.2f' % (ratioEdges * 100)) + "%)\n"
        summary += "W.r.t. the total of edges in reference graph:\t\t" + str('%.2f' % (ratioAllEdges * 100)) + "%\n"
    else:
        summary += "There is no common edge between both graphs.\n"
    
    commonEdgesList = list(commonEdges)
    summaryEdges = ""
    if len(commonEdgesList) > 0:
        commonEdgesList.sort()
        summaryEdges += "--- Identical edges ---\n"
        for i in range(0, len(commonEdgesList)):
            summaryEdges += commonEdgesList[i] + "\n"
    
    if len(summaryEdges) > 0:
        print(summaryEdges)
    summary = summary[:-1] # Removes last \n
    print(summary)
