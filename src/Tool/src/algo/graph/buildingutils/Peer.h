/*
 * Peer.h
 *
 *  Created on: Nov 20, 2019
 *      Author: jefgrailet
 *
 * This class models a "peer", which can be roughly described here as a neighborhood appearing in 
 * the topology prior to reaching other neighborhoods. It consists in a list of IPs, these IPs 
 * being used to aggregate subnets in the first place. In simple situations, the list will usually 
 * contain only one IP; a Peer object with several IPs can correspond to:
 * -an aggregate of subnets built on the basis of flickering IPs that have been aliased, 
 * -several aggregates gathered together because their respective "identifying IPs" were aliased.
 *
 * In this context, a Peer is merely a gathering of IPs acting as labels for aggregates to prepare 
 * the construction of the actual neighborhoods of a network (neighborhood-based) graph; it is 
 * therefore much simpler than the class of the same name in SAGE v1.
 */

#ifndef PEER_H_
#define PEER_H_

#include "../../structure/Alias.h"

class Peer
{
public:

    // Constructors and destructor
    Peer(InetAddress peerIP);
    Peer(Alias *alias);
    ~Peer();
    
    inline list<InetAddress> *getInterfaces() { return &interfaces; }
    
    // Utility methods
    bool contains(InetAddress interface);
    static bool compare(Peer *p1, Peer *p2);
    string toString();
    
private:
    
    list<InetAddress> interfaces;
    
};

#endif /* PEER_H_ */
