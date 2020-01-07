/*
 * RouteHop.h
 *
 *  Created on: Sept 6, 2018
 *      Author: jefgrailet
 *
 * A simple class to represent a single hop in a route.
 *
 * N.B.: this class is equivalent to "RouteInterface" in SAGE v1.0 and late TreeNET versions, but 
 * with a few changes to be more adapted to the probing strategies of WISE/SAGE v2.0.
 */

#ifndef ROUTEHOP_H_
#define ROUTEHOP_H_

#include "../../common/inet/InetAddress.h"
#include "../../prober/structure/ProbeRecord.h"

class RouteHop
{
public:

    // Possible methods being used when this IP is associated to a router
    enum HopStates
    {
        NOT_MEASURED, // Not measured yet
        ANONYMOUS, // Tried to get it via traceroute, but only got timeout
        VIA_TRACEROUTE, // Intermediate hop obtained through traceroute
        PEERING_POINT // IP appears as the (direct) trail of some neighborhood, i.e. it's a peer
    };
    
    // Output method (N.B.: reqTTL / replyTTL not shown, only used for inferring initial TTL)
    friend ostream &operator<<(ostream &out, const RouteHop &hop)
    {
        if(hop.state == NOT_MEASURED)
            out << "Not measured";
        else if(hop.state == ANONYMOUS)
            out << "Anonymous";
        else if(hop.state == PEERING_POINT)
            out << "[" << hop.ip << "]"; // << " - " << (unsigned short) hop.replyTTL;
        else
            out << hop.ip; // << " - " << (unsigned short) hop.replyTTL;
        return out;
    }
    
    RouteHop(); // Creates a "NOT_MEASURED" route hop
    RouteHop(ProbeRecord &rec, bool peer = false);
    RouteHop(const RouteHop &other);
    ~RouteHop();
    
    /*
     * Note (October 2019): a RouteHop is built from a ProbeRecord. Initially, only the IP address 
     * was given as parameter of the (non-default) constructor, but for the needs of extended 
     * fingerprinting (and perhaps other future needs), it was necessary to store also the TTL 
     * of the probe and the TTL of the reply, and therefore more convenient to use a pointer to a 
     * ProbeRecord rather than several individual parameters.
     */
     
    // Normally added implicitely by compiler; added explicitely to signal its use
    RouteHop &operator=(const RouteHop &other);
    
    // Short methods to avoid using "RouteHop::STATE" in other parts of the code
    inline bool isUnset() { return state == NOT_MEASURED; }
    inline bool isAnonymous() { return state == ANONYMOUS; }
    inline bool isValidHop() { return state == VIA_TRACEROUTE || state == PEERING_POINT; }
    inline bool isPeer() { return state == PEERING_POINT; }
    
    InetAddress ip;
    unsigned short state;
    unsigned char reqTTL;
    unsigned char replyTTL;

};

#endif /* ROUTEHOP_H_ */
