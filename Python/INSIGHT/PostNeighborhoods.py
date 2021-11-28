#! /usr/bin/env python

'''
This module provides a class to model "post-neighborhoods", i.e., neighborhoods that were 
post-processed with alias resolution data captured during the final stage of SAGE. It is involved 
in the construction of tripartite graphs (Layer-2/Layer-3/Subnet-level). Several additional 
functions are also provided to take advantage of post-neighborhoods to discover the edges of a 
corresponding tripartite graph and output them as text files.

(tripartite graphs only)
'''

import Routers

class PostNeighborhood:
    '''
    Class which models a post-neighborhood, as a collection of Router objects, each having its own 
    adjacencies.
    
    Comments:
        * A special boolean tells whether the post-neighborhood consists of a single router. In 
          some cases, this avoids the creation of a side imaginary router for subnets without 
          contra-pivot interfaces.
        * Router objects including IP interfaces that identify the initial neighborhoods are also 
          listed in a special dictionary, involved in the final steps of the construction of the 
          tripartite graph (i.e., following the edges of the parent DAG to connect neighborhoods 
          with their peers). Remark: by design, with SAGE, there should be only one entry router, 
          but temporar failures during the final alias resolution step can change the results 
          (final step does not take advantage of alias transivity heuristic for now).
        * The constructor is not responsible for telling whether the (post-)neighborhood consists 
          of a single device or not. It's up to the function that actually post-processes a 
          neighborhood and creates the associated Router objects.
    '''
    
    def __init__(self, neighborhood):
        self.ID = neighborhood.getID()
        fullLabel = neighborhood.getLabel()
        firstSplit = fullLabel.split(" - {")
        secondSplit = firstSplit[1].split("}")
        labelStr = secondSplit[0]
        if neighborhood.isCluster() or (", " in labelStr and "Echo" not in labelStr):
            labelsList = labelStr.split(", ")
            for i in range(0, len(labelsList)): # Removes trailing (B) or (F)
                if " " in labelsList[i]:
                    thirdSplit = labelsList[i].split(" ")
                    labelsList[i] = thirdSplit[0]
            self.labels = labelsList
        else:
            self.labels = [labelStr]
        self.entryRouters = dict()
        self.defaultEntry = None # First router inserted in self.entryRouters
        self.routers = [] # Only includes "real" routers
        self.imaginary = None # Ref to imaginary router (if any)
        self.singleRouter = False
    
    # Tells if the initial neighborhood is best effort (trail with anomalies or "rule 3" subnets)
    def isBestEffort(self):
        if len(self.labels) > 1:
            return False
        singleLabel = self.labels[0]
        if "|" in singleLabel or "Echo" in singleLabel:
            return True
        return False
    
    def getLabels(self):
        return self.labels
    
    def getRouters(self):
        return self.routers
    
    def getNbRouters(self):
        nb = len(self.routers)
        if self.imaginary != None:
            nb += 1
        return nb
    
    def addRouter(self, router):
        rIPs = router.getInterfaces()
        labelsSet = set(self.labels)
        for i in range(0, len(rIPs)):
            if rIPs[i] in labelsSet:
                if len(self.entryRouters) == 0:
                    self.defaultEntry = router
                self.entryRouters[rIPs[i]] = router
        self.routers.append(router)
    
    def setAsUniqueRouter(self):
        self.singleRouter = True
    
    def isUniqueRouter(self):
        return self.singleRouter
    
    def isImaginaryRouter(self):
        return self.singleRouter and self.imaginary != None
    
    def setImaginaryRouter(self, router):
        self.imaginary = router
    
    # Gets the router to bind with remote links and indirect links
    def getDefaultEntryRouter(self):
        if self.singleRouter: # Returns the single router in any case
            if self.imaginary != None:
                return self.imaginary
            else:
                return self.routers[0]
        elif self.imaginary != None:
            return self.imaginary
        if len(self.entryRouters) > 0: # Empty for best effort neighborhoods (-> self.routers)
            return self.defaultEntry
        return self.routers[0]
    
    # Gets router to which subnets mapped to incident edges should be connected
    def getEntryRouter(self, IP):
        if self.singleRouter: # Returns the single router in any case
            if self.imaginary != None:
                return self.imaginary
            else:
                return self.routers[0]
        if IP not in self.entryRouters:
            return self.imaginary
        return self.entryRouters[IP]
    
    # Methods enumerating vertices/edges
    def listEdgesL3S(self): # L3S = Layer-3/Subnet
        edges = []
        for i in range(0, len(self.routers)):
            subEdges = self.routers[i].listSubnetEdges()
            for j in range(0, len(subEdges)):
                edges.append(subEdges[j])
        if self.imaginary != None:
            subEdges = self.imaginary.listSubnetEdges()
            for j in range(0, len(subEdges)):
                edges.append(subEdges[j])
        return edges
    
    def listEdgesL2L3(self, providedID=0):
        edges = []
        if self.singleRouter:
            return edges
        labelSwitch = "E"
        if providedID == 0:
            labelSwitch += str(self.ID)
        else:
            labelSwitch += str(providedID)
        for i in range(0, len(self.routers)):
            edges.append([labelSwitch, self.routers[i].getLabel()])
        if self.imaginary != None:
            edges.append([labelSwitch, self.imaginary.getLabel()])
        return edges
    
    # Output methods
    def routersToString(self):
        routersStr = "N" + str(self.ID) + ": "
        if self.singleRouter and self.imaginary != None:
            routersStr += self.imaginary.getLabel() + " (hypothetical)"
            return routersStr
        for i in range(0, len(self.routers)):
            if i > 0:
                routersStr += ", "
            routersStr += self.routers[i].getLabel()
        if self.imaginary != None:
            routersStr += " + " + self.imaginary.getLabel()
        return routersStr
    
    def L3SToString(self): # L3S = Layer-3/Subnet
        edges = self.listEdgesL3S()
        edgesStr = ""
        for i in range(0, len(edges)):
            edgesStr += edges[i][0] + " - " + edges[i][1] + "\n"
        return edgesStr
    
    def L2L3ToString(self, providedID=0):
        edges = self.listEdgesL2L3(providedID)
        edgesStr = ""
        for i in range(0, len(edges)):
            edgesStr += edges[i][0] + " - " + edges[i][1] + "\n"
        return edgesStr

