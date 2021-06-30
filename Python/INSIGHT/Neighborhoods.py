#! /usr/bin/env python

'''
This module provides classes and a function to handle neighborhoods as discovered by SAGE.
'''

class NeighborSubnet:
    '''
    NeighborSubnet combines a subnet with aggregation details (i.e., pre-echoing/trail IPs).

    Comments:
        * The presence of aggregation details is handled with 3 constants: 0 for no aggregation 
          details (neighborhood based on a single trail), 1 for detailed trails (cluster 
          neighborhood) and 2 for detailed pre-echoing IPs (neighborhood surrounded by "rule-3" 
          subnets).
    '''
    DETAILS_NONE = 0
    DETAILS_TRAILS = 1
    DETAILS_PREECHOING = 2

    def __init__(self, subnetLine):
        subnetCIDR = subnetLine
        detailsStr = ""
        if " (" in subnetLine:
            splitLine = subnetLine.split(" (")
            subnetCIDR = splitLine[0]
            detailsStr = splitLine[1][:-1]
        self.subnet = subnetCIDR # Later replaced by a Subnet object
        self.details = self.DETAILS_NONE
        self.detailsIPs = []
        if detailsStr.startswith("trail"):
            self.details = self.DETAILS_TRAILS
        elif detailsStr.startswith("pre-echoing"):
            self.details = self.DETAILS_PREECHOING
        if self.details != self.DETAILS_NONE:
            secondSplit = detailsStr.split(": ")
            self.detailsIPs = secondSplit[1].split(", ")
    
    def mapSubnet(self, subnetsDict): # "subnetsDict" is a dictionary CIDR => subnet
        if not isinstance(self.subnet, str):
            return
        if self.subnet in subnetsDict:
            self.subnet = subnetsDict[self.subnet] # Assumed to be a Subnet object
    
    def getCIDR(self):
        if isinstance(self.subnet, str):
            return self.subnet
        return self.subnet.getRawCIDR()
    
    def toString(self):
        asString = ""
        if isinstance(self.subnet, str): # Still just a string
            asString += self.subnet
        else:
            asString += self.subnet.getAdjustedCIDR()
        if self.details != self.DETAILS_NONE:
            asString += " "
            if self.details == self.DETAILS_PREECHOING:
                asString += "(pre-echoing: "
            else:
                asString += "(trail"
                if len(self.detailsIPs) > 1:
                    asString += "s"
                asString += ": "
            for i in range(0, len(self.detailsIPs)):
                if i > 0:
                    asString += ", "
                asString += self.detailsIPs[i]
            asString += ")"
        return asString

