#! /usr/bin/env python

'''
This module provides 2 classes and a function to handle subnets as discovered by WISE/SAGE.
'''

class SubnetInterface:
    '''
    Class used to model a single interface of a subnet.
    '''
    PIVOT = 0
    CONTRAPIVOT = 1
    OUTLIER = 2

    def __init__(self, asLine):
        splitLine = asLine.split(" - ")
        self.IP = splitLine[1]
        self.TTL = int(splitLine[0])
        self.trail = splitLine[2][1:-1]
        self.typeStr = splitLine[3]
        self.type = self.PIVOT
        if "Contra-pivot" in self.typeStr:
            self.type = self.CONTRAPIVOT
        elif "Outlier" in self.typeStr:
            self.type = self.OUTLIER
    
    def toString(self):
        asString = str(self.TTL) + " - "
        asString += self.IP + " - "
        asString += "[" + self.trail + "] - "
        asString += self.typeStr
        return asString
    
    def isContrapivot(self):
        if self.type == self.CONTRAPIVOT:
            return True
        return False
   
    def isOutlier(self):
        if self.type == self.OUTLIER:
            return True
        return False

class Subnet:
    '''
    Class to model a full subnet.

    Comments:
        * The interfaces are provided as a list of SubnetInterface objects, completed with side 
          lists containing the indexes (in the full list) of: pivots, contra-pivots and outliers. 
          This allows to quickly know how many interfaces of each type are present.
    '''

    def __init__(self, subnetCIDR, altCIDR=""):
        self.CIDR = subnetCIDR
        self.altCIDR = ""
        if altCIDR:
            self.altCIDR = altCIDR
        self.stop = "" # Not known at first
        self.interfaces = [] # Full list of interfaces (SubnetInterface objects)
        self.pivots = [] # Indexes of pivots
        self.contrapivots = [] # Ditto for contra-pivots
        self.outliers = [] # Ditto for outliers
    
    def addInterface(self, interface):
        self.interfaces.append(interface)
        index = len(self.interfaces) - 1
        if interface.isContrapivot():
            self.contrapivots.append(index)
        elif interface.isOutlier():
            self.outliers.append(index)
        else:
            self.pivots.append(index)
        
    def countInterfaces(self):
        return len(self.interfaces)
    
    def countPivots(self):
        return len(self.pivots)
    
    def countContrapivots(self):
        return len(self.contrapivots)
    
    def countOutliers(self):
        return len(self.outliers)
    
    def getMaxNbInterfaces(self, adjustedPrefix = False):
        splitPrefix = None
        if not adjustedPrefix:
            splitPrefix = self.CIDR.split('/')
        else:
            splitPrefix = self.altCIDR.split('/')
        prefixLength = int(splitPrefix[1])
        if prefixLength > 32: # In case of misuse
            return 1
        return pow(2, 32 - prefixLength)
    
    def setStop(self, stop):
        self.stop = stop
    
    def getKeyInDictionary(self):
        if self.altCIDR:
            return self.altCIDR
        return self.CIDR
    
    def getCIDR(self, showAdjustedPrefix = False):
        asString = ""
        if self.altCIDR and showAdjustedPrefix:
            asString = self.altCIDR + " (adjusted from " + self.CIDR + ")"
        else:
            asString = self.CIDR
        return asString
    
    def getRawCIDR(self):
        return self.CIDR
    
    def getAdjustedCIDR(self):
        if self.altCIDR:
            return self.altCIDR
        return self.CIDR
    
    def toString(self):
        asString = ""
        if self.altCIDR:
            asString = self.altCIDR + " (adjusted from " + self.CIDR + ")\n"
        else:
            asString = self.CIDR + "\n"
        for i in range(0, len(self.interfaces)):
            asString += self.interfaces[i].toString() + "\n"
        if self.stop:
            asString += self.stop + "\n"
        return asString
    
    def isSound(self):
        totalInterfaces = len(self.interfaces)
        ratioPivots = len(self.pivots) / float(totalInterfaces)
        if len(self.pivots) == 1 and (len(self.contrapivots) == 1 or len(self.contrapivots) == 2):
            return True
        if len(self.contrapivots) > 0 and ratioPivots > 0.66: # 66% is arbitrary; edit if needed
            return True
        return False

def parseSubnets(lines):
    '''
    Function which turns a set of lines into Subnet objects, as a dictionary mapping a CIDR 
    notation to a Subnet object. A full (ordered) list of the CIDR notations is also returned.
    
    :param lines: A list of lines describing subnets
    :return: A dictionnary of subnets (CIDR -> Subnet object) and an ordered list of CIDR notations
    '''
    curSubnet = None
    parsedSubnets = dict()
    CIDRsList = []
    for i in range(0, len(lines)):
        # Empty line (delimits subnets)
        if not lines[i]:
            if curSubnet != None:
                key = curSubnet.getKeyInDictionary()
                parsedSubnets[key] = curSubnet
                if curSubnet.getRawCIDR() != key:
                    parsedSubnets[curSubnet.getRawCIDR()] = curSubnet
                CIDRsList.append(key)
                curSubnet = None
            continue
        # Subnet interface
        if curSubnet != None:
            if "Stop: " in lines[i]:
                curSubnet.setStop(lines[i])
                continue
            curSubnet.addInterface(SubnetInterface(lines[i]))
        # Subnet CIDR notation
        else:
            subnetPrefix = lines[i]
            adjustedPrefix = "" # To be dealt with later
            if "adjusted" in lines[i]:
                prefixSplit = lines[i].split(" (adjusted from ")
                adjustedPrefix = prefixSplit[0]
                subnetPrefix = prefixSplit[1][:-1]
                curSubnet = Subnet(subnetPrefix, adjustedPrefix)
            else:
                curSubnet = Subnet(subnetPrefix)
    
    # Returns the parsed subnets
    return parsedSubnets, CIDRsList
