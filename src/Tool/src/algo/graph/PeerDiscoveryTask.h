/*
 * PeerDiscoveryTask.h
 *
 *  Created on: Aug 19, 2019
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes a list of given target IPs, provided as SubnetInterface 
 * objects. PeerDiscoveryTask probes each target IP backwards, in a traceroute-like manner (but 
 * starting with an offset to avoid reprobing the part of the route that should correspond to the 
 * trail of the target), and stops when it discovers an IP that appears in the IP dictionary as 
 * identifying a neighborhood, as this means we discovered a peer (see PeerScanner class). It then 
 * updates the object associated to the target to keep track of the discovered peer (and route). 
 * In an ideal situation (probing of a large network where all neighborhoods can be seen), there 
 * should be no intermediate hop between the trail and a peer IP, meaning only one additional 
 * probe per target is required.
 * 
 * Only one target is probed at once by a same PeerDiscoveryTask thread, and a small delay is 
 * added between the probing of each listed target to avoid being too aggressive while probing the 
 * target domain (just like in the LocationTask class of the scanning/ module).
 */

#ifndef PEERDISCOVERYTASK_H_
#define PEERDISCOVERYTASK_H_

#include <utility>
using std::pair;

#include "../Environment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"

class PeerDiscoveryTask : public Runnable
{
public:

    // Constructor, destructor
    PeerDiscoveryTask(Environment &env, 
                      list<pair<Subnet*, SubnetInterface*> > targets, 
                      unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                      unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                      unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                      unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE);
    ~PeerDiscoveryTask();
    
    void run();
    
private:

    // Pointer to the environment object
    Environment &env;
    
    // List of subnet interfaces to probe
    list<pair<Subnet*, SubnetInterface*> > targets;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord probe(const InetAddress &dst, unsigned char TTL);
    
    // "Stop" method (when resources are lacking)
    void stop();
    
    // Verbosity/debug stuff
    bool advertiseRoute, displayFinalRoute, debugMode;
    string log;
};

#endif /* PEERDISCOVERYTASK_H_ */
