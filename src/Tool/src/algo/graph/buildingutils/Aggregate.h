/*
 * Aggregate.h
 *
 *  Created on: Aug 22, 2019
 *      Author: jefgrailet
 *
 * This class models an aggregate, which is the most basic form of neighborhood, i.e., a group of 
 * subnets which are located at at most one hop away from each other. Aggregates are inferred on 
 * the basis of trails; i.e., if the last hop(s) before reaching a set of subnets are identical, 
 * then the last router crossed before reaching these subnets is likely the same or part of a 
 * mesh, and therefore subnets are in the vicinity of each other. The data collected along subnets 
 * can later be used to dig deeper into the topology of the target domain.
 *
 * Subnets are of course typically aggregated when they have the same trail, but due to echoing 
 * and flickering trails, there are some situations where aggregation can't just rely on the same 
 * trail. For echoing subnets, "pre-trail" IPs are used as a basis, while flickering IPs and their 
 * aliases are used to build aggregates for subnets built with rules 4 and 5 of subnet inference.
 * 
 * Like in SAGE v1, the Aggregate class kind of works like a "proto-neighborhood", i.e. it's not 
 * necessarily the final neighborhood in the network graph but rather a "work in progress" object 
 * which notably provides several methods to discover and manage the peers (see below).
 */

#ifndef AGGREGATE_H_
#define AGGREGATE_H_

#include <map>
using std::map;

#include "../../Environment.h"
#include "Peer.h"

class Aggregate
{
public:

    // Constructors/destructor
    Aggregate(Subnet *first);
    Aggregate(list<Subnet*> subnets);
    ~Aggregate();
    
    /*
     * N.B.: next methods don't check the subnets are in the same neighborhood. It's up to the 
     * calling code to verify this; next methods only inserts the subnets and updates the list of 
     * trails accordingly. It is also assumed, for addSubnets(), that the provided subnets all 
     * have the same trail.
     */
    
    void addSubnet(Subnet *newSubnet);
    void addSubnets(list<Subnet*> newSubnets);
    
    /*
     * Next methods are used to accurately discover peer IPs prior to creating the Peer objects.
     * -To call once the aggregate has been fully built, discoverPeerIPs() is responsible for 
     *  listing potential peer IPs as well as IPs appearing at the same offset in partial routes.
     * -Before peer disambiguation, adjustPeerIPs() ensure that IPs belonging to previously 
     *  discovered aliases are all replaced with a same IP from the associated alias, as a way to 
     *  prevent ambiguous situations during graph building (more details in TopologyInferrer.cpp). 
     *  Returns true if such an adjustment took place.
     * -To call after peer disambiguation, discoverBlindspots() is responsible for checking if any 
     *  of the "miscellaneous IPs" was aliased with an actual peer IP, using the aliases 
     *  discovered during peer disambiguation (it is assumed "aliases" is that set). If yes, these 
     *  IPs are considered as being "blindspots" and must be labelled as such.
     * -To call after blindspot identification, improvePeerIPs() reviews again the partial routes 
     *  to see if we can find better peer IP(s) for this aggregate. The Environment object is 
     *  needed to check all existing sets of  aliases when checking a hop can be considered as a 
     *  peer (previous method only needing the set built during peer disambiguation).
     * -To call at the start of vertex creation, listFinalPeers() visits a map giving matchings 
     *  InetAddress -> Peer* to get the final peers for this aggregate. They differ in the sense 
     *  that the Peer objects can consist of one IP or several IPs previously aliased together.
     */
    
    void discoverPeerIPs();
    bool adjustPeerIPs(AliasSet *aliases);
    void discoverBlindspots(IPLookUpTable *dict, AliasSet *aliases);
    void improvePeerIPs(Environment &env);
    void listFinalPeers(map<InetAddress, Peer*> peersMap);
    