def subnetOverlaps(subnetCIDR, IP):
    '''
    Function testing whether a given IP belongs to a given subnet, provided as a CIDR/IPv4 block. 
    It consists in computing the limits of the given IPv4 block (as 32-bit integers) and checking 
    whether the IP (still as a 32-bit integer) is included in the interval bounded by both limits.
    
    :param subnetCIDR:  The CIDR block notation denoting a subnet
    :param IP:          The IP address to test (whether it belongs to the provided subnet)
    :return:  True if the provided IP address belongs to the given subnet
    '''
    splitPrefix = subnetCIDR.split('/')
    prefixIP = splitPrefix[0]
    prefixLength = int(splitPrefix[1])
    nbInterfaces = pow(2, 32 - prefixLength)
    
    splitIP = IP.split(".")
    inputAsInt = int(splitIP[0]) * 256 * 256 * 256
    inputAsInt += int(splitIP[1]) * 256 * 256
    inputAsInt += int(splitIP[2]) * 256
    inputAsInt += int(splitIP[3])
    
    splitPrefix = prefixIP.split(".")
    prefixAsInt = int(splitPrefix[0]) * 256 * 256 * 256
    prefixAsInt += int(splitPrefix[1]) * 256 * 256
    prefixAsInt += int(splitPrefix[2]) * 256
    prefixAsInt += int(splitPrefix[3])
    return inputAsInt >= prefixAsInt and inputAsInt <= (prefixAsInt + nbInterfaces)

