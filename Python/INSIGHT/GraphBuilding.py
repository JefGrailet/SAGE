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
    :return: A directed graph as built with NetworkX
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
    :return: A bipartite graph as built with NetworkX, plus one dictionary providing the mappings 
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
        thirdParty = ""
        # Case of an unknown medium: creates an hypothetic subnet
        if lineSplit[3] == "(unknown":
            subnetName = "T" + str(hypotSubnetCount)
            hypotSubnets.add(subnetName)
            hypotSubnetCount += 1
        # Case of a remote link: creates a special subnet vertex
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
                if len(lineSplit) == 7: # "(from ...)"
                    thirdParty = lineSplit[6][:-1]
            else:
                subnetName = "S" + str(subnetIDCount)
                subnetsDict[subnetName] = subnetCIDR
                subnetsReverse[subnetCIDR] = subnetName
                nodesSubnets.add(subnetName)
                subnetIDCount += 1
                if len(lineSplit) == 7: # "(from ...)"
                    thirdParty = lineSplit[6][:-1]
        # Not a valid line: skips to next iteration
        else:
            continue
        
        # Creates the edges
        formattedEdges.append([lineSplit[0], subnetName])
        formattedEdges.append([lineSplit[2], subnetName])
        if len(thirdParty) > 0:
            formattedEdges.append([thirdParty, subnetName])
    
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
    complements it with subnets that have a degree of one. Indeed, such subnets are not visible in 
    the .graph file of a snapshot (which only lists subnets acting as links), but adding the 
    corresponding edges is necessary to compute some metrics.
    
    :param bipGraph:         The initial bipartite graph (as built by bipartiteGraph())
    :param neighborhoods:    The list of neighborhoods parsed in the same snapshot used to build 
                             bipGraph
    :param subnetsVertices:  The mappings between S_X vertices (in bip graph) and subnets as CIDR's
    :return: A copy of bipGraph that has been completed with degree-1 subnets
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
                bipCopy.add_edge(nID, sID) # Will also add degree-1, isolated neighborhoods
                subnetIDCount += 1
    
    return bipCopy

def formatForProjections(bipGraph):
    '''
    Function which builds a copy of a bipartite graph and removes all multi-hop nodes (U_X) and 
    their incident edges from it. The purpose is to prepare the graph for top/bottom projections, 
    as such projections should only feature one-hop relationships.
    
    :param bipGraph:  The bipartite graph to prepare (as built by bipartiteGraph() or returned 
                      by completeBipartiteGraph())
    '''
    
    # Creates a copy of the initial graph
    bipCopy = bipGraph.copy()
    
    # Removes all U_X vertices
    allVertices = list(bipCopy.nodes)
    for vertex in allVertices:
        if vertex.startswith("U"):
            bipCopy.remove_node(vertex)
    
    return bipCopy

def bipTopProjection(bipGraph, neighborhoods):
    '''
    Function which builds the projection of a bipartite graph on its neighborhoods (top nodes) and 
    formats the resulting graph as a dictionary where each item corresponds to one neighborhood 
    and its list of adjacent vertices (other neighborhoods).
    
    :param bipGraph:       The bipartite graph after omitting multi-hops vertices (i.e., output 
                           formatForProjections())
    :param neighborhoods:  The list of neighborhoods parsed in the same snapshot used to build 
                           bipGraph
    '''
    
    neighborhoodsLabels = []
    bipNodes = list(bipGraph.nodes)
    for neighborhood in neighborhoods:
        nID = neighborhood.getLabelID()
        if nID in bipNodes:
            neighborhoodsLabels.append(nID)
    topProj = nx.bipartite.projected_graph(bipGraph, neighborhoodsLabels)
    
    projDict = dict()
    allVertices = list(topProj.nodes)
    for vertex in allVertices:
        incidentEdges = topProj.edges(vertex)
        projDict[vertex] = []
        for edge in incidentEdges:
            other = edge[1]
            if edge[1] == vertex:
                other = edge[0]
            projDict[vertex].append(other)
        projDict[vertex].sort()
    
    return projDict

def bipBotProjection(bipGraph, subnetsVertices):
    '''
    Function which builds the projection of a bipartite graph on its subnets (bottom nodes) and 
    formats the resulting graph as a dictionary where each item corresponds to one subnet and its 
    list of adjacent vertices (other subnets). Note that hypothetic subnets are included, since 
    they model existing links that could not be seen through subnet inference.
    
    :param bipGraph:       The bipartite graph after omitting multi-hops vertices (i.e., output 
                           formatForProjections())
    :param neighborhoods:  The list of subnets used as vertices
    '''
    
    subnetsLabels = []
    bipNodes = list(bipGraph.nodes)
    for subnet in subnetsVertices:
        if subnet in bipNodes:
            subnetsLabels.append(subnet)
    topProj = nx.bipartite.projected_graph(bipGraph, subnetsLabels)
    
    projDict = dict()
    allVertices = list(topProj.nodes)
    for vertex in allVertices:
        incidentEdges = topProj.edges(vertex)
        projDict[vertex] = []
        for edge in incidentEdges:
            other = edge[1]
            if edge[1] == vertex:
                other = edge[0]
            projDict[vertex].append(other)
        projDict[vertex].sort()
    
    return projDict

