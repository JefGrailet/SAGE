/*
 * LocationTask.h
 *
 *  Created on: Sep 7, 2018
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes a list of target IPs to discover their minimum distance 
 * TTL-wise (i.e., what's the minimum TTL value to get an echo reply) as well as the IPs that 
 * appear in the "time exceeded" replies right before the minimum TTL, if not the complete route.
 *
 * In order to speed up a single task, the class relies on the fact that a TargetList consists of 
 * an ordered list of targets which are consecutive with respect to their liveness and the IPv4 
 * scope. It will perform a traceroute-like probing with the first IP, but it will probe the next 
 * IP at the TTL discovered for the first and make adjustements w.r.t. this TTL. The intuition is 
 * that close IPs are likelier to be part of a same subnet, and therefore, located at roughly the 
 * same distance. As a consequence, one can avoid completely evaluating the distance (i.e., with 
 * the traceroute-like method) for all target IPs and just perform it for the first target.
 */

#ifndef LOCATIONTASK_H_
#define LOCATIONTASK_H_

#include "../Environment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"

class LocationTask : public Runnable
{
public:
    
    const static unsigned char MAX_CONSECUTIVE_ANONYMOUS_HOPS = 4;
    const static unsigned char MAX_TTL_ALLOWED = 48;

    // Constructor
    LocationTask(Environment &env, 
                 list<IPTableEntry*> targets, 
                 unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                 unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                 unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                 unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE);
    
    // Destructor, run method and print out method
    ~LocationTask();
    void run();
    
private:

    // Pointer to the environment singleton
    Environment &env;
    
    // Pointer to the target list
    list<IPTableEntry*> targets;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord probe(IPTableEntry *target, unsigned char TTL);
    
    // "Stop" method (when resources are lacking)
    void stop();
    
    // Verbosity/debug stuff
    bool showProbingDetails, debugMode;
    string log;
    
    /*** Probing steps ***/
    
    /*
     * Probes "forward" a target, i.e., it first probes it with a given TTL value (initTTL), then 
     * increments it if this first probe led to an ICMP "time exceeded" reply. It stops as soon as 
     * it gets an "echo" reply, and initializes the route (with discovered interfaces) in the 
     * IPTableEntry object.
     *
     * @param IPTableEntry* target   The target IP, as an IPTableEntry object
     * @param unsigned char initTTL  TTL value used in the first probe
     * @return bool                  True if an "echo" reply was obtained in the end
     * @throws SocketException       Re-thrown from probe() in case of connectivity issues
     */
    
    bool forwardProbing(IPTableEntry *target, unsigned char initTTL);
    
    /*
     * Probes "backward" a target, i.e., it first probes it with the registered minimum TTL minus 
     * one, then decrements it if the first probe led to an ICMP "echo" reply. It stops as soon as 
     * it gets a "time exceeded" reply, and updates the IPTableEntry object.
     *
     * @param IPTableEntry* target   The target IP, as an IPTableEntry object
     * @return bool                  True if backward probing was successful (false for timeouts)
     * @throws SocketException       Re-thrown from probe() in case of connectivity issues
     */
    
    bool backwardProbing(IPTableEntry *target);
    
    /*
     * Checks the route (if any) towards a target IP to see if a trail can be computed from it. If 
     * such trail cannot be obtained as soon as this method is called, it will do a variant of 
     * backward probing where all replies should be "time exceeded" and will stop as soon as it 
     * gets all the interfaces it needs to compute the trail, i.e., 
     * -at least one non-anonymous hop shortly before the target (ideally at min. TTL - 1), 
     * -at least one hop before that non-anonymous hop that differs from it, as a way to ensure 
     *  the last non-anonymous hop isn't a cycle.
     * It is worth noting that this method should only be called after a route has been set 
     * through previous forward and/or backward probing, otherwise nothing happens.
     *
     * @param IPTableEntry* target   The target IP, as an IPTableEntry object
     * @throws SocketException       Re-thrown from probe() in case of connectivity issues
     */
    
    void setTrail(IPTableEntry *target);
    
    // Utility method to convert a TTL (unsigned char) into string format.
    static string TTLToString(unsigned char TTL);
    
};

#endif /* LOCATIONTASK_H_ */