def postProcess(neighborhoods):
    '''
    This function receives a fully processed list of neighborhoods, i.e. complete with alias 
    resolution and subnet-level data, and turns said neighborhoods into "post-neighborhoods", 
    i.e., the equivalent of the router-level if one takes account of the alias resolution data 
    captured by the final step of SAGE. It is worth noting that the resulting post-neighborhoods 
    are not connected together, because the data from the .graph file still needs to be read in 
    order to account for the adjacencies between neighborhoods. I.e., the post-neighborhoods only 
    connect their (inferred) routers with the neighboring subnets at this stage. This function 
    also returns the mappings between subnets (as CIDR/IPv4 blocks), since they are inserted in 
    the post-neighborhoods as labels (SX where X is a unique ID), and since these mappings are 
    needed to connect specific subnets with adjacent neighborhoods (i.e., direct/indirect links in 
    the neighborhood-based DAG).
    
    :param neighborhoods:  This list of (fully annotated) neighborhoods to post-process
    :return:  A dictionnary of PostNeighborhood objects (neighborhood ID => "post" object) and two 
              dictionaries of subnet mappings (ID in "post" data => CIDR and opposite)
    '''
    routerID = 1 # Counter for the unique IDs associated to routers
    hypotRouterID = 1 # Counter for the unique IDs associated to hypothetical routers
    subnetID = 1 # Ditto for subnets
    postNeighborhoods = dict() # One ID => one post-neighborhood
    subnetMappings = dict() # ID => CIDR
    revSubnetMappings = dict() # CIDR => ID
    
    for i in range(0, len(neighborhoods)):
        # Gets useful components of the neighborhood
        current = neighborhoods[i]
        aliasLists = current.getAliases()
        subnets = current.getSubnets() # As NeighborSubnet objects (getCIDR() available)
        
        # Instantiates post-neighborhood (will be completed by next instructions)
        post = PostNeighborhood(current)
        
        # Cases for which only one router is created (zero or one alias pair/list)
        if current.isSingleRouter() or current.isAnonymousRouter():
            # Creates the router
            newRouter = None
            if current.isAnonymousRouter():
                newRouter = Routers.Router(hypotRouterID) # Imaginary router
                hypotRouterID += 1
            else:
                newRouter = Routers.Router(routerID, aliasLists[0])
                routerID += 1
            
            # Sets the post-neighborhood as a unique router
            post.setAsUniqueRouter()
            if current.isAnonymousRouter():
                post.setImaginaryRouter(newRouter)
            else:
                post.addRouter(newRouter)
            
            # Maps all neighboring subnets to the sole Router object
            for j in range(0, len(subnets)):
                # Creates the mapping for this subnet and records it
                subnetLabel = "S" + str(subnetID)
                subnetID += 1
                subnetMappings[subnetLabel] = subnets[j].getCIDR()
                revSubnetMappings[subnets[j].getCIDR()] = subnetLabel
                
                # Maps the router with the subnet
                newRouter.addNeighboringSubnet(subnetLabel)
            
        # Cases with multiple routers
        else:
            # Creates a copy list with the CIDR/IPv4 blocks of the neighboring subnets
            remainingSubnets = []
            for j in range(0, len(subnets)):
                remainingSubnets.append(subnets[j].getCIDR())
            
            # Browses the alias pairs/lists
            for j in range(0, len(aliasLists)):
                newRouter = Routers.Router(routerID, aliasLists[j])
                routerID += 1
                
                # For each aliased interface
                rIPs = newRouter.getInterfaces()
                for k in range(0, len(rIPs)):
                    indexMapped = -1
                    # Looks for an unmapped subnet that includes it (via the CIDR notation)
                    for l in range(0, len(remainingSubnets)):
                        subnet = remainingSubnets[l]
                        if subnetOverlaps(subnet, rIPs[k]):
                            # Creates the mapping for this subnet and records it
                            subnetLabel = "S" + str(subnetID)
                            subnetID += 1
                            indexMapped = l
                            subnetMappings[subnetLabel] = subnet
                            revSubnetMappings[subnet] = subnetLabel
                            
                            # Maps the router with the subnet and gets out of loop
                            newRouter.addNeighboringSubnet(subnetLabel)
                            break
                    if indexMapped >= 0:
                        del remainingSubnets[indexMapped]
                
                # Records the router
                post.addRouter(newRouter)
            
            # If there remain subnets, creates an imaginary router to map them
            if len(remainingSubnets) > 0:
                imaginaryRouter = Routers.Router(hypotRouterID)
                hypotRouterID += 1
                post.setImaginaryRouter(imaginaryRouter)
                # Maps remaining subnets
                for j in range(0, len(remainingSubnets)):
                    subnet = remainingSubnets[j]
                    
                    # Creates the mapping for this subnet and records it
                    subnetLabel = "S" + str(subnetID)
                    subnetID += 1
                    subnetMappings[subnetLabel] = subnet
                    revSubnetMappings[subnet] = subnetLabel
                    
                    # Maps the router with the subnet
                    imaginaryRouter.addNeighboringSubnet(subnetLabel)
        
        # Records the post-neighborhood
        postNeighborhoods[current.getID()] = post
    
    # Done
    return postNeighborhoods, subnetMappings, revSubnetMappings
    
