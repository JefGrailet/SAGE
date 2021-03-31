#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes one CDF for the length of cycles found in bipartite graphs. Uses the output files 
# produced by INSIGHT for a collection of ASes and dates to so.

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

def getBipCycles(lines):
    longuestCycle = 0
    
    # Locates the start of the cycles section and finds the length of the longuest non-bogus cycle
    count = 0
    startIndex = 0
    for i in range(0, len(lines)):
        if not lines[i]:
            if count < 4:
                count += 1
                continue
            else:
                break
        
        if count != 4:
            continue
        
        if lines[i].startswith("---"):
            startIndex = i + 1
            continue
        
        # Ignores bogus cycles (i.e., some subnet prefix is too small for its amount of edges)
        if "(B)" not in lines[i]:
            # Cycle length is divided by two because subnets act as links, not hops
            cycleLength = int((lines[i].count(", ") + 1) / 2)
            if cycleLength > longuestCycle:
                longuestCycle = cycleLength
    
    if startIndex == 0 or longuestCycle == 0:
        return None
    
    # Counts the cycles by hop count
    res = []
    for i in range(0, longuestCycle + 1):
        res.append(0)
    
    for i in range(startIndex, len(lines)):
        if not lines[i]:
            break
        
        if "(B)" in lines[i]:
            continue
        
        cycleLength = int((lines[i].count(", ") + 1) / 2)
        res[cycleLength - 1] += 1
    
    return res

