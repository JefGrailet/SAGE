#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
Module for drawing graphs as built by the functions of the GraphBuilding module.
'''

import networkx as nx
import matplotlib.pyplot as plt
import numpy as np # N.B.: package scipy also needed

def drawDirectedGraph(directedGraph, outputFileName):
    '''
    Function which draws a provided (simple) directed graph.
    
    :param directedGraph:  The directed graph as built with NetworkX
    :param outputFileName: Filename for the PDF that will contain the figure
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return None
    finalOutputFileName = nameSplit[0]
    
    # Lists the vertices/edges of the graph per type to colour them properly
    vertices = []
    miscellaneous = []
    oneHopEdges = []
    multiHopsEdges = []
    
    nodes = directedGraph.nodes()
    edges = directedGraph.edges()
    
    for vertex in nodes:
        if vertex.startswith("N"):
            vertices.append(vertex)
        else:
            miscellaneous.append(vertex)
    
    for edge in edges:
        if edge[0].startswith("N") and edge[1].startswith("N"):
            oneHopEdges.append(edge)
        else:
            multiHopsEdges.append(edge)
    
    # The use of a specific seed allows to get the same figure upon re-run
    pos = nx.spring_layout(directedGraph, seed=np.random.RandomState(223973))
    plt.figure(figsize=(100,100))
    
    # Draws nodes (blue = neighborhoods, red nodes = miscellaenous)
    nx.draw_networkx_nodes(directedGraph, 
                           pos,
                           nodelist=list(vertices),
                           node_color='blue',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(directedGraph, 
                           pos,
                           nodelist=list(miscellaneous),
                           node_color='green',
                           node_size=500,
                           alpha=0.6)
    
    # Draws edges (black = regular graph, red = traceroute sub-graph)
    nx.draw_networkx_edges(directedGraph, pos, edgelist=oneHopEdges)
    nx.draw_networkx_edges(directedGraph, pos, edgelist=multiHopsEdges, edge_color='g')
    
    # Adds labels
    nx.draw_networkx_labels(directedGraph, pos)
    
    # Saves the result with the given output file name (may overwrite an existing file)
    plt.savefig(finalOutputFileName + ".pdf")
    return

def drawBipartiteGraph(bipGraph, outputFileName):
    '''
    Function which draws a provided simple bipartite graph.
    
    :param bipGraph:       The bipartite graph as built with NetworkX
    :param outputFileName: Filename for the PDF that will contain the figure
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return
    finalOutputFileName = nameSplit[0]
    
    # Lists the vertices/edges of the graph per type to colour them properly
    listVertices1_1 = []
    listVertices2_1 = []
    listVertices2_2 = []
    listVertices2_3 = []
    oneHopEdges = []
    multiHopsEdges = []
    
    vertices = bipGraph.nodes()
    edges = bipGraph.edges()
    
    for vertex in vertices:
        if vertex.startswith("N"):
            listVertices1_1.append(vertex)
        elif vertex.startswith("S"):
            listVertices2_1.append(vertex)
        elif vertex.startswith("T"):
            listVertices2_2.append(vertex)
        else:
            listVertices2_3.append(vertex)
    
    for edge in edges:
        if edge[1].startswith("U"):
            multiHopsEdges.append(edge)
        else:
            oneHopEdges.append(edge)
    
    # Chosen dimensions are arbitrary
    dimension = 75
    if len(vertices) > 4000:
        dimension = 300
    elif len(vertices) > 2000:
        dimension = 225
    elif len(vertices) > 1000:
        dimension = 150
    
    # The use of a specific seed allows to get the same Figure upon re-run
    pos = nx.spring_layout(bipGraph, seed=np.random.RandomState(223973))
    plt.figure(figsize=(dimension, dimension))
    
    # Draws nodes (blue = neighborhoods, red = subnets, hypothetic sub = light red, green = remote)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listVertices1_1,
                           node_color='blue',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listVertices2_1,
                           node_color='red',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listVertices2_2,
                           node_color='lightcoral',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listVertices2_3,
                           node_color='green',
                           node_size=500,
                           alpha=0.6)
    
    # Draws edges
    nx.draw_networkx_edges(bipGraph, pos, edgelist=oneHopEdges)
    nx.draw_networkx_edges(bipGraph, pos, edgelist=multiHopsEdges, edge_color='green')
    
    # Adds labels
    nx.draw_networkx_labels(bipGraph, pos)
    
    # Saves the result with the given output file name (may overwrite an existing file)
    plt.savefig(finalOutputFileName + ".pdf")
    return

def drawTripartiteGraph(tripGraph, outputFileName):
    '''
    Function which draws a provided tripartite graph (Layer-2/Layer-3/Subnet).
    
    :param tripGraph:      The tripartite graph as built with NetworkX
    :param outputFileName: Filename for the PDF that will contain the figure
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return
    finalOutputFileName = nameSplit[0]
    
    # Lists the vertices/edges of the graph per type to colour them properly
    listVertices0_1 = [] # Hypothetical switches
    listVertices1_1 = [] # Inferred routers (alias pairs/lists)
    listVertices1_2 = [] # Hypothetical routers
    listVertices2_1 = [] # Inferred subnets
    listVertices2_2 = [] # Hypothetical subnets
    listVertices2_3 = [] # "Fake" subnets / remote links (completed with multiHopsEdges)
    regularEdges = [] # Not always one hop here (router - switch =/= one hop)
    multiHopsEdges = []
    
    vertices = tripGraph.nodes()
    edges = tripGraph.edges()
    
    for vertex in vertices:
        if vertex.startswith("E"):
            listVertices0_1.append(vertex)
        elif vertex.startswith("R"):
            listVertices1_1.append(vertex)
        elif vertex.startswith("I"):
            listVertices1_2.append(vertex)
        elif vertex.startswith("S"):
            listVertices2_1.append(vertex)
        elif vertex.startswith("T"):
            listVertices2_2.append(vertex)
        else:
            listVertices2_3.append(vertex)
    
    for edge in edges:
        if edge[1].startswith("U"):
            multiHopsEdges.append(edge)
        else:
            regularEdges.append(edge)
    
    # Chosen dimensions are arbitrary
    dimension = 75
    if len(vertices) > 4000:
        dimension = 300
    elif len(vertices) > 2000:
        dimension = 225
    elif len(vertices) > 1000:
        dimension = 150
    
    # The use of a specific seed allows to get the same Figure upon re-run
    pos = nx.spring_layout(tripGraph, seed=np.random.RandomState(223973))
    plt.figure(figsize=(dimension, dimension))
    
    # Draws nodes (yellow = switch, blue = router, red = subnet, green = remote link)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices0_1,
                           node_color='yellow',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices1_1,
                           node_color='blue',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices1_2,
                           node_color='cornflowerblue',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices2_1,
                           node_color='red',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices2_2,
                           node_color='lightcoral',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(tripGraph, 
                           pos,
                           nodelist=listVertices2_3,
                           node_color='green',
                           node_size=500,
                           alpha=0.6)
    
    # Draws edges
    nx.draw_networkx_edges(tripGraph, pos, edgelist=regularEdges)
    nx.draw_networkx_edges(tripGraph, pos, edgelist=multiHopsEdges, edge_color='green')
    
    # Adds labels
    nx.draw_networkx_labels(tripGraph, pos)
    
    # Saves the result with the given output file name (may overwrite an existing file)
    plt.savefig(finalOutputFileName + ".pdf")
    return
