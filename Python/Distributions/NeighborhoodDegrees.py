#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Computes CDFs for neighborhood degrees in both bipartite graphs and their projections to compare 
# them. Uses the output files produced by INSIGHT for a collection of ASes and dates to so.

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

def getBipDegrees(lines):
    degrees = []
    amounts = []
    
    # Gets the degrees and their amounts
    count = 0
    for i in range(0, len(lines)):
        if not lines[i]:
            if count < 2:
                count += 1
                continue
            else:
                break
        
        if count != 2:
            continue
        
        if lines[i].startswith("---"):
            continue
        
        split = lines[i].split(" ")
        degree = int(split[1][:-1])
        if len(split) >= 4 and split[3] == "neighborhoods":
            amount = int(split[2])
        else:
            amount = len(split) - 2
        
        degrees.append(degree)
        amounts.append(amount)
    
    if len(degrees) == 0:
        return None
    
    # Formats the results as a single array where index = degree - 1
    largestDegree = degrees[len(degrees) - 1]
    res = []
    for i in range(0, largestDegree):
        res.append(0)
    
    for i in range(0, len(degrees)):
        res[degrees[i] - 1] = amounts[i]
        
    return res

def getProjDegrees(lines):
    largestDegree = 0
    
    # Does a first traversal to spot the largest degree
    for i in range(0, len(lines)):
        curDegree = lines[i].count(", ") + 1
        if curDegree > largestDegree:
            largestDegree = curDegree
    
    if largestDegree == 0:
        return None
    
    res = []
    for i in range(0, largestDegree):
        res.append(0)
    
    # Now records the degrees in the same format as getBipDegrees()
    for i in range(0, len(lines)):
        curDegree = lines[i].count(", ") + 1
        res[curDegree - 1] += 1
    
    return res

