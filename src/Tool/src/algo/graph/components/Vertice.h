/*
 * Vertice.h
 *
 *  Created on: Nov 22, 2019
 *      Author: jefgrailet
 *
 * Superclass modeling a vertice of a network graph. It has a design similar to that of the 
 * Neighborhood class from the same module in SAGE v1, but differs in terms of interfaces and 
 * content. In particular, it manages its peers directly as Vertice objects rather than through 
 * Peer objects (therefore acting as intermediate), which are disposed of after building the graph.
 */

#ifndef VERTICE_H_
#define VERTICE_H_

#include <map>
using std::map;

#include "../buildingutils/GraphPeers.h" // Indirectly: Subnet, Aggregate, Peer, etc.
#include "../../structure/Alias.h"

#include "Edge.h"
class Edge; // Forward declaration

class Vertice
{
public:

    // Constructor/destructor
    Vertice();
    virtual ~Vertice();
    
    // Accessers/setter
    inline unsigned int getID() { return ID; }
    inline list<Trail> *getTrails() { return &trails; }
    inline list<Subnet*> *getSubnets() { return &subnets; }
    inline list<Alias*> *getAliases() { return &aliases; }
    inline unsigned short getPeersOffset() { return peersOffset; }
    inline list<Vertice*> *getPeers() { return &peers; }
    
    inline void setID(unsigned int ID) { this->ID = ID; }
    
    // Methods to collect useful metrics
    inline unsigned short getNbPeers() { return (unsigned short) peers.size(); }
    inline unsigned short getNbSubnets() { return (unsigned short) subnets.size(); }
    inline unsigned short getNbAliases() { return (unsigned short) aliases.size(); }
    unsigned int getSubnetCoverage(); // Total of IPs encompassed by the subnet prefixes
    
    // Edges stuff
    inline list<Edge*> *getEdges() { return &edges; }
    inline void addEdge(Edge *e) { edges.push_back(e); }
    
    inline unsigned short getInDegree() { return this->getNbPeers(); }
    inline unsigned short getOutDegree() { return (unsigned short) edges.size(); }
    
    /*
     * Subsequent methods are involved in the construction of a network graph.
     */
    
    /*
     * As the name implies, next method sets the details on peers using a "GraphPeers" object and 
     * map Peer -> Vertice. The reason why listing peers as Vertice objects is not done at Vertice 
     * construction is simply because it's simpler to first build all Vertice objects while 
     * putting peering info aside, then using the latter to complete the existing Vertice objects.
     */
    
    void setPeers(GraphPeers *peersInfo, map<Peer*, Vertice*> peersMap);
    
    /*
     * Returns the subnet containing nextHop, NULL otherwise. This method is used to find out if 
     * a direct link can be created between two vertices.
     */
    
    Subnet *getConnectingSubnet(InetAddress nextHop);
    
    /*
     * Enumerates the (partial) routes (towards pivot interfaces of the aggregated subnets) which 
     * exists between this vertice and a given list of peer IPs, given as a list of Trail (for 
     * simplicity; said lists are normally the trail(s) used to identify other Vertice objects). 
     * This method is useful to complete the data tied to remote links (edges connecting vertices 
     * that are remote peers).
     */
    
    list<vector<RouteHop> > findUniqueRoutes(list<Trail> peerIPs);
    
    /*
     * Lists the interfaces (as IPTableEntry objects) which are potential alias candidates for 
     * this neighborhood, removing duplicates in the process.
     */
    
    list<IPTableEntry*> getAliasCandidates();
    
    /*
     * Stores the inferred aliases.
     */
    
    void storeAliases(list<Alias*> newAliases);
    
    // Sorting method
    static bool smallerID(Vertice *v1, Vertice *v2);
    
    // virtual list<InetAddress> listAllInterfaces() = 0;
    virtual string getFullLabel() = 0;
    virtual string toString() = 0;

protected:

    // Main fields
    unsigned int ID; // To be set after the graph has been fully generated, for output purpose only
    list<Trail> trails; // To identify the vertice by IP(s)
    
    // Data fields
    list<Subnet*> subnets;
    list<Alias*> aliases;
    
    // Peering details
    unsigned short peersOffset;
    list<Vertice*> peers;
    
    // Edges ("out" only, i.e. which the tail is this vertice)
    list<Edge*> edges;
    
    // Method to display peers (inherited by Node/Cluster)
    string peersToString();

};

#endif /* VERTICE_H_ */