    /*
     * Methods to handle peer/label IPs or to count them.
     * -listInitialPeerIPs() lists peer IPs along with "miscellaneous IPs", i.e. IPs not 
     *  identifying a neighborhood so far that appear at the same distance as peer IPs.
     * -listTruePeerIPs() does the same job but takes "blindspots" instead of miscellaneous IPs. 
     *  A blindspot is a formerly miscellaneous IP that has been aliased with a peer IP during 
     *  topology inference.
     * -listIdentifyingIPs() lists the IPs from direct trail(s) of this aggregate. It's used 
     *  during vertex creation to identify "terminus" neighborhoods.
     */
    
    list<InetAddress> listInitialPeerIPs();
    list<InetAddress> listTruePeerIPs();
    list<InetAddress> listIdentifyingIPs();
    inline size_t getNbPeerIPs() { return peerIPs.size(); }
    inline size_t getNbMiscellaneousIPs() { return miscellaneousIPs.size(); }
    inline size_t getNbBlindspots() { return blindspots.size(); }
    
    // Various accessers/setters
    inline list<Trail> *getTrails() { return &trails; }
    inline list<InetAddress> *getPreEchoing() { return &preEchoing; }
    inline unsigned short getPreEchoingOffset() { return preEchoingOffset; }
    inline list<Subnet*> *getSubnets() { return &subnets; }
    inline bool hasPeers() { return finalPeers.size() > 0; }
    inline unsigned short getPeersOffset() { return peersOffset; }
    inline list<Peer*> *getFinalPeers() { return &finalPeers; }
    inline void setPreEchoingOffset(unsigned short offset) { preEchoingOffset = offset; }
    inline void setPreEchoing(list<InetAddress> preEchoing) { this->preEchoing = preEchoing; }
    inline unsigned short getNbSubnets() { return (unsigned short) subnets.size(); }
    
    // Comparison method for sorting
    static bool compare(Aggregate *a1, Aggregate *a2);
    
    // String methods
    string getLabel();
    string toString();

private:
    
    // Main fields
    list<Subnet*> subnets;
    
    /*
     * "trails" stores the trail(s) of the aggregated subnets. It can have multiple trails due to 
     * the existence of flickering trails, but most of the time the list will contain only one 
     * entry. In the case of an aggregate of subnets based on the 3rd rule of inference (echo 
     * rule), "trails" is left empty since there's no common trail, and the "label" of the 
     * aggregate (i.e. string returned by the getLabel() method) will be rather based on the 
     * the TTL of the pivot IPs (always the same in this case) and "pre-echoing" IPs, i.e., the 
     * first non-anonymous IPs appearing before the echoing trails in the partial routes (which 
     * can also be, but are not always, peers).
     */
    
    list<Trail> trails;
    list<InetAddress> preEchoing;
    unsigned short preEchoingOffset;
    
    /*
     * Fields used to handle peers:
     * -peersOffset gives the difference in TTL between the trail(s) of the subnet interfaces and 
     *  the first peer IP(s). Ideally, it should be equal to 0 (direct peer(s)).
     * -peerIPs lists the IPs that strictly fulfill the definition of peer for the given offset.
     * -miscellaneousIPs lists IPs that appear at the same distance in partial routes but which 
     *  don't match an IP in the dictionary that is used to denote an aggregate/neighborhood.
     * -blindspots is used to eventually list "miscellaneous IPs" (see above) that were aliased 
     *  with peer IPs during peer disambiguation (see TopologyInferrer). Blindspots are IPs that 
     *  technically fulfill the same role as peers but couldn't appear as labels for neighborhoods 
     *  due to traffic engineering (e.g. load-balancing). If an IP found in "miscellaneousIPs" is 
     *  identified as a blindspot, it's moved from this list to "blindspots" and therefore doesn't 
     *  appear at the same time in both lists.
     * -finally, "finalPeers" lists the peers as Peer objects, which can model a peer made of a 
     *  single IP or made of a list of IPs (previously aliased together). Keeping track of these 
     *  Peer objects in Aggregate simplifies subsequent steps of the network graph building.
     */
    
    unsigned short peersOffset;
    list<InetAddress> peerIPs, miscellaneousIPs, blindspots;
    list<Peer*> finalPeers;
    
    // Private method(s)
    void updateTrails(Trail newTrail);

};

#endif /* AGGREGATE_H_ */
