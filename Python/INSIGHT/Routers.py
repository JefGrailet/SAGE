#! /usr/bin/env python

'''
This module provides a class to model routers in a tripartite graph, i.e., combination of an alias 
pair/list with the subnets this router is supposed to be connected with.

(tripartite graphs only)
'''

class Router:
    '''
    Class which models a single router, consisting of an alias list and the surrounding subnets.
    
    Comments:
        * A special boolean tells whether the router is "imaginary" or not. Imaginary routers are 
          used to connect subnets from a given neighborhood that cannot be mapped to any 
          identified router and to model the "entry" router of a best effort neighborhood. It is 
          worth noting such imaginary routers are only created when the neighborhood already 
          consists of several alias pairs/lists: if there's only one, it's reasonable to assume 
          the neighborhood is actually a single router, which can be connected to all neighboring 
          subnets and/or can assume the role of the "entry" router.
        * Subnets are split in two sets: one lists the subnets the router is (allegedly) connected 
          to that were found in the originating neighborhood while the other set gathers subnets 
          connected to the router via the data gathered in the parent neighborhood-based DAG. The 
          former set gathers "neighboring" subnets while the latter gathers "incident" subnets.
        * Subnets inserted here are not the equivalent objects, but their identifiers in string 
          format (SX where X is a unique integer).
    '''
    
    def __init__(self, number, initialAlias = None):
        self.ID = number
        self.alias = initialAlias
        if initialAlias != None:
            self.imaginary = False
        else:
            self.imaginary = True
        self.allSubnets = set() # To avoid duplicate links ((in)direct links with same subnet)
        self.nSubnets = [] # n -> neighboring
        self.iSubnets = [] # i -> incident
    
    def getInterfaces(self):
        return self.alias.getInterfaces()
    
    def getLabel(self):
        if self.imaginary:
            return "I" + str(self.ID)
        return "R" + str(self.ID)
    
    def toString(self):
        asString = ""
        if not self.imaginary:
            asString = "R" + str(self.ID) + " - " + self.alias.toString()
        else:
            asString = "I" + str(self.ID) + " - Hypothetical"
        return asString
    
    def addNeighboringSubnet(self, subnet):
        if subnet not in self.allSubnets:
            self.nSubnets.append(subnet)
            self.allSubnets.add(subnet)
    
    def addIncidentSubnet(self, subnet):
        if subnet not in self.allSubnets:
            self.iSubnets.append(subnet)
            self.allSubnets.add(subnet)
    
    def listSubnetEdges(self):
        edges = []
        labelStr = self.getLabel()
        for i in range(0, len(self.iSubnets)):
            edges.append([labelStr, self.iSubnets[i]])
        for i in range(0, len(self.nSubnets)):
            edges.append([labelStr, self.nSubnets[i]])
        return edges

# TODO: function to build a router if multi-counter several + "cheat" with the Alias constructor
