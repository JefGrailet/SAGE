#! /usr/bin/env python
# -*- coding: utf-8 -*-

'''
Module for generating figures on the basis of the various graphs built by INSIGHT. Depending on 
the type of figure, additional data besides the graph itself might be required.

It's worth noting calling functions from this library does not write anything on the standard 
output (it's assumed the user knows where to find the output files), but will write error messages 
if the provided output file name has a problematic format.
'''

import networkx as nx
import matplotlib.pyplot as plt
import numpy as np

# Bipartite metrics
from networkx.algorithms import bipartite

# Needed to complete graphs to get clustering coefficients
import Neighborhoods
import Subnets

def collectionsToCDF(collections):
    '''
    Utility function which transforms a dictionary of collections (mapping a vertex of a graph 
    with a collection of elements) into a CDF of the size of each collection.
    
    :param collections:  A dictionary of collections (vertex -> collection)
    :return:             The unique sorted lengths (all collections considered) and the 
                         corresponding CDF, both returned as lists
    '''
    
    # Finds the maximum size of a collection
    maxSize = 0
    for key in collections:
        if key > maxSize:
            maxSize = key
    
    # Creates a distribution (index=size -> value=#collections of this size)
    sizeDistrib = []
    for i in range(0, maxSize + 1):
        if i in collections:
            sizeDistrib.append(len(collections[i]))
        else:
            sizeDistrib.append(0)
    
    # Turns the distribution into a CDF
    CDF = []
    totalItems = 0
    for i in range(0, len(sizeDistrib)):
        totalItems += sizeDistrib[i]
        CDF.append(sizeDistrib[i])
        if i == 0:
            continue
        CDF[i] += CDF[i-1]
    for i in range(0, len(CDF)):
        CDF[i] = float(CDF[i]) / totalItems
    
    # Returns the resulting CDF
    return CDF

def coeffsToCDF(coefficients):
    '''
    Utility function which transforms a dictionary of floating point coefficients (mapping a 
    vertex of a graph with its coefficient) into a CDF.
    
    :param clustering:  A dictionary of coefficients (vertex -> coefficient)
    :return:            The sorted coefficients and the corresponding CDF of the coefficients, 
                        both returned as lists
    '''
    
    # Makes a dictionary of coefficients, counts amount of vertices by coefficient
    verticesByCoefficient = dict()
    for vertex in coefficients:
        coeff = coefficients[vertex]
        if coeff in verticesByCoefficient:
            verticesByCoefficient[coeff] += 1
        else:
            verticesByCoefficient[coeff] = 1
    
    # Sorts the coefficients (equivalent to the x axis)
    sortedCoeffs = []
    for coeff in verticesByCoefficient:
        sortedCoeffs.append(coeff)
    sortedCoeffs.sort()
    
    # Computes the CDF itself (equivalent to the y axis)
    CDF = []
    nbVertices = 0
    for i in range(0, len(sortedCoeffs)):
        nbVertices += verticesByCoefficient[sortedCoeffs[i]]
        CDF.append(verticesByCoefficient[sortedCoeffs[i]])
        if i == 0:
            continue
        CDF[i] += CDF[i-1]
    for i in range(0, len(CDF)):
        CDF[i] = float(CDF[i]) / nbVertices
    
    # Returns the results
    return sortedCoeffs, CDF

def getNbInterfacesOf(CIDR):
    '''
    Utility function which computes the maximum amount of interfaces a subnet prefix (given in 
    CIDR notation) can cover. It is equivalent to the getMaxNbInterfaces() method of the Subnet 
    class and is used by some subsequent function(s) which have to verify this amount without 
    consulting the Subnet objects.
    
    :param CIDR:  A subnet prefix in CIDR notation
    :return:      The maximum amount of interfaces this prefix length can cover
    '''
    
    splitPrefix = CIDR.split('/')
    prefixLength = int(splitPrefix[1])
    if prefixLength > 32: # In case of misuse
        return 1
    return pow(2, 32 - prefixLength)

