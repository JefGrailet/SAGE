/*
 * IPTableEntry.h
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file defines a class "IPTableEntry" which extends the InetAddress class, and constitutes, 
 * as the name suggests, an entry of the IP dictionnary (IPLookUpTable). It's an extension of the 
 * InetAddress class to add fields to maintain data collected during probing, such as the timeout 
 * delay with which the first successful probe to this IP was obtained or alias resolution data.
 */

#ifndef IPTABLEENTRY_H_
#define IPTABLEENTRY_H_

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <list>
using std::list;

#include "../../common/date/TimeVal.h"
#include "../../common/inet/InetAddress.h"
#include "RouteHop.h"
#include "Trail.h"
#include "AliasHints.h"

class IPTableEntry : public InetAddress
{
public:
    
    // Misc. constants
    const static unsigned char NO_KNOWN_TTL = (unsigned char) 255; // Default when TTL is unknown
    const static unsigned long DEFAULT_TIMEOUT_SECONDS = 2;
    
    // "void" objects to handle absence of alias resolution hints/trail
    static AliasHints VOID_HINTS;
    static Trail VOID_TRAIL;
    
    // Constants telling what kind of IP we have
    const static unsigned short RESPONSIVE_TARGET = 0; // Default value
    const static unsigned short SUCCESSFULLY_SCANNED = 1; // Parts of the targets and scanned
    const static unsigned short UNSUCCESSFULLY_SCANNED = 2; // Only responsive at pre-scanning
    const static unsigned short SEEN_IN_TRAIL = 3; // Not a target (not scanned) but in a trail
    const static unsigned short SEEN_WITH_TRACEROUTE = 4; // Only seen in traceroute (for later)

    // Constructor, destructor
    IPTableEntry(InetAddress ip);
    ~IPTableEntry();
    
    /*** General methods ***/
    
    // Accessers/setters
    inline unsigned char getTTL() { return TTL; } // Returns the minimum observed TTL
    inline TimeVal getPreferredTimeout() { return preferredTimeout; }
    inline unsigned short getType() { return type; }
    inline unsigned char getTimeExceedediTTL() { return timeExceedediTTL; }
    
    inline void setTTL(unsigned char TTL) { this->TTL = TTL; }
    inline void setPreferredTimeout(TimeVal timeout) { preferredTimeout = timeout; }
    inline void setType(unsigned short type) { this->type = type; }
    inline void setTimeExceedediTTL(unsigned char iTTL) { timeExceedediTTL = iTTL; }
    inline void setRoute(vector<RouteHop> route) { this->route = route; } // Same as above
    
    // Methods to handle the different TTLs at which this IP has been seen (as dest. or route hop)
    inline bool sameTTL(unsigned char t) { return (TTL != 0 && TTL == t); }
    bool hasTTL(unsigned char t);
    void recordTTL(unsigned char t);
    void pickMinimumTTL();
    inline size_t getNbTTLs() { return TTLs.size(); }
    inline static bool compareTTLs(unsigned char t1, unsigned char t2) { return t1 < t2; }
    
    // Comparison methods (for sorting)
    static bool compare(IPTableEntry *ip1, IPTableEntry *ip2);
    static bool compareWithTTL(IPTableEntry *ip1, IPTableEntry *ip2);
    static bool compareLattestFingerprints(IPTableEntry *ip1, IPTableEntry *ip2); // Alias res.
    
    // Output methods
    string toString();
    string toStringSimple(); // Displays only the IP
    
    /*** Route and trail handling ***/
    
    inline vector<RouteHop> &getRoute() { return route; }
    inline Trail &getTrail() { return trail; }
    
    void initRoute(); // Route length from TTL (nothing happens if unset); deletes previous route
    bool anonymousEndOfRoute(); // Returns true if the last hop in the route is anonymous
    bool setTrail(); // Returns false if trail cannot be set yet
    bool hasCompleteRoute(); // Returns true if a full traceroute towards this IP has been done
    string routeToString(); // Returns the route in string format (TTL - hop\n)
    
    /*** Special IPs ***/
    
    inline bool isInTrail() { return trailIP; }
    inline bool isWarping() { return warping; }
    inline bool isFlickering() { return flickering; }
    inline bool isEchoing() { return echoing; }
    inline list<InetAddress> *getFlickeringPeers() { return &flickeringPeers; }
    
    // Used for neighborhood peer discovery
    inline bool denotesNeighborhood() { return denotingNeighborhood; }
    inline bool isABlindspot() { return blindspot; }
    
    inline void setAsTrailIP() { trailIP = true; }
    inline void setAsWarping() { warping = true; }
    inline void setAsFlickering() { flickering = true; }
    inline void setAsEchoing() { echoing = true; }
    
    // Used for neighborhood peer discovery
    inline void setAsDenotingNeighborhood() { denotingNeighborhood = true; }
    inline void setAsBlindspot() { blindspot = true; }
    
    void addFlickeringPeer(InetAddress peer);
    
    /*** Alias resolution hints ***/
    
    inline size_t getNbHints() { return ARHints.size(); }
    inline void addHints(AliasHints hints) { ARHints.push_back(hints); }
    
    // N.B.: we consider hints always come in order regarding probing stage.
    
    AliasHints &getLattestHints();
    string hintsToString();
    
private:

    // Basic details
    unsigned char TTL; // TTL estimated at scanning if target; otherwise shortest TTL (traceroute)
    TimeVal preferredTimeout;
    list<unsigned char> TTLs; // Lists all TTLs at which this IP has been observed
    unsigned short type; // See public constants
    
    // "Time exceeded" inferred initial TTL (for alias resolution), found after scanning
    unsigned char timeExceedediTTL;
    
    // Route observed during scanning and inferred trail
    vector<RouteHop> route;
    Trail trail;
    
    /*
     * Flags used to advertise whether this IP is in a trail, and if yes, if it's a potential 
     * candidate for neighborhood peering (see neighborhoods/ module), if it's warping, flickering 
     * and/or echoing. These flags are initially set to false and needs to be set to true (when 
     * relevant) with some post-processing of the IP dictionary.
     */
   
    bool trailIP, warping, flickering, echoing;
    bool denotingNeighborhood;
    bool blindspot; // Unique to SAGE
    
    // List of IPs that are flickering with this one
    list<InetAddress> flickeringPeers;
    
    // Collection of alias resolution hints
    list<AliasHints> ARHints;

};

#endif /* IPTABLEENTRY_H_ */