if __name__ == "__main__":

    if len(sys.argv) < 2 or len(sys.argv) > 4:
        print("Usage:")
        print("1) python NeighborhoodDegrees.py [text file w/ ASes] [text file w/ dates]")
        print("2) python NeighborhoodDegrees.py [list of \"AS+dd-mm-yyy\" separated with comma]")
        print("In both cases, you can use the optional arg [output file name (w/o extension)].")
        sys.exit()
    
    firstArg = str(sys.argv[1])
    data = [] # Will contain the snapshots to process as list of (AS, dd, mm, yyy) tuples
    outputLabel = "Neighborhood_degrees" # Default name for the output figure (as PDF)
    
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
    largestBipDegree = 0
    largestBipDegreeDate = ""
    largestBipDegreeAS = ""
    largestProjDegree = 0
    largestProjDegreeDate = ""
    largestProjDegreeAS = ""
    bipDegrees = []
    projDegrees = []
    for i in range(0, len(data)):
        filePrefix = datasetPrefix + data[i][0] + "/" + data[i][3] + "/" + data[i][2] + "/"
        filePrefix += data[i][1] + "/" + data[i][0] + "_" + data[i][1] + "-" + data[i][2] + "-"
        filePrefix += data[i][3]
        
        bipFilePath = filePrefix + "_Bip.txt"
        projFilePath = filePrefix + "_BipProjTop.txt"

        # Checks the summary file for the bipartite graph exists and parses it
        if not os.path.isfile(bipFilePath):
            print("Bipartite graph summary file does not exist (path: " + bipFilePath + ").")
            sys.exit()
        
        with open(bipFilePath) as f:
            bipFileLines = f.read().splitlines()
        
        # Ditto for the projection
        if not os.path.isfile(projFilePath):
            print("File describing the projection does not exist (path: " + projFilePath + ").")
            sys.exit()
        
        with open(projFilePath) as f:
            projFileLines = f.read().splitlines()
            
        curBipDegrees = getBipDegrees(bipFileLines)
        if curBipDegrees == None:
            continue
        curProjDegrees = getProjDegrees(projFileLines)
        if curProjDegrees == None:
            continue
        
        if len(curBipDegrees) > largestBipDegree:
            largestBipDegree = len(curBipDegrees)
            largestBipDegreeDate = data[i][1] + "/" + data[i][2] + "/" + data[i][3]
            largestBipDegreeAS = data[i][0]
        if len(curProjDegrees) > largestProjDegree:
            largestProjDegree = len(curProjDegrees)
            largestProjDegreeDate = data[i][1] + "/" + data[i][2] + "/" + data[i][3]
            largestProjDegreeAS = data[i][0]
        
        bipDegrees.append(curBipDegrees)
        projDegrees.append(curProjDegrees)
    
    largestBipDegreeStr = "Largest degree in a bipartite graph (" + str(largestBipDegree)
    largestBipDegreeStr += ") found in " + largestBipDegreeAS + " on "
    largestBipDegreeStr += largestBipDegreeDate + "."
    print(largestBipDegreeStr)
    
    largestProjDegreeStr = "Largest degree in a projection (" + str(largestProjDegree)
    largestProjDegreeStr += ") found in " + largestProjDegreeAS + " on "
    largestProjDegreeStr += largestProjDegreeDate + "."
    print(largestProjDegreeStr)
    
    largestDegreeOverall = largestBipDegree
    if largestProjDegree > largestDegreeOverall:
        largestDegreeOverall = largestProjDegree
    
    # Computes final degree distributions
    finalBipDegrees = []
    for i in range(0, largestBipDegree):
        finalBipDegrees.append(0)
    
    totalBip = 0
    for i in range(0, len(bipDegrees)):
        curDegrees = bipDegrees[i]
        for j in range(0, len(curDegrees)):
            finalBipDegrees[j] += curDegrees[j]
            totalBip += curDegrees[j]
    
    finalProjDegrees = []
    for i in range(0, largestProjDegree):
        finalProjDegrees.append(0)
    
    totalProj = 0
    for i in range(0, len(projDegrees)):
        curDegrees = projDegrees[i]
        for j in range(0, len(curDegrees)):
            finalProjDegrees[j] += curDegrees[j]
            totalProj += curDegrees[j]
    
    # Computes one CDF for each distribution
    ratiosBipDegrees = []
    for i in range(0, len(finalBipDegrees)):
        ratiosBipDegrees.append(float(finalBipDegrees[i]) / float(totalBip))
    
    ratiosProjDegrees = []
    for i in range(0, len(finalProjDegrees)):
        ratiosProjDegrees.append(float(finalProjDegrees[i]) / float(totalProj))
    
    xAxisBip = np.arange(0, largestBipDegree + 1, 1)
    bipCDF = []
    bipCDF.append(0)
    for i in range(0, len(ratiosBipDegrees)):
        bipCDF.append(bipCDF[i] + ratiosBipDegrees[i])
    
    xAxisProj = np.arange(0, largestProjDegree + 1, 1)
    projCDF = []
    projCDF.append(0)
    for i in range(0, len(ratiosProjDegrees)):
        projCDF.append(projCDF[i] + ratiosProjDegrees[i])
    
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
    
    plt.semilogx(xAxisBip, bipCDF, color='#000000', linestyle=":", linewidth=3, label="Degree in bipartite graph")
    plt.semilogx(xAxisProj, projCDF, color='#000000', linewidth=3, label="Degree in projection")
    plt.ylim([0, 1])
    plt.xlim([1, largestDegreeOverall])
    plt.yticks(np.arange(0, 1.1, 0.1), **hfont2)
    
    # Ticks for the X axis (log scale)
    tickValues = []
    tickDisplay = []
    tickValues.append(1)
    tickDisplay.append(1)
    if largestDegreeOverall > 10:
        power = 1
        exponent = 0
        while power < largestDegreeOverall:
            power *= 10
            exponent += 1
            if power < largestDegreeOverall:
                tickValues.append(power)
                tickDisplay.append(r"$10^{{ {:1d} }}$".format(exponent))
    else:
        for i in range(2, largestDegreeOverall):
            tickValues.append(i)
            tickDisplay.append(i)
    plt.xticks(tickValues, tickDisplay, **hfont2)
    
    axis = plt.gca()
    yaxis = axis.get_yaxis()
    xaxis = axis.get_xaxis()
    
    # Nit: hides minor ticks that automatically appear
    xticks = xaxis.get_minor_ticks()
    for i in range(0, len(xticks)):
        xticks[i].label1.set_visible(False)
    
    # Nit: hides the 0 from the ticks of y axis
    yticks = yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    
    plt.ylabel('CDFs of degrees', **hfont)
    plt.xlabel('Neighborhood degree', **hfont)
    plt.grid()
    
    plt.legend(bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=2,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.,
               fontsize=23)

    plt.savefig(outputLabel + ".pdf")
    plt.clf()
    print("CDFs of degrees have been saved in " + outputLabel + ".pdf.")
