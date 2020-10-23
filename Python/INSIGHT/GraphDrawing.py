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
                           node_color='b',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(directedGraph, 
                           pos,
                           nodelist=list(miscellaneous),
                           node_color='g',
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
    listBip1_1 = []
    listBip2_1 = []
    listBip2_2 = []
    listBip2_3 = []
    oneHopEdges = []
    multiHopsEdges = []
    
    vertices = bipGraph.nodes()
    edges = bipGraph.edges()
    
    for vertex in vertices:
        if vertex.startswith("N"):
            listBip1_1.append(vertex)
        elif vertex.startswith("S"):
            listBip2_1.append(vertex)
        elif vertex.startswith("T"):
            listBip2_2.append(vertex)
        else:
            listBip2_3.append(vertex)
    
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
    
    # Draws nodes (blue = neighborhoods, red = subnets, hypothetic sub = light red green = remote)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listBip1_1,
                           node_color='b',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listBip2_1,
                           node_color='r',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listBip2_2,
                           node_color='lightcoral',
                           node_size=500,
                           alpha=0.8)
    nx.draw_networkx_nodes(bipGraph, 
                           pos,
                           nodelist=listBip2_3,
                           node_color='g',
                           node_size=500,
                           alpha=0.6)
    
    # Draws edges
    nx.draw_networkx_edges(bipGraph, pos, edgelist=oneHopEdges)
    nx.draw_networkx_edges(bipGraph, pos, edgelist=multiHopsEdges, edge_color='g')
    
    # Adds labels
    nx.draw_networkx_labels(bipGraph, pos)
    
    # Saves the result with the given output file name (may overwrite an existing file)
    plt.savefig(finalOutputFileName + ".pdf")
    return
