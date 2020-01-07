/*
 * PeerScanner.h
 *
 *  Created on: Aug 19, 2019
 *      Author: jefgrailet
 *
 * PeerScanner is the main component for probing furthermore subnet interfaces in order to conduct 
 * peer discovery. In WISE/SAGE terminology, a peer is an IP appearing before the trail(s) towards 
 * a subnet (or some of its interfaces) that is also appearing as (direct) trails of pivots from 
 * other subnets. A peer is said to be "direct" if there's no intermediate hop between it and the 
 * trail (otherwise, it's said to be "remote"). As a consequence, if we define a neighborhood as 
 * an aggregate of subnets which the trails for the pivots are identical, a neighborhood and its 
 * direct peer(s) are neighborhoods located next to each other in the topology of the target 
 * domain. This is a pivotal concept for establishing a graph of the target network.
 *
 * PeerScanner is responsible for listing the subnet interfaces that should be probed to discover 
 * potential peers and scheduling the threads which will actually probe said interfaces.
 */

#ifndef PEERSCANNER_H_
#define PEERSCANNER_H_

#include <utility>
using std::pair;

#include "../Environment.h"

class PeerScanner
{
public:

    // Constructor, destructor
    PeerScanner(Environment &env);
    ~PeerScanner();
    
    void scan(); // Schedules PeerDiscoveryTask threads
    void output(string filename); // Outputs partial routes found for each initial target

private:

    // Pointer to the environment singleton
    Environment &env;
    
    // List of targets
    list<pair<Subnet*, SubnetInterface*> > targets;
    
    /*
     * Remark (October 2019): targets are listed as pairs of one subnet with one subnet interface. 
     * More precisely, each subnet interface is paired with its encompassing subnet. Why ? Because 
     * routing/measurement issues can cause, in rare instances, IPs from the same subnet (which 
     * includes the target) to re-appear when doing traceroute probing towards the target 
     * interface. Regardless of what causes this problem (do we have a bad subnet measurement, or 
     * is the routing unusual ?), such hops shouldn't be considered as potential peers, as it can 
     * cause aberrant topology inference or even peer cycling (a big problem for writing sound 
     * graph building algorithms). The same goes for the trail IP (if we have a direct trail) 
     * re-appearing in the traceroute measurement (practical and obvious case of peer cycling).
     *
     * As SubnetInterface objects don't keep a pointer to the "parent" subnet (because it's 
     * useless in all situations but this one), a std::pair is used instead to keep track of the 
     * relationship for this particular context.
     */

};

#endif /* PEERSCANNER_H_ */