def projectionToText(projDict):
    '''
    Utility function which turns a projection dictionary into a single string that can be flushed 
    into an output file.
    
    :param projDict:  The dictionary of a projection as created by bipTopProjection()
    '''
    
    finalStr = ""
    for vertex in projDict:
        neighborsList = projDict[vertex]
        if len(neighborsList) == 0:
            continue
        neighborsStr = ""
        for i in range(0, len(neighborsList)):
            if i > 0:
                neighborsStr += ", "
            neighborsStr += neighborsList[i]
        finalStr += vertex + ": " + neighborsStr + "\n"
    
    return finalStr

def doubleBipartiteGraph(postNeighborhoods, prune=False):
    '''
    Function which builds a double bipartite graph, using the post-neighborhood data (see the file 
    PostNeighborhoods.py). The function features an optional "prune" boolean parameter which 
    should be set to True to remove all degree-1 vertices. This option is used to produce visual 
    renders of a graph, as drawing all degree-1 vertices considerably slows down the rendering in 
    addition to making it difficult to read. Moreover, they add little information w.r.t. the 
    topology, i.e., vertices with a degree > 1 tell more about the topology.
    
    :param postNeighborhoods:  The dictionary of post-neighborhoods (ID -> object)
    :param prune:              The optional "pruning"; set to True to prune the degree-1 vertices 
                               from the final graph (no pruning by default)
    :return: A double bipartite graph as built with NetworkX
    '''
    
    switches = []
    routers = []
    subnets = []
    allEdges = []
    switchCount = 1 # Same principle as in PostNeighborhoods.outputDoubleBipartite()
    
    # Routers/subnets (+ incident edges) enumeration depends on pruning
    if prune:
        degreeDict = dict()
        routersWithL2 = set()
        for ID in postNeighborhoods:
            current = postNeighborhoods[ID]
            L3SEdges = current.listEdgesL3S()
            for i in range(0, len(L3SEdges)):
                rID = L3SEdges[i][0]
                sID = L3SEdges[i][1]
                if rID not in degreeDict:
                    degreeDict[rID] = 1 # No degree-0 by design
                    if not current.isUniqueRouter():
                        routersWithL2.add(rID)
                else:
                    degreeDict[rID] += 1
                if sID not in degreeDict:
                    degreeDict[sID] = 1 # No degree-0 by design
                else:
                    degreeDict[sID] += 1
        
        degreeOneVertices = set()
        for vertex in degreeDict:
            if degreeDict[vertex] == 1 and vertex not in routersWithL2:
                degreeOneVertices.add(vertex)
        
        # Degree-1 vertices are now known, so routers/subnets can be listed for the final graph
        alreadyKnown = set()
        for ID in postNeighborhoods:
            current = postNeighborhoods[ID]
            L3SEdges = current.listEdgesL3S()
            for i in range(0, len(L3SEdges)):
                rID = L3SEdges[i][0]
                sID = L3SEdges[i][1]
                if sID in degreeOneVertices:
                    continue
                if rID in degreeOneVertices and rID not in routersWithL2:
                    continue
                if rID not in alreadyKnown:
                    alreadyKnown.add(rID)
                    routers.append(rID)
                if sID not in alreadyKnown:
                    alreadyKnown.add(sID)
                    subnets.append(sID)
                allEdges.append([rID, sID])
    else:
        alreadyKnown = set()
        for ID in postNeighborhoods:
            current = postNeighborhoods[ID]
            L3SEdges = current.listEdgesL3S()
            for i in range(0, len(L3SEdges)):
                rID = L3SEdges[i][0]
                sID = L3SEdges[i][1]
                if rID not in alreadyKnown:
                    alreadyKnown.add(rID)
                    routers.append(rID)
                if sID not in alreadyKnown:
                    alreadyKnown.add(sID)
                    subnets.append(sID)
                allEdges.append([rID, sID])
    
    # Switches (no pruning here, because by design, always have a degree > 1)
    for ID in postNeighborhoods:
        current = postNeighborhoods[ID]
        if not current.isUniqueRouter():
            switches.append("E" + str(switchCount))
            newEdges = current.listEdgesL2L3(switchCount)
            for i in range(0, len(newEdges)):
                allEdges.append(newEdges[i])
            switchCount += 1
    
    # N.B.: there is no sorting here, because the post-neighborhood data is already sorted by 
    # design (e.g., subnet IDs follow the order of insertion of subnets).
    
    # Creates the graph, adds all components to it and returns it
    dBip = nx.Graph()
    dBip.add_nodes_from(switches, bipartite=0)
    dBip.add_nodes_from(routers, bipartite=1)
    dBip.add_nodes_from(subnets, bipartite=2)
    dBip.add_edges_from(allEdges)
    return dBip
