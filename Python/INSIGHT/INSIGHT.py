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
import PostNeighborhoods # Tripartite graphs
import GraphBuilding
import GraphDrawing
import FigureFactory

if __name__ == "__main__":

    if len(sys.argv) != 4 and len(sys.argv) != 5:
        print("Use this command: python INSIGHT.py [mode] [files prefix path] [output file name] [[max to render]]")
        sys.exit()
    
    mode = str(sys.argv[1])
    filesPrefix = str(sys.argv[2])
    outputFileName = str(sys.argv[3])
    mode = mode.casefold()
    maxToRender = 0
    if len(sys.argv) == 5:
        maxToRender = int(sys.argv[4])
    
    # Removes the extension from outputFileName (if any)
    if "." in outputFileName:
        splitName = outputFileName.split(".")
        outputFileName = splitName[0]
    
    # Paths of the relevant files (follows the order of file generation in SAGE)
    paths = []
    paths.append(filesPrefix + ".subnets")
    paths.append(filesPrefix + ".neighborhoods")
    paths.append(filesPrefix + ".graph")
    paths.append(filesPrefix + ".aliases-f") # Relevant for tripartite
    
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
    
    # The "mode" is used to select what kind of graph the script will build and/or analyze.
    # Currently available modes are:
    # * Raw: builds a simple graph which the vertices model neighborhoods while the edges show 
    #   how these neighborhoods are located w.r.t. each others, using the raw neighborhood-based 
    #   DAG built by SAGE.
    # * Bip: builds a simple bipartite graph where the first party contains the neighborhoods 
    #   while the second party consists of the subnets that connect them. A variety of figures are 
    #   generated on the basis of this bipartite graph, which are completed by a visual render of 
    #   the final graph (unless the number of vertices exceed a provided limit).
    # * BipTopProj: builds a simple bipartite just like in "Bip" mode, then projects it on top 
    #   vertices (neighborhoods) and writes the result to an output text file.
    # * BipBotProj: builds a simple bipartite just like in "Bip" mode, then projects it on bottom 
    #   vertices (subnets) and writes the result to an output text file.
    # * Trip: builds a tripartite graph, i.e., where (virtual) Layer-2 equipment can be connected 
    #   with (inferred) routers and where the same routers can be connected with subnets as well. 
    #   For now, connections between Layer-2 equipment and subnets are ignored. This model is 
    #   designed to represent more realistically the key components of a network topology, as a 
    #   neighborhood (see Bip mode) can hide either a single router, either a mesh involving 
    #   Layer-2 equipment. In addition to a visual render of the graph (unless the number of 
    #   vertices exceed a provided limit), this mode outputs figures to evaluate the degree and 
    #   local density of routers.
    
    if mode == "raw":
        directed = GraphBuilding.directedGraph(filesAsLines[2])
        
        GraphDrawing.drawDirectedGraph(directed, outputFileName)
    elif mode == "bip":
        bip, subnetsVertices = GraphBuilding.bipartiteGraph(filesAsLines[2])
        bipComplete = GraphBuilding.completeBipartiteGraph(bip, neighborhoods, subnetsVertices)
        
        # Names of the output files
        outCDFs = outputFileName + "_degree_CDFs"
        outClustering = outputFileName + "_top_local_density"
        outCyclesPDF = outputFileName + "_cycles"
        outGraph = outputFileName + "_graph"
        
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
        
        print("\n--- Subnet mappings ---")
        listMappings = []
        for i in range(0, len(subnetsVertices)):
            listMappings.append(0)
        for subnet in subnetsVertices:
            listMappings[int(subnet[1:]) - 1] = subnetsVertices[subnet]
        for i in range(0, len(listMappings)):
            print("S" + str(i+1) + " = " + listMappings[i])
        
        neighborhoodsByDegree, subnetsByDegree = FigureFactory.degreeCDFsBip(bipComplete, outCDFs)
        FigureFactory.topClusteringByDegreeBip(bipComplete, outClustering)
        maxTopDegree = 0
        maxBottomDegree = 0
        for topDegree in neighborhoodsByDegree:
            if topDegree > maxTopDegree:
                maxTopDegree = topDegree
        for bottomDegree in subnetsByDegree:
            if bottomDegree > maxBottomDegree:
                maxBottomDegree = bottomDegree
        
        print("\n--- Top (neighborhood) degrees ---")
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
        print("\n--- Bottom (subnet) degrees ---")
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
                    if subnets[subnet].isSound() and subnets[subnet].getMaxNbInterfaces() >= i:
                        degreeStr += " (sound)"
            print(degreeStr)
        
        # Cycles
        print("\n--- Base cycles ---")
        cycles = FigureFactory.cyclesDistributionBip(bip, subnetsVertices, outCyclesPDF)
        
        if cycles != None:
            for i in range(0, len(cycles)):
                cycleStr = ""
                nbHypotheticVertices = 0
                for j in range(0, len(cycles[i])):
                    if j > 0:
                        cycleStr += ", "
                    if not cycles[i][j].startswith("S"):
                        cycleStr += cycles[i][j]
                        continue
                    curSubnet = subnets[subnetsVertices[cycles[i][j]]]
                    curSubnetDegree = len(bip.edges(cycles[i][j]))
                    if curSubnet.getMaxNbInterfaces() >= curSubnetDegree:
                        cycleStr += cycles[i][j]
                    else:
                        cycleStr += cycles[i][j] + " (B)"
                print(cycleStr)
        else:
            print("No cycle could be found.")
        
        # Rendering the graph
        nbVerticesToRender = len(list(bip.nodes()))
        if maxToRender != 0 and nbVerticesToRender > maxToRender:
            print("\nGraph to render has more vertices (= " + str(nbVerticesToRender) + ") than allowed (= " + str(maxToRender) + ").")
        else:
            GraphDrawing.drawBipartiteGraph(bip, outGraph)
    elif mode == "bipprojtop":
        bip, subnetsVertices = GraphBuilding.bipartiteGraph(filesAsLines[2])
        forProjs = GraphBuilding.formatForProjections(bip)
        
        topProj = GraphBuilding.bipTopProjection(forProjs, neighborhoods)
        topProjFile = open(outputFileName + ".txt", "w")
        topProjFile.write(GraphBuilding.projectionToText(topProj))
        topProjFile.close()
    elif mode == "bipprojbot":
        bip, subnetsVertices = GraphBuilding.bipartiteGraph(filesAsLines[2])
        bipComplete = GraphBuilding.completeBipartiteGraph(bip, neighborhoods, subnetsVertices)
        forProjs = GraphBuilding.formatForProjections(bipComplete)
        
        botProj = GraphBuilding.bipBotProjection(forProjs, subnetsVertices)
        botProjFile = open(outputFileName + ".txt", "w")
        botProjFile.write(GraphBuilding.projectionToText(botProj))
        botProjFile.close()
    elif mode == "trip":
        post, IDsToCIDRs, CIDRsToIDs = PostNeighborhoods.postProcess(neighborhoods)
        PostNeighborhoods.connectNeighborhoods(post, CIDRsToIDs, filesAsLines[2])
        PostNeighborhoods.outputTripartite(post, IDsToCIDRs, outputFileName)
        
        # Names of the output files
        outDegrees = outputFileName + "_router_degrees"
        outClustering = outputFileName + "_router_local_density"
        outGraph = outputFileName + "_graph"
        
        # Computes a figure to compare the different degrees
        degrees = FigureFactory.routerDegreeTrip(post, outDegrees)
        
        # Reviews the degrees
        maxDegrees = [0, 0, 0, 0] # 0 = exp, 1 = min, 2 = max, 3 = mix
        for i in range(0, 4):
            for curDegree in degrees[i]:
                if curDegree > maxDegrees[i]:
                    maxDegrees[i] = curDegree
        
        # router-exp
        print("--- Neighborhood degree (router-exp) ---")
        for i in range(1, maxDegrees[0] + 1):
            if i not in degrees[0]:
                continue
            degreeStr = "Degree " + str(i) + ": "
            if len(degrees[0][i]) > 10:
                degreeStr += str(len(degrees[0][i])) + " neighborhoods"
            else:
                neighborhoodsToShow = degrees[0][i]
                neighborhoodsToShow.sort()
                for j in range(0, len(neighborhoodsToShow)):
                    if j > 0:
                        degreeStr += ", "
                    degreeStr += "N" + str(neighborhoodsToShow[j])
            print(degreeStr)
        
        # router-min, router-max, router-mix
        labels = ["\n--- Router degree with default L2 (router-min) ---", 
                  "\n--- Router degree without L2 (router-max) ---", 
                  "\n--- Router degree with conditional L2 (router-mix) ---"]
        for i in range(1, 4):
            print(labels[i - 1])
            for j in range(1, maxDegrees[i] + 1):
                if j not in degrees[i]:
                    continue
                degreeStr = "Degree " + str(j) + ": "
                if len(degrees[i][j]) > 10:
                    degreeStr += str(len(degrees[i][j])) + " routers"
                else:
                    routersToShow = degrees[i][j]
                    routersToShow.sort()
                    for k in range(0, len(routersToShow)):
                        if k > 0:
                            degreeStr += ", "
                        degreeStr += routersToShow[k]
                print(degreeStr)
        
        tripComplete = GraphBuilding.tripartiteGraph(post)
        FigureFactory.routerClusteringTrip(tripComplete, outClustering)
        
        # Rendering the graph
        tripToRender = GraphBuilding.tripartiteGraph(post, True)
        nbVerticesToRender = len(list(tripToRender.nodes()))
        if maxToRender != 0 and nbVerticesToRender > maxToRender:
            print("\nGraph to render has more vertices (= " + str(nbVerticesToRender) + ") than allowed (= " + str(maxToRender) + ").")
        else:
            GraphDrawing.drawTripartiteGraph(tripToRender, outGraph)
        
    else:
        errMsg = "Unknown mode "
        errMsg += "\"" + mode + "\""
        errMsg += " (available modes: Raw, Bip, BipProjTop, BipProjBot, Trip)."
        print(errMsg)
