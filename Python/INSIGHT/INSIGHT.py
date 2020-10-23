#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
INSIGHT - Investigate Networks from Subnet Inference to GrapH Transformations

Important remark: requires NetworkX to run (see here: https://pypi.org/project/networkx/)
'''

import os
import sys
import networkx as nx
import matplotlib.pyplot as plt

# Custom modules
import Subnets
import Neighborhoods
import Aliases
import GraphBuilding
import GraphDrawing
import FigureFactory

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Use this command: python INSIGHT.py [mode] [files prefix path] [output file name]")
        sys.exit()
    
    mode = str(sys.argv[1])
    filesPrefix = str(sys.argv[2])
    outputFileName = str(sys.argv[3])
    
    # Removes the extension from outputFileName (if any)
    if "." in outputFileName:
        splitName = outputFileName.split(".")
        outputFileName = splitName[0]
    
    # Checks if details must be printed in the terminal
    withDetails = False
    if "+" in mode:
        modeSplit = mode.split("+")
        mode = modeSplit[0]
        if len(modeSplit) == 2 and modeSplit[1].lower() == "details":
            withDetails = True
    mode.lower()
    
    # Paths of the relevant files (follows the order of file generation in SAGE)
    paths = []
    paths.append(filesPrefix + ".subnets")
    paths.append(filesPrefix + ".neighborhoods")
    paths.append(filesPrefix + ".graph")
    paths.append(filesPrefix + ".aliases-f")
    
    # Checks existence of each file
    for iPath in paths:
        if not os.path.isfile(iPath):
            print(iPath + " does not exist.")
            sys.exit()
    
    # Parses them as lines
    filesAsLines = []
    for i in range(0, len(paths)):
        with open(paths[i]) as f:
            fileRaw = f.read().splitlines()
        filesAsLines.append(fileRaw)
    
    # Parses subnets and neighborhoods and maps them together
    subnets, CIDRs = Subnets.parseSubnets(filesAsLines[0])
    neighborhoods = Neighborhoods.parseNeighborhoods(filesAsLines[1], subnets)
    for i in range(0, len(neighborhoods)):
        neighborhoods[i].mapPeers(neighborhoods)
    
    # Parses aliases and maps them to the corresponding neighborhoods
    aliases = Aliases.parseAliases(filesAsLines[3])
    Aliases.mapAliases(aliases, neighborhoods)
    
    # The "mode" is used to select what kind of graph the script will build and analyze.
    # Currently available modes are:
    # * Raw: builds a simple graph which the vertices model neighborhoods while the edges show 
    #   how these neighborhoods are located w.r.t. each others.
    # * Bip: builds a simple bipartite graph where the first party contains the neighborhoods 
    #   while the second party consists of the subnets that connect them. A variety of figures are 
    #   generated on the basis of this bipartite graph.
    
    if mode == "raw":
        # Creates a directed graph using the lines from the .graph file
        directed = GraphBuilding.directedGraph(filesAsLines[2])
        
        # Draws the graph
        GraphDrawing.drawDirectedGraph(directed, outputFileName)
    elif mode == "bip":
        # Creates a bipartite graph using the lines from the .graph file
        bip, subnetsVertices = GraphBuilding.bipartiteGraph(filesAsLines[2])
        bipComplete = GraphBuilding.completeBipartiteGraph(bip, neighborhoods, subnetsVertices)
        
        # General metrics on the graph
        print("--- About the graph ---")
        nodes = list(bip.nodes)
        nbEdges = len(list(bip.edges))
        if len(nodes) == 0:
            print("Graph doesn't contain any neighborhood. No further processing.")
            sys.exit()
        if nbEdges == 0:
            print("Graph doesn't contain any link (only neighborhoods). No further processing.")
            sys.exit()
        totalSubnets = 0
        totalHypothetic = 0
        totalKnownSubnets = 0
        for i in range(0, len(nodes)):
            if nodes[i].startswith("S") or nodes[i].startswith("T") or nodes[i].startswith("U"):
                totalSubnets += 1
            if nodes[i].startswith("T") or nodes[i].startswith("U"):
                totalHypothetic += 1
            if nodes[i].startswith("S"):
                totalKnownSubnets += 1
        ratioHypothetic = float(totalHypothetic) / float(len(nodes)) * 100
        ratioSubnets = float(totalSubnets) / float(len(nodes)) * 100
        ratioKnownSubnets = float(totalKnownSubnets) / float(totalSubnets) * 100
        print("Total of vertices (all kinds): " + str(len(nodes)))
        print("Hypothetic vertices: " + str(totalHypothetic) + " / " + str(len(nodes)) + " (" + str('%2f' % ratioHypothetic) + "%)")
        print("Subnet vertices: " + str(totalSubnets) + " / " + str(len(nodes)) + " (" + str('%2f' % ratioSubnets) + "%)")
        print("Known subnet vertices: " + str(totalKnownSubnets) + " / " + str(totalSubnets) + " (" + str('%2f' % ratioKnownSubnets) + "%)")
        
        # Gives the mappings for subnets acting as links (i.e. S_X => CIDR)
        print("\n--- Subnet mappings ---")
        listMappings = []
        for i in range(0, len(subnetsVertices)):
            listMappings.append(0)
        for subnet in subnetsVertices:
            listMappings[int(subnet[1:]) - 1] = subnetsVertices[subnet]
        listMappings.sort()
        for i in range(0, len(listMappings)):
            print("S" + str(i+1) + " = " + listMappings[i])
        
        # Degree distribution
        print("\n--- Degree analysis ---")
        outCDFs = outputFileName + "_degree_CDFs"
        outClustering = outputFileName + "_top_local_density"
        neighborhoodsByDegree, subnetsByDegree = FigureFactory.degreeCDFsBip(bipComplete, outCDFs)
        FigureFactory.topClusteringByDegreeBip(bipComplete, outClustering)
        maxTopDegree = 0
        maxBottomDegree = 0
        for topDegree in neighborhoodsByDegree:
            if topDegree > maxTopDegree:
                maxTopDegree = topDegree
        for bottomDegree in neighborhoodsByDegree:
            if bottomDegree > maxBottomDegree:
                maxBottomDegree = bottomDegree
        print("Maximum top (neighborhood) degree: " + str(maxTopDegree))
        print("Maximum bottom (subnet) degree: " + str(maxBottomDegree))
        
        # The subsequent instructions are only relevant if user asked for details
        if withDetails:
            print("\nDetails on top (neighborhood) degree:")
            for i in range(1, maxTopDegree + 1):
                if i not in neighborhoodsByDegree:
                    continue
                degreeStr = "Degree " + str(i) + ": "
                if len(neighborhoodsByDegree[i]) > 10:
                    degreeStr += str(len(neighborhoodsByDegree[i])) + " neighborhoods"
                else:
                    neighborhoodsToShow = neighborhoodsByDegree[i]
                    neighborhoodsToShow.sort()
                    for j in range(0, len(neighborhoodsToShow)):
                        if j > 0:
                            degreeStr += ", "
                        degreeStr += neighborhoodsToShow[j]
                print(degreeStr)
            print("\nDetails on bottom (subnet) degree:")
            for i in range(1, maxBottomDegree + 1):
                if i not in subnetsByDegree:
                    continue
                degreeStr = "Degree " + str(i) + ": "
                if len(subnetsByDegree[i]) > 10:
                    degreeStr += str(len(subnetsByDegree[i])) + " subnets"
                else:
                    subnetsToShow = subnetsByDegree[i]
                    subnetsToShow.sort()
                    for j in range(0, len(subnetsToShow)):
                        if j > 0:
                            degreeStr += ", "
                        subnet = subnetsVertices[subnetsToShow[j]]
                        degreeStr += subnet
                        if subnets[subnet].isSound():
                            degreeStr += " (sound)"
                print(degreeStr)
        
        # Cycles
        print("\n--- Base cycles ---")
        outCyclesPDF = outputFileName + "_cycles"
        cycles = FigureFactory.cyclesDistributionBip(bip, subnetsVertices, outCyclesPDF)
        
        # Prints cycles (if any)
        if cycles != None:
            for i in range(0, len(cycles)):
                cycleStr = ""
                nbHypotheticVertices = 0
                for j in range(0, len(cycles[i])):
                    if j > 0:
                        cycleStr += ", "
                    cycleStr += cycles[i][j]
                    if cycles[i][j].startswith("T"):
                        nbHypotheticVertices += 1
                # The subsequent instructions are only relevant if user asked for details
                if not withDetails:
                    print(cycleStr)
                    continue
                soundVertices = []
                bogusVertices = [] # I.e., degree is larger than the max amount of interfaces
                # Re-checks the cycle to check if the subnets vertices are sound
                for j in range(0, len(cycles[i])):
                    if not cycles[i][j].startswith("S"):
                        continue
                    curSubnet = subnets[subnetsVertices[cycles[i][j]]]
                    curSubnetDegree = len(bip.edges(cycles[i][j]))
                    if curSubnet.getMaxNbInterfaces() >= curSubnetDegree:
                        soundVertices.append(curSubnet.getCIDR())
                    else:
                        bogusVertices.append(curSubnet.getCIDR())
                if (len(bogusVertices) * 2) == len(cycles[i]):
                    cycleStr += "\nAll subnet vertices are bogus."
                elif len(bogusVertices) == 0:
                    cycleStr += "\nAll subnet vertices are sound."
                else:
                    if len(soundVertices) > 1:
                        cycleStr += "\nSound vertices: "
                        for j in range(0, len(soundVertices)):
                            if j > 0:
                                cycleStr += ", "
                            cycleStr += soundVertices[j]
                    elif len(soundVertices) == 1:
                        cycleStr += "\nSound vertex: " + soundVertices[0]
                    if len(bogusVertices) > 1:
                        cycleStr += "\nBogus vertices: "
                        for j in range(0, len(bogusVertices)):
                            if j > 0:
                                cycleStr += ", "
                            cycleStr += bogusVertices[j]
                    elif len(bogusVertices) == 1:
                        cycleStr += "\nBogus vertex: " + bogusVertices[0]
                    
                print(cycleStr)
        else:
            print("No cycle could be found.")
        
        # Draws the graph
        GraphDrawing.drawBipartiteGraph(bip, outputFileName + "_graph")
    else:
        print("Unknown mode (available modes: Raw, Bip).")