class Neighborhood:
    '''
    Neighborhood class models a neighborhood as discovered by SAGE.

    Comments:
        * Subnets first come as CIDR's annotated with details (if any), i.e. strings. The 
          NeighborSubnet objects stores initially the CIDR as a string, later replaced by a Subnet 
          object with the help of the mapSubnets() method (see below). The same idea applies with 
          peers: they are first stored as strings (cf. "addPeer()") then mapped to Neighborhood 
          objects via mapPeers().
        * Aliases are stored in one additional list as Alias objects. They are only added after 
          fully parsing the neighborhoods and mapping the Subnet objects to them. Aliases are also 
          omitted from the regular toString() method and have their own output method.
    '''
    TYPE_REGULAR = 0
    TYPE_CLUSTER = 1
    
    def __init__(self, labelLine):
        splitLine = labelLine.split(" - ")
        self.ID = int(splitLine[0][1:]) # Removes the "N"
        self.label = ""
        self.type = self.TYPE_REGULAR
        if "(cluster)" in splitLine[1]:
            self.type = self.TYPE_CLUSTER
            self.label = splitLine[1][1:-12] # Removes the "{" and "} (cluster):"
        else:
            self.label = splitLine[1][1:-2] # Removes the "{" and "}:"
        self.neighbors = [] # NeighborSubnet objects
        self.peersOffset = 0
        self.peers = []
        self.aliases = []
    
    def getID(self):
        return self.ID
    
    def getLabel(self):
        return self.label
    
    def addSubnet(self, subnetLine):
        if isinstance(subnetLine, str):
            self.neighbors.append(NeighborSubnet(subnetLine))
    
    def mapSubnets(self, subnetsDict): # "subnetsDict" is a dictionary CIDR => subnet
        for i in range(0, len(self.neighbors)):
            self.neighbors[i].mapSubnet(subnetsDict)
    
    def getSubnets(self):
        return self.neighbors
    
    def setPeersOffset(self, offset):
        self.peersOffset = offset
    
    def addPeer(self, peer): # Peer(s) are still strings at this point
        if isinstance(peer, str):
            self.peers.append(peer)
    
    def mapPeers(self, neighborhoods): # "neighborhoods" is a list where i => Neighborhood N(i+1)
        concretePeers = []
        for i in range(0, len(self.peers)):
            splitLine = self.peers[i].split(" - {")
            if splitLine[0].startswith("Cluster") or splitLine[0].startswith("Node"): # Artefact
                continue
            peerID = int(splitLine[0][1:])
            concretePeers.append(neighborhoods[peerID - 1])
        self.peers = concretePeers
    
    def isCluster(self):
        if self.type == self.TYPE_CLUSTER:
            return True
        return False
    
    def getLabelID(self):
        return "N" + str(self.ID)
    
    def getLabel(self):
        baseLabel = "N" + str(self.ID) + " - {" + self.label + "}"
        if self.type == self.TYPE_CLUSTER:
            baseLabel += " (cluster)"
        return baseLabel
    
    def toString(self, showNbIPs = False):
        asString = self.getLabel() + ":\n"
        if len(self.neighbors) == 0: # Just in case
            return asString
        for i in range(0, len(self.neighbors)):
            neighbor = self.neighbors[i]
            asString += neighbor.toString()
            if showNbIPs: # Shows or not the amount of IPs of the neighbor subnet
                nbIPs = neighbor.subnet.countInterfaces()
                if nbIPs > 1:
                    asString += " (" + str(nbIPs) + " IPs)"
                elif nbIPs == 1:
                    asString += " (one IP)"
            asString += "\n"
        if len(self.peers) > 0:
            if len(self.peers) > 1:
                asString += "Peers"
            else:
                asString += "Peer"
            if self.peersOffset > 0:
                asString += " (offset=" + str(self.peersOffset) + ")"
            asString += ":\n"
            for i in range(0, len(self.peers)):
                if isinstance(self.peers[i], Neighborhood): # If it's still only a string
                    asString += self.peers[i].getLabel() + "\n"
                else:
                    asString += self.peers[i]
        else:
            asString += "No peers; gate to the topology.\n"
        return asString
    
    def addAlias(self, alias):
        self.aliases.append(alias) # Assumed to be an Alias object
    
    def aliasesToString(self):
        if len(self.aliases) == 0:
            return ""
        asString = self.getLabel() + ":\n"
        for i in range(0, len(self.aliases)):
            asString += self.aliases[i].toString() + "\n"
        return asString
    
    def isSingleRouter(self):
        return len(self.aliases) == 1
    
    def isAnonymousRouter(self): # Can happen with best effort neighborhoods
        return len(self.aliases) == 0
    
    def getAliases(self):
        return self.aliases

def parseNeighborhoods(lines, subnetsDict):
    '''
    Function which turns a set of lines into Neighborhood objects, as a list where index i is 
    Neighborhood N(i+1).
    
    :param lines: A list of lines describing neighborhoods
    :param subnetsDict: A dictionnary mapping a CIDR notation to a Subnet object
    :return: A list of Neighborhood objects where each index i is mapped to neighborhood N(i+1)
    '''
    curNeighborhood = None
    neighborhoods = []
    alreadySeen = set() # Prevents subnet duplicates (can happen in clusters due to a small bug)
    passedSubnets = False
    ignoreEverything = False
    for i in range(0, len(lines)):
        # Empty line (delimits neighborhoods)
        if not lines[i]:
            if curNeighborhood != None:
                curNeighborhood.mapSubnets(subnetsDict) # Finalizes by adding Subnet refs
                neighborhoods.append(curNeighborhood)
                curNeighborhood = None
                passedSubnets = False
                ignoreEverything = False
            continue
        # Ignores everything after a "No peers" line (except empty lines)
        if ignoreEverything:
            continue
        # Subnet or peer
        if curNeighborhood != None:
            if "gate" in lines[i]:
                ignoreEverything = True
                continue
            if "Peer" in lines[i]:
                passedSubnets = True # Will now add peers rather than subnets
                offsetStr = ""
                if "offset" in lines[i]:
                    splitLine = lines[i].split("offset=")
                    offsetStr = splitLine[1][:-2]
                if offsetStr:
                    curNeighborhood.setPeersOffset(int(offsetStr))
                continue
            if passedSubnets:
                curNeighborhood.addPeer(lines[i])
            else:
                prefix = lines[i]
                if " " in prefix:
                    splitLine = prefix.split(" ")
                    prefix = splitLine[0]
                if prefix in alreadySeen:
                    continue
                alreadySeen.add(prefix)
                curNeighborhood.addSubnet(lines[i])
        # Neighborhood label (see constructor for how it handles it)
        else:
            curNeighborhood = Neighborhood(lines[i])
    
    # Returns the list of parsed neighborhoods
    return neighborhoods