def connectNeighborhoods(postNeighborhoods, CIDRsToIDs, graphFileLines):
    '''
    This procedure completes the task started by the previous function by reading the neighborhood 
    adjacencies found in the neighborhood-based DAG described in the .graph output file produced 
    by SAGE. It therefore receives the dictionary of post-neighborhoods and the mappings between 
    CIDR/IPv4 blocks and subnet IDs returned by the postProcess() function. It also receives the 
    content of the associated .graph file to include the connections between routers and subnets 
    that correspond to the direct/indirect links of the DAG, as well as the remote links (in the 
    form of special subnets, much like neighborhood - subnet bipartite graphs). In the end, all 
    vertices and edges that should constitute a tripartite model (Layer-2/Layer-3/Subnet) are 
    known and can be written in some output file(s).
    
    :param postNeighborhoods:  The dictionary of post-neighborhoods (ID -> object) produced by the 
                               postProcess() function
    :param CIDRsToIDs:         The mappings between subnets (as CIDR/IPv4 blocks) and their labels 
                               in the post-neighborhood data, as returned by postProcess()
    :param graphFileLines:     The content of the .graph file providing the neighborhood-based DAG 
                               describing neighborhood adjacencies and their connecting subnets, 
                               as a list of lines (.graph being a text file)
    '''
    
    hypotSubnetCount = 1
    remoteLinksCount = 1
    
    # Gets the lines giving the adjacencies
    actualGraphLines = []
    blankSpaces = 0
    for i in range(0, len(graphFileLines)):
        if not graphFileLines[i]:
            blankSpaces += 1
            continue
        elif blankSpaces == 1:
            actualGraphLines.append(graphFileLines[i])
    
    # For each of these lines
    for i in range(0, len(actualGraphLines)):
        lineSplit = actualGraphLines[i].split(" ")
        
        # Badly formatted line
        if len(lineSplit) < 3 or lineSplit[1] != "->":
            continue
        
        IDu = int(lineSplit[0][1:])
        IDv = int(lineSplit[2][1:])
        
        if IDu not in postNeighborhoods or IDv not in postNeighborhoods:
            continue # Ignores the ligne (should not happen in theory; just in case)
        
        u = postNeighborhoods[IDu]
        v = postNeighborhoods[IDv]
        
        # Case of an unknown medium/remote link: creates a special subnet to connect u and v
        if lineSplit[3] == "(unknown" or lineSplit[3] == "through":
            imaginarySubnet = ""
            if lineSplit[3] == "(unknown":
                imaginarySubnet = "T" + str(hypotSubnetCount)
                hypotSubnetCount += 1
            else:
                imaginarySubnet = "U" + str(remoteLinksCount)
                remoteLinksCount += 1
            routerU = u.getDefaultEntryRouter()
            routerV = v.getDefaultEntryRouter()
            routerU.addNeighboringSubnet(imaginarySubnet)
            routerV.addIncidentSubnet(imaginarySubnet)
        # Case of a known medium
        elif lineSplit[3] == "via":
            subnetCIDR = lineSplit[4]
            # To fix a minor issue with snapshots collected prior to Sept 2020
            if v.isBestEffort():
                imaginarySubnet = "T" + str(hypotSubnetCount)
                hypotSubnetCount += 1
                routerU = u.getDefaultEntryRouter()
                routerV = v.getDefaultEntryRouter()
                routerU.addNeighboringSubnet(imaginarySubnet)
                routerV.addIncidentSubnet(imaginarySubnet)
            # Gets the subnet ID to create the mapping
            elif subnetCIDR in CIDRsToIDs:
                subnetID = CIDRsToIDs[subnetCIDR]
                # Gets the label of v that is encompassed by subnetCIDR
                labelsV = v.getLabels()
                selectedLabel = ""
                for j in range(0, len(labelsV)):
                    label = labelsV[j]
                    if subnetOverlaps(subnetCIDR, label):
                        selectedLabel = label
                        break
                if len(selectedLabel) == 0:
                    continue # Ignores the ligne (should not happen in theory; just in case)
                routerV = v.getEntryRouter(selectedLabel)
                routerV.addIncidentSubnet(subnetID)
                if len(lineSplit) == 7: # Indirect link: one additional binding is necessary
                    routerU = u.getDefaultEntryRouter()
                    routerU.addNeighboringSubnet(subnetID)
                    # N.B.: by design (cf. postProcess()), already mapped to its ingress router
            else:
                continue # Ignores the ligne (should not happen in theory; just in case)
        # Not a valid line: skips to next iteration
        else:
            continue

