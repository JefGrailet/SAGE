#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
Module for building graphs, using various resources such as the .graph lines or the data parsed in 
other files produced by SAGE.
'''

import networkx as nx
import matplotlib.pyplot as plt

def directedGraph(graphFileLines):
    '''
    Function which builds a simple directed graph using the raw lines from a .graph file and 
    without using previously parsed data found in other files (e.g. .subnets, .neighborhoods).
    
    :param graphFileLines: The (raw) lines from a .graph file
    :return: A directed graph as built by NetworkX
    '''
    
    # Identifies the graph lines
    actualGraphLines = []
    blankSpaces = 0
    for i in range(0, len(graphFileLines)):
        if not graphFileLines[i]:
            blankSpaces += 1
            continue
        if blankSpaces == 1:
            actualGraphLines.append(graphFileLines[i])
    
    # Creates the edges (very simple due to the nature of this graph)
    formattedEdges = []
    remoteLinksCount = 1
    for i in range(0, len(actualGraphLines)):
        lineSplit = actualGraphLines[i].split(" ")
        if len(lineSplit) < 3 or lineSplit[1] != "->":
            continue
        # Remote link (is simplified in the graph a "R" vertex (R for Remote)
        if lineSplit[3] == "through":
            remoteLink = "R" + str(remoteLinksCount)
            remoteLinksCount += 1
            formattedEdges.append([lineSplit[0], remoteLink])
            formattedEdges.append([remoteLink, lineSplit[2]])
            continue
        formattedEdges.append([lineSplit[0], lineSplit[2]])
    
    # Finally, creates and returns the directed graph
    directed = nx.DiGraph()
    directed.add_edges_from(formattedEdges)
    return directed

def bipartiteGraph(graphFileLines):
    '''
    Function which builds a simple bipartite graph, using the raw lines from a .graph file and 
    without using previously parsed data found in other files (e.g. .subnets, .neighborhoods).
    
    :param graphFileLines: The (raw) lines from a .graph file
    :return: A directed graph as built by NetworkX, plus one dictionary providing the mappings 
             between the S_X vertices (subnets) and the corresponding subnets as CIDR's
    '''
    
    # Identifies the labels of neighborhoods and the graph lines
    neighborhoodsLabels = dict()
    actualGraphLines = []
    blankSpaces = 0
    for i in range(0, len(graphFileLines)):
        if not graphFileLines[i]:
            blankSpaces += 1
            continue
        if blankSpaces == 0:
            splitLine = graphFileLines[i].split(" - ")
            neighborhoodsLabels[splitLine[0]] = splitLine[1]
        elif blankSpaces == 1:
            actualGraphLines.append(graphFileLines[i])
    
    # Sets of vertices (actual neighborhoods/subnets)
    nodesNeighborhoods = set()
    nodesSubnets = set()
    subnetsDict = dict()
    subnetsReverse = dict()
    subnetIDCount = 1
    
    # Hypothetic subnets
    hypotSubnets = set()
    hypotSubnetCount = 1
    
    # Hypothetic subnets modeling remote links
    remoteLinks = set()
    remoteLinksCount = 1
    
    # N.B.: naming for subnets: S = real subnets, T = hypothetic, U = "unknown" / remote paths
    
    # Starts browsing the edges discovered by SAGE to format them to create the bipartite graph
    formattedEdges = []
    for i in range(0, len(actualGraphLines)):
        lineSplit = actualGraphLines[i].split(" ")
        
        # Badly formatted line
        if len(lineSplit) < 3 or lineSplit[1] != "->":
            continue
        
        if lineSplit[0] not in nodesNeighborhoods:
            nodesNeighborhoods.add(lineSplit[0])
        if lineSplit[2] not in nodesNeighborhoods:
            nodesNeighborhoods.add(lineSplit[2])
        
        subnetName = ""
        # Case of an unknown medium: creates an hypothetic subnet
        if lineSplit[3] == "(unknown":
            subnetName = "T" + str(hypotSubnetCount)
            hypotSubnets.add(subnetName)
            hypotSubnetCount += 1
        # Case of a remote link: creates a special hypotnary subnet
        elif lineSplit[3] == "through":
            subnetName = "U" + str(remoteLinksCount)
            remoteLinks.add(subnetName)
            remoteLinksCount += 1
        # Case of a known medium
        elif lineSplit[3] == "via":
            subnetCIDR = lineSplit[4]
            # To fix a minor issue with snapshots collected prior to Sept 2020
            if "Echo" in neighborhoodsLabels[lineSplit[2]]:
                subnetName = "T" + str(hypotSubnetCount)
                hypotSubnets.add(subnetName)
                hypotSubnetCount += 1
            elif subnetCIDR in subnetsReverse:
                subnetName = subnetsReverse[subnetCIDR]
            else:
                subnetName = "S" + str(subnetIDCount)
                subnetsDict[subnetName] = subnetCIDR
                subnetsReverse[subnetCIDR] = subnetName
                nodesSubnets.add(subnetName)
                subnetIDCount += 1
        # Not a valid line: skips to next iteration
        else:
            continue
        
        # Creates the edges
        formattedEdges.append([lineSplit[0], subnetName])
        formattedEdges.append([lineSplit[2], subnetName])
    
    # The sets are turned into ordered lists to be able to draw the same figure upon re-run
    listBip1_1 = list(nodesNeighborhoods)
    listBip2_1 = list(nodesSubnets)
    listBip2_2 = list(hypotSubnets)
    listBip2_3 = list(remoteLinks)
    listBip1_1.sort() # Comment any of these lines to see a different figure at each re-run
    listBip2_1.sort()
    listBip2_2.sort()
    listBip2_3.sort()
    
    # Edges are sorted too
    formattedEdges.sort()
    
    # Finally, creates the graph and adds all components to it
    bip = nx.Graph()
    bip.add_nodes_from(listBip1_1, bipartite=0)
    bip.add_nodes_from(listBip2_1, bipartite=1)
    bip.add_nodes_from(listBip2_2, bipartite=1)
    bip.add_nodes_from(listBip2_3, bipartite=1)
    bip.add_edges_from(formattedEdges)
    
    # Returns the graph and the subnets mappings
    return bip, subnetsDict

def completeBipartiteGraph(bipGraph, neighborhoods, subnetsVertices):
    '''
    Function which builds a copy of a bipartite graph previously built with bipartiteGraph() and 
    complements it with subnets have a degree of one. Indeed, such subnets are not visible in the 
    .graph file of a snapshot (which only lists subnets acting as links), but adding the 
    corresponding edges is necessary to compute some metrics.
    
    :param bipGraph:         The initial bipartite graph (as built by bipartiteGraph())
    :param neighborhoods:    The list of neighborhoods parsed in the same snapshot used to build 
                             bipGraph
    :param subnetsVertices:  The mappings between S_X vertices (in bip graph) and subnets as CIDR's
    :return:                 A copy of bipGraph that has been completed with degree-1 subnets
    '''
    
    # Creates a copy of the initial graph
    bipCopy = bipGraph.copy()
    
    # Lists all CIDR's mentioned in the subnetsVertices dictionary
    subnetsLinks = set()
    for sID in subnetsVertices:
        subnetsLinks.add(subnetsVertices[sID])
    
    # Adds degree-1 subnets
    subnetIDCount = len(subnetsVertices) + 1
    for i in range(0, len(neighborhoods)):
        subnets = neighborhoods[i].getSubnets()
        nID = "N" + str(i+1)
        for subnet in subnets:
            if subnet.getCIDR() not in subnetsLinks:
                sID = "S" + str(subnetIDCount)
                bipCopy.add_node(sID)
                bipCopy.add_edge(nID, sID)
                subnetIDCount += 1
    
    return bipCopy