def degreeCDFsBip(bipGraph, outputFileName):
    '''
    Function which computes a CDF of the degree of respectively top vertices (neighborhoods) and 
    bottom vertices (subnets) and plots both CDFs in a single figure. The X axis for the top 
    degree is in logarithmic scale to better visualize the CDF (neighborhoods can have much 
    greater degrees than subnets). The provided bipartite graph must be a complete one (i.e., it 
    must contain degree-1 subnets) built with the completeBipartiteGraph() function from the 
    GraphBuilding module.
    
    :param bipGraph:        The bipartite graph as built with NetworkX (must be complete)
    :param outputFileName:  Filename for the PDF that will contain the figure
    :return:                Two dictionaries mapping a degree (for a top/bottom vertex) with a 
                            list of (top/bottom) vertices having that degree (can be useful to 
                            make a detailed review of the vertices with large degrees)
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return None
    finalOutputFileName = nameSplit[0]
    
    # Processes the list of all vertices to isolate neighborhoods (N_X) and (real) subnets (S_X)
    allVertices = list(bipGraph.nodes)
    vNeighborhoods = [] # v for vertices
    vSubnets = []
    for vertex in allVertices:
        if vertex.startswith("N"):
            vNeighborhoods.append(vertex)
        elif vertex.startswith("S"):
            vSubnets.append(vertex)
    
    # Computes a dictionary of neighborhoods by degree (top vertices)
    neighborhoodsByDegree = dict()
    for vertex in vNeighborhoods:
        curDegree = len(bipGraph.edges(vertex))
        if curDegree == 0:
            continue # Ignores isolated vertices
        if curDegree in neighborhoodsByDegree:
            neighborhoodsByDegree[curDegree].append(vertex)
        else:
            neighborhoodsByDegree[curDegree] = [vertex]
    
    # Computes a dictionary of subnets by degree (bottom vertices)
    subnetsByDegree = dict()
    for vertex in vSubnets:
        curDegree = len(bipGraph.edges(vertex))
        if curDegree in subnetsByDegree:
            subnetsByDegree[curDegree].append(vertex)
        else:
            subnetsByDegree[curDegree] = [vertex]
    
    # Computes the CDFs via a side function
    neighborhoodDegreeCDF = collectionsToCDF(neighborhoodsByDegree)
    subnetDegreeCDF = collectionsToCDF(subnetsByDegree)
    
    # Sizes for the ticks and labels of the plot
    hfont = {'fontsize': 30}
    hfont2 = {'fontsize': 26}
    
    # Stats plotting
    plt.figure(figsize=(13,11))
    plt.rcParams.update({"text.usetex": True, "font.family": "serif", "font.serif": ["Times"]})
    
    # --- First subplot: CDF of the neighborhood degree (top vertices) ---
    plt.subplot(2, 1, 1)
    
    xAxis = range(0, len(neighborhoodDegreeCDF), 1)
    plt.plot(xAxis, neighborhoodDegreeCDF, color='#0000FF', marker='o', linewidth=3)
    plt.xscale('log')
    
    # Computes the smallest power of 10 (+ ticks for the X axis) greater than the max degree
    topLimitX = 1
    topXTicks = [topLimitX]
    maxNeighborhoodDegree = len(neighborhoodDegreeCDF) - 1
    while topLimitX < maxNeighborhoodDegree:
        topLimitX *= 10
        topXTicks.append(topLimitX)
    
    # Limits
    plt.ylim([0, 1.05]) # + 0.5 so the top of the curve can be seen
    plt.xlim([1, topLimitX])
    
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
    yTicks = np.arange(0, 1.1, 0.1)
    plt.yticks(yTicks, **hfont2)
    plt.xticks(topXTicks, **hfont2)
    plt.grid()
    plt.ylabel(r'\textbf{CDF ($\top$)}', **hfont)
    plt.xlabel(r'\textbf{Neighborhood ($\top$) degree}', **hfont)
    
    # --- Second subplot: CDF of the subnet degree (bottom vertices) ---
    plt.subplot(2, 1, 2)
    
    xAxis = range(0, len(subnetDegreeCDF), 1)
    plt.plot(xAxis, subnetDegreeCDF, color='#FF0000', marker='o', linewidth=3)
    
    # Limits
    plt.ylim([0, 1.05]) # + 0.5 so the top of the curve can be seen
    plt.xlim([0, len(subnetDegreeCDF) - 1])
    
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
    
    # Ticks and labels; ticks on X are adjusted if too many of them
    xTicksLabels = xAxis
    if len(xTicksLabels) > 30: # More than 30 ticks makes it difficult to read
        dividers = [5, 10, 50, 100, 500, 1000, 5000, 10000] # Unlikely to have more
        i = 0
        while i < 8 and len(xTicksLabels) / dividers[i] > 30:
            i += 1
        maxValue = xAxis[len(xAxis) - 1]
        xTicksLabels = range(0, maxValue, dividers[i])
    
    plt.yticks(yTicks, **hfont2)
    plt.xticks(xTicksLabels, **hfont2)
    plt.grid()
    plt.ylabel(r'\textbf{CDF ($\bot$)}', **hfont)
    plt.xlabel(r'\textbf{Subnet ($\bot$) degree}', **hfont)
    
    # Adjusts final figure and saves it
    plt.subplots_adjust(hspace=0.4)
    plt.savefig(outputFileName + ".pdf")
    plt.clf()
    
    return neighborhoodsByDegree, subnetsByDegree

def topClusteringByDegreeBip(bipGraph, outputFileName):
    '''
    Function which computes the clustering of top vertices (neighborhoods) and plots the resulting 
    coefficients in function of the degree of the corresponding vertex in a scatter plot with a 
    logarithmic scale. The provided bipartite graph must be a complete one (i.e., it must contain 
    degree-1 subnets) built with completeBipartiteGraph() from the GraphBuilding module.
    
    :param bipGraph:        The bipartite graph as built with NetworkX (must be complete)
    :param outputFileName:  Filename for the PDF that will contain the figure
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return None
    finalOutputFileName = nameSplit[0]
    
    # Isolates the neighborhoods as vertices (N_X)
    allVertices = list(bipGraph.nodes)
    neighborhoods = []
    subnets = []
    for vertex in allVertices:
        if len(bipGraph.edges(vertex)) == 0:
            continue # Ignores isolated vertices
        if vertex.startswith("N"):
            neighborhoods.append(vertex)
        elif vertex.startswith("S"):
            subnets.append(vertex)
    
    # Gets their clustering
    topClustering = bipartite.clustering(bipGraph, nodes=neighborhoods, mode="min")
    
    # Computes a dictionary of neighborhoods by degree (top vertices)
    neighborhoodsByDegree = dict()
    maxNeighborhoodDegree = 0
    for vertex in neighborhoods:
        curDegree = len(bipGraph.edges(vertex))
        if curDegree in neighborhoodsByDegree:
            neighborhoodsByDegree[curDegree].append(vertex)
        else:
            neighborhoodsByDegree[curDegree] = [vertex]
        if curDegree > maxNeighborhoodDegree:
            maxNeighborhoodDegree = curDegree
    
    # Creates the lists corresponding to the X and Y axis of the figures
    topCoeffs = [] # Y axis
    topDegrees = [] # X axis
    for i in range(0, maxNeighborhoodDegree + 1):
        if i not in neighborhoodsByDegree:
            continue
        curDegreeList = neighborhoodsByDegree[i]
        for j in range(0, len(curDegreeList)):
            topDegrees.append(i)
            topCoeffs.append(topClustering[curDegreeList[j]])
    
    # Sizes for the ticks and labels of the plot
    hfont = {'fontsize': 28}
    hfont2 = {'fontsize': 24}
    
    # Stats plotting
    plt.figure(figsize=(13,9))
    plt.rcParams.update({"text.usetex": True, "font.family": "serif", "font.serif": ["Times"]})
    plt.scatter(topDegrees, topCoeffs, color='#0000FF', marker='x', linewidth=2)
    plt.xscale('log')
    
    # Gets the smallest power of then that is higher than the maximum degree
    limitX = 1
    xTicks = [limitX]
    while limitX < maxNeighborhoodDegree:
        limitX *= 10
        xTicks.append(limitX)
    
    # Limits
    plt.ylim([0, 1.05]) # + 0.5 so the top markers can be seen more easily
    plt.xlim([1, limitX])
    
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
    plt.xticks(xTicks, **hfont2)
    plt.grid()
    plt.ylabel(r'\textbf{Clustering ($\top$)}', **hfont)
    plt.xlabel(r'\textbf{Neighborhood ($\top$) degree}', **hfont)
    
    # Saves the figure
    plt.savefig(outputFileName + ".pdf")
    plt.clf()

