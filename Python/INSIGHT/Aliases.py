#! /usr/bin/env python

'''
This module provides a class and two functions to handle aliases as discovered by SAGE and as 
provided by the .aliases-f output files.
'''

class Alias:
    '''
    Class which models a single alias, consisting of a list of IPs associated to a neighborhood 
    and an alias method.
    
    Comments:
        * The absence of a valid alias method (i.e., the "method" instance variable is an empty 
          string) combined with a list containing a single IP amounts to an unaliased IP interface.
        * A list of IPs without a valid method is considered to be an alias based on a unknown 
          method (unlikely to happen with output files as produced by SAGE).
    '''
    
    def __init__(self, nID, aliasLine):
        self.neighborhoodID = nID
        self.method = ""
        if " (" in aliasLine:
            splitLine = aliasLine.split(" (")
            self.method = splitLine[1][:-1]
            aliasLine = splitLine[0]
        self.interfaces = aliasLine.split(", ")
    
    def countInterfaces(self):
        return len(self.interfaces)
    
    def isOrphan(self):
        return len(self.interfaces) == 1
    
    def getInterfaces(self):
        return self.interfaces
    
    def hasInterface(self, IP):
        for i in range(0, len(self.interfaces)):
            if self.interfaces[i] == IP:
                return True
        return False
    
    def toString(self):
        if len(self.interfaces) == 0:
            return ""
        asString = ", ".join(self.interfaces)
        if self.method:
            asString += " (" + self.method + ")"
        return asString

def parseAliases(lines):
    '''
    Function which turns a set of lines into Alias objects, returned as a list. The data contained 
    in each object is enough to later map aliases to neighborhoods.
    
    :param lines: A list of lines describing the discovered alias pairs/lists
    :return: A list of Alias objects, in the same order as they appear in the input "lines" list
    '''
    curNeighborhood = 0
    aliases = []
    for i in range(0, len(lines)):
        if not lines[i]: # Empty line (delimits neighborhoods)
            curNeighborhood = 0
            continue
        if curNeighborhood != 0: # Alias line
            aliases.append(Alias(curNeighborhood, lines[i]))
        else: # Neighborhood ID
            lineSplit = lines[i].split(" - ")
            curNeighborhood = int(lineSplit[0][1:])
    
    # Returns the list of parsed aliases
    return aliases

def mapAliases(aliases, neighborhoods):
    '''
    Function which maps previously parsed aliases (as Alias objects) to Neighborhood objects, 
    provided as a list where each index i is mapped to neighborhood N(i+1). Nothing is 
    returned.
    
    :param aliases: A list of previously parsed aliases
    :param neighborhoods: A list of previously parsed neighborhoods
    '''
    for i in range(0, len(aliases)):
        neighborhoods[aliases[i].neighborhoodID - 1].addAlias(aliases[i])
    return None