def outputTripartite(postNeighborhoods, IDsToCIDRs, outputFilePrefix):
    '''
    This last procedure simply writes the tripartite components produced by the postProcess() 
    function and the connectNeighborhoods() procedure into two text output files. The first file 
    (.trip-graph) describes the edges while the second file (.trip-mappings) provides the mapping 
    between each subnet/router ID in the graph and the data collected by SAGE (e.g., for each 
    subnet label, provides the associated subnet as a CIDR/IPv4 block).
    
    :param postNeighborhoods:  The dictionary of post-neighborhoods (ID -> object) produced by the 
                               postProcess() function and completed by connectNeighborhoods()
    :param IDsToCIDRs:         The mappings between subnet labels and the corresponding subnets 
                               (as CIDR/IPv4 blocks), as returned by postProcess()
    :param outputFilePrefix:   A string prefix for the output files; if denoted as [p], the 
                               resulting files will be [p].trip-graph and [p].trip-mappings
    '''
    
    # Outputs graph file
    graphString = ""
    
    # Writes (hypothetical) Layer-2 topology
    switchCount = 1
    for ID in postNeighborhoods:
        current = postNeighborhoods[ID]
        if not current.isUniqueRouter():
            graphString += current.L2L3ToString(switchCount)
            switchCount += 1
    
    if len(graphString) > 0:
        graphString += "\n"
    
    # Writes router/subnet adjacencies
    for ID in postNeighborhoods:
        graphString += postNeighborhoods[ID].L3SToString()
    
    graphFile = open(outputFilePrefix + ".trip-graph", "w")
    graphFile.write(graphString)
    graphFile.close()
    
    # Outputs mappings
    mappingsString = ""
    
    # Routers per neighborhood (only labels)
    allRouters = []
    for ID in postNeighborhoods:
        current = postNeighborhoods[ID]
        mappingsString += current.routersToString() + "\n"
        assocRouters = current.getRouters()
        for i in range(0, len(assocRouters)):
            allRouters.append(assocRouters[i]) # Should be in order (follows order of insertion)
    mappingsString += "\n"
    
    # Routers themselves (RX -> alias list; hypothetical routers not listed)
    for i in range(0, len(allRouters)):
        mappingsString += allRouters[i].toString() + "\n"
    mappingsString += "\n"
    
    # Subnets
    for ID in IDsToCIDRs: # Should be in order too (follows order of insertion)
        mappingsString += ID + " - " + IDsToCIDRs[ID] + "\n"
    
    mappingsFile = open(outputFilePrefix + ".trip-mappings", "w")
    mappingsFile.write(mappingsString)
    mappingsFile.close()