def cyclesDistributionBip(bipGraph, subnetsVertices, outputFileName):
    '''
    Function which enumerates the base cycles found in a bipartite graph, assesses their soundness 
    and plots their amounts in a bar chart (where the X axis gives the length of the cycle in hop 
    count). This function also returns the cycles themselves so the user can print more details 
    about them in the terminal. The provided bipartite graph doesn't need to be complete.
    
    :param bipGraph:         The bipartite graph as built with NetworkX
    :param subnetsVertices:  The mappings between the S_X IDs and the corresponding subnets; this 
                             is required to know how many interfaces a subnet can cover (and 
                             therefore, what is its maximum degree)
    :param outputFileName:   Filename for the PDF that will contain the figure
    '''
    
    # Processes the given outputFileName
    nameSplit = outputFileName.split(".")
    if len(nameSplit) > 2:
        print("File name is incorrectly formatted (shouldn't have \".\" except for extension).")
        return None
    finalOutputFileName = nameSplit[0]
    
    # Uses the cycle_basis() function from NetworkX to find base cycles
    allVerticesList = list(bipGraph.nodes)
    allVerticesList.sort()
    rawCycles = nx.cycle_basis(bipGraph, root=allVerticesList[0])
    
    # N.B.: the root is set as the first vertex to make sure one always gets the same cycles.
    
    if len(rawCycles) == 0:
        return None
    
    # Sorts the cycles to guarantee they can always be printed the same way
    cycles = []
    for i in range(0, len(rawCycles)):
        curCycle = rawCycles[i]
        lowestID = 65535
        lowestIndex = 0
        throughMainGraph = True
        for j in range(0, len(curCycle)):
            if curCycle[j].startswith("U"): # Remote peers
                throughMainGraph = False
                break
            if curCycle[j].startswith("N"):
                ID = int(curCycle[j][1:])
                if ID < lowestID:
                    lowestID = ID
                    lowestIndex = j
        # If the cycle passes through remote peers, ignores it
        if not throughMainGraph:
            continue
        # Cycle will start will the neighborhood having the smallest ID
        while lowestIndex > 0:
            element = curCycle.pop(0)
            curCycle.append(element)
            lowestIndex -= 1
        # Enforces a same reading direction by checking the IDs of first and last vertices
        startID = int(curCycle[1][1:])
        endID = int(curCycle[len(curCycle) - 1][1:])
        if startID > endID:
            head = curCycle.pop(0)
            curCycle.append(head)
            curCycle.reverse()
        cycles.append(curCycle)
    
    if len(cycles) == 0:
        return None
    
    # Remark: the output of cycles is random at first due the algorithm relying on sets. An 
    # alternative to the code above would be to modify the cycle_basis() function from NetworkX: 
    # https://github.com/networkx/networkx/blob/master/networkx/algorithms/cycles.py
    
    # Computes a dictionary of cycles per length
    cyclesPerLength = dict()
    maxCycleLength = 0
    for i in range(0, len(cycles)):
        cycleLength = len(cycles[i])
        if cycleLength > maxCycleLength:
            maxCycleLength = cycleLength
        if cycleLength in cyclesPerLength:
            cyclesPerLength[cycleLength].append(cycles[i])
        else:
            cyclesPerLength[cycleLength] = [cycles[i]]
    
    # Computes lists to make bar charts depicting the distribution of cycles per length
    soundCycles = []
    bogusCycles = [] # I.e., some subnets aren't large enough for their degree
    labels = []
    maxCyclesAmount = 0
    for i in range(4, maxCycleLength+1, 2): # No cycle of length below 4 + length always even
        if i not in cyclesPerLength:
            soundCycles.append(0)
            bogusCycles.append(0)
            labels.append("")
            continue
        nbSound = 0
        nbBogus = 0
        nbCycles = len(cyclesPerLength[i])
        for j in range(0, nbCycles):
            cycle = cyclesPerLength[i][j]
            withBogusLink = False
            for k in range(0, len(cycle)):
                if cycle[k].startswith("S"):
                    subnetID = cycle[k]
                    degree = len(bipGraph.edges(subnetID))
                    maxInterfaces = getNbInterfacesOf(subnetsVertices[subnetID])
                    if degree > maxInterfaces:
                        withBogusLink = True
                        break
            if withBogusLink:
                nbBogus += 1
            else:
                nbSound += 1
        soundCycles.append(nbSound)
        bogusCycles.append(nbBogus)
        labels.append(str(nbSound) + "+" + str(nbBogus))
        if nbCycles > maxCyclesAmount:
            maxCyclesAmount = nbCycles
    
    # Sizes for the ticks and labels of the plot
    hfont = {'fontweight': 'bold', 'fontsize': 28}
    hfont2 = {'fontsize': 24}
    
    ind = np.arange(2, len(soundCycles) + 2, 1)
    width = 0.8
    
    # Plots the results with stacked bar charts (bar 1 = sound cycles, bar 2 = bogus cycles)
    plt.figure(figsize=(13,9))
    plt.rc('text', usetex=False) # In case set to true by a previous function call
    plt.rc('font', family='Times New Roman')
    
    # Creates the bar charts
    p1 = plt.bar(ind, soundCycles, width, color='#F0F0F0', edgecolor='#000000')
    p2 = plt.bar(ind, bogusCycles, width, color='#D0D0D0', edgecolor='#000000', bottom=soundCycles)
    
    # Gets the next multiple of a power of 10 higher than the max amount of cycles
    maxValueY = (int(maxCyclesAmount / 10) + 1) * 10
    if maxValueY > 20:
        maxValueY += 10
    
    # Limits
    plt.ylim([0, maxValueY])
    plt.xlim([0, len(soundCycles) + 2])
    
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
    
    # Ticks and labels; ticks are adjusted on both axis if too many of them
    if maxValueY == 10:
        yTicks = range(0, 11, 1)
    else:
        yTicks = range(0, maxValueY + 1, 10)
    plt.yticks(yTicks, **hfont2)
    plt.xticks(range(0, int(maxCycleLength / 2) + 1, 1), **hfont2)
    plt.grid()
    plt.ylabel('Amount of cycles', **hfont)
    plt.xlabel('Hop count', **hfont)
    
    # Writes the total amount of cycles (sound + bogus) on top of each bar
    for rect1, rect2, label in zip(p1, p2, labels):
        if len(label) == 0:
            continue
        height = rect1.get_height() + rect2.get_height()
        axis.text(rect1.get_x() + rect1.get_width() / 2, height + 0.2, label, ha='center', va='bottom', **hfont2)
    
    # Legend
    plt.legend((p1[0], p2[0]), 
               ('Sound cycles', 'Bogus cycles'), 
               bbox_to_anchor=(0.05, 1.02, 0.90, .102), 
               loc=3,
               ncol=3, 
               mode="expand", 
               borderaxespad=0.,
               fontsize=26)
    
    # Saves the figure
    plt.savefig(outputFileName + ".pdf")
    plt.clf()
    
    # Sorts the cycles before returning them
    cycles.sort()
    return cycles