if __name__ == "__main__":
    
    if len(sys.argv) < 2 or len(sys.argv) > 4:
        print("Usage:")
        print("1) python CyclesDistribution.py [text file w/ ASes] [text file w/ dates]")
        print("2) python CyclesDistribution.py [list of \"AS+dd-mm-yyy\" separated with comma]")
        print("In both cases, you can use the optional arg [output file name (w/o extension)].")
        sys.exit()
    
    firstArg = str(sys.argv[1])
    data = [] # Will contain the snapshots to process as list of (AS, dd, mm, yyy) tuples
    outputLabel = "Cycles_distribution" # Default name for the output figure (as PDF)
    
    # TODO: change this ! (path to the dataset)
    datasetPrefix = "/home/jefgrailet/Online repositories/SAGE/Python/INSIGHT/Results/"
    
    # Snapshots are given as a string (for EdgeNet snapshots with semi-random VPs)
    if len(sys.argv) == 2 or not os.path.isfile(firstArg):
        splitArg = firstArg.split(",")
        for i in range(0, len(splitArg)):
            secondSplit = splitArg[i].split("+")
            ASN = secondSplit[0]
            date = secondSplit[1].split("-")
            data.append([ASN, date[0], date[1], date[2]])
        
        if len(sys.argv) >= 3:
            outputLabel = str(sys.argv[2])
            if "." in outputLabel:
                print("Warning: suggested name for the output PDF contains an extension. Please remove it.")
                sys.exit()
        if len(sys.argv) == 4:
            print("Warning: one additional arg has been provided; will be ignored.")
    # ASes + dates provided in side text files (for PlanetLab snapshots w/ VP rotations)
    elif len(sys.argv) == 3 or os.path.isfile(firstArg):
        ASesFilePath = firstArg
        datesFilePath = str(sys.argv[2])
        if len(sys.argv) == 4:
            outputLabel = str(sys.argv[3])
            if "." in outputLabel:
                print("Warning: suggested name for the output PDF contains an extension. Please remove it.")
                sys.exit()
        
        # Checks existence of the file providing ASes and parses it
        if not os.path.isfile(ASesFilePath):
            print(ASesFilePath + " does not exist.")
            sys.exit()
        with open(ASesFilePath) as f:
            ASes = f.read().splitlines()
        
        # Ditto for dates
        if not os.path.isfile(datesFilePath):
            print(datesFilePath + " does not exist.")
            sys.exit()
        with open(datesFilePath) as f:
            datesRaw = f.read().splitlines()
        dates = []
        for i in range(0, len(datesRaw)):
            splitDate = datesRaw[i].split('/')
            dates.append(splitDate)
        
        # Lists snapshots to process
        for i in range(0, len(ASes)):
            for j in range(0, len(dates)):
                data.append([ASes[i], dates[j][0], dates[j][1], dates[j][2]])
    
    # Will now process the snapshots
    longuestCycle = 0
    longuestCycleDate = ""
    longuestCycleAS = ""
    bipCycles = []
    for i in range(0, len(data)):
        bipFilePath = datasetPrefix + data[i][0] + "/" + data[i][3] + "/" + data[i][2] + "/"
        bipFilePath += data[i][1] + "/" + data[i][0] + "_" + data[i][1] + "-" + data[i][2] + "-"
        bipFilePath += data[i][3] + "_Bip.txt"

        # Checks the summary file for the bipartite graph exists and parses it
        if not os.path.isfile(bipFilePath):
            print("Bipartite graph summary file does not exist (path: " + bipFilePath + ").")
            sys.exit()
        
        with open(bipFilePath) as f:
            bipFileLines = f.read().splitlines()
        
        curCycles = getBipCycles(bipFileLines)
        if curCycles == None:
            continue
        
        if len(curCycles) > longuestCycle:
            longuestCycle = len(curCycles)
            longuestCycleDate = data[i][1] + "/" + data[i][2] + "/" + data[i][3]
            longuestCycleAS = data[i][0]
        
        bipCycles.append(curCycles)
    
    longuestCycleStr = "Longuest cycle (" + str(longuestCycle - 1) + " hops) found in "
    longuestCycleStr += longuestCycleAS + " on " + longuestCycleDate + "."
    print(longuestCycleStr)
    
    # Computes final cycle length distribution
    finalCycles = []
    for i in range(0, longuestCycle):
        finalCycles.append(0)
    
    totalCycles = 0
    for i in range(0, len(bipCycles)):
        curCycles = bipCycles[i]
        for j in range(0, len(curCycles)):
            finalCycles[j] += curCycles[j]
            totalCycles += curCycles[j]
    
    print(str(totalCycles) + " cycles found in " + str(len(data)) + " snapshots.")
    
    # Computes one CDF with the distribution
    ratiosCycles = []
    for i in range(0, len(finalCycles)):
        ratiosCycles.append(float(finalCycles[i]) / float(totalCycles))
    
    xAxis = np.arange(0, longuestCycle, 1)
    CDF = []
    CDF.append(0)
    for i in range(0, len(ratiosCycles) - 1):
        CDF.append(CDF[i] + ratiosCycles[i])
    
    # Plots result
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':26}

    hfont2 = {'fontname':'serif',
             'fontsize':22}
    
    hfont3 = {'fontname':'serif',
             'fontsize':16}
    
    hfont4 = {'fontname':'serif',
             'fontsize':12}
    
    plt.figure(figsize=(13,9))
    
    plt.plot(xAxis, CDF, color='#000000', linewidth=3)
    plt.ylim([0, 1])
    plt.xlim([1, longuestCycle - 1])
    plt.yticks(np.arange(0, 1.1, 0.1), **hfont2)
    
    # Ticks for the X axis (log scale)
    tickValues = []
    tickDisplay = []
    tickValues.append(1)
    tickDisplay.append(1)
    for i in range(2, longuestCycle):
        tickValues.append(i)
        tickDisplay.append(i)
    plt.xticks(tickValues, tickDisplay, **hfont2)
    
    axis = plt.gca()
    yaxis = axis.get_yaxis()
    xaxis = axis.get_xaxis()
    
    # Nit: hides the 0 from the ticks of y axis
    yticks = yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    
    plt.ylabel('Cycle CDF', **hfont)
    plt.xlabel('Hop count (cycle length)', **hfont)
    plt.grid()

    plt.savefig(outputLabel + ".pdf")
    plt.clf()
    print("CDF of the length of cycles have been saved in " + outputLabel + ".pdf.")
