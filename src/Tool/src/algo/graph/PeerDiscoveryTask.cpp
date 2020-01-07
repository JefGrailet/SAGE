/*
 * PeerDiscoveryTask.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in PeerDiscoveryTask.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "PeerDiscoveryTask.h"
#include "../../common/thread/Thread.h" // invokeSleep()

PeerDiscoveryTask::PeerDiscoveryTask(Environment &e, 
                                     list<pair<Subnet*, SubnetInterface*> > sis, 
                                     unsigned short lbii, 
                                     unsigned short ubii, 
                                     unsigned short lbis, 
                                     unsigned short ubis):
env(e), 
targets(sis)
{
    try
    {
        unsigned short protocol = env.getProbingProtocol();
    
        if(protocol == Environment::PROBING_PROTOCOL_UDP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env.usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectUDPWrappedICMPProber(env.getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    env.getTimeoutPeriod(), 
                                                    env.getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env.debugMode());
        }
        else if(protocol == Environment::PROBING_PROTOCOL_TCP)
        {
            int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
            if(env.usingFixedFlowID())
                roundRobinSocketCount = 1;
            
            prober = new DirectTCPWrappedICMPProber(env.getAttentionMessage(), 
                                                    roundRobinSocketCount, 
                                                    env.getTimeoutPeriod(), 
                                                    env.getProbeRegulatingPeriod(), 
                                                    lbii, 
                                                    ubii, 
                                                    lbis, 
                                                    ubis, 
                                                    env.debugMode());
        }
        else
        {
            prober = new DirectICMPProber(env.getAttentionMessage(), 
                                          env.getTimeoutPeriod(), 
                                          env.getProbeRegulatingPeriod(), 
                                          lbii, 
                                          ubii, 
                                          lbis, 
                                          ubis, 
                                          env.debugMode());
        }
    }
    catch(SocketException &se)
    {
        ostream *out = env.getOutputStream();
        Environment::consoleMessagesMutex.lock();
        (*out) << "Caught an exception because no new socket could be opened." << endl;
        Environment::consoleMessagesMutex.unlock();
        this->stop();
        throw;
    }
    
    // Verbosity/debug stuff
    advertiseRoute = false;
    displayFinalRoute = false; // Default
    debugMode = false; // Default
    unsigned short displayMode = env.getDisplayMode();
    if(displayMode >= Environment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        advertiseRoute = true;
    if(displayMode >= Environment::DISPLAY_MODE_VERBOSE)
        displayFinalRoute = true;
    if(displayMode >= Environment::DISPLAY_MODE_DEBUG)
        debugMode = true;
    
    // Prints first probing details (debug mode only)
    if(debugMode)
    {
        Environment::consoleMessagesMutex.lock();
        ostream *out = env.getOutputStream();
        (*out) << "Initiating partial traceroute towards " << targets.size() << " IP";
        if(targets.size() > 1)
            (*out) << "s";
        (*out) << "...\n" << prober->getAndClearLog();
        Environment::consoleMessagesMutex.unlock();
    }
}

PeerDiscoveryTask::~PeerDiscoveryTask()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

ProbeRecord PeerDiscoveryTask::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env.getLocalIPAddress();
    ProbeRecord record = DirectProber::VOID_RECORD;
    
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, true);
    }
    catch(SocketException &se)
    {
        throw;
    }
    
    if(debugMode) // Debug log
        this->log += prober->getAndClearLog();
    
    /*
     * N.B.: Fixed flow is ALWAYS used in this case because we want to use the Paris variant of 
     * traceroute to mitigate problems encountered with some types of load-balancers.
     */

    return record;
}

void PeerDiscoveryTask::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

void PeerDiscoveryTask::run()
{
    IPLookUpTable *dictionary = env.getIPDictionary();
    AliasSet *aliases = env.getLattestAliases(); // Aliases from subnet inference
    
    list<pair<Subnet*, SubnetInterface*> >::iterator it;
    for(it = targets.begin(); it != targets.end(); ++it)
    {
        Subnet *encompassingSubnet = (*it).first;
        SubnetInterface *target = (*it).second;
        IPTableEntry *targetIP = target->ip;
        // No proper IP dictionary entry or trail: moves to next iteration (shouldn't occur)
        if(targetIP == NULL)
            continue;
        Trail &targetTrail = targetIP->getTrail();
        if(targetTrail.isVoid())
            continue;
        InetAddress targetTrailIP(0);
        if(targetTrail.getNbAnomalies() == 0)
            targetTrailIP = targetTrail.getLastValidIP();
        // Very rare, but can occur with close targets + timeouts
        if(targetTrailIP == InetAddress(0))
            continue;
        
        /*
         * (November 2019) To prevent potential "self-peering", the code quickly checks if the 
         * trail of the target corresponds to a flickering IP, and if yes, gets the corresponding 
         * alias (if any) to ensure we don't select an alias of said trail as a peer.
         */
        
        Alias *assocAlias = NULL;
        IPTableEntry *trailEntry = dictionary->lookUp(targetTrailIP);
        if(trailEntry != NULL && trailEntry->isFlickering())
            assocAlias = aliases->findAlias(targetTrailIP);
        
        InetAddress probeDst = (InetAddress) (*targetIP);
        unsigned char probeTTL = targetIP->getTTL() - 1 - targetTrail.getLengthInTTL();
        unsigned short initialTTL = (unsigned short) probeTTL; // For display only
        if(probeTTL == 0 || probeDst == InetAddress(0)) // Bad probing parameters: quits
            continue;
        
        // Changes timeout if necessary for this IP
        TimeVal initialTimeout, preferredTimeout, usedTimeout;
        initialTimeout = prober->getTimeout();
        preferredTimeout = targetIP->getPreferredTimeout();
        if(preferredTimeout > initialTimeout)
        {
            prober->setTimeout(preferredTimeout);
            usedTimeout = preferredTimeout;
        }
        else
            usedTimeout = initialTimeout;
        
        // Probes the target IP with decreasing TTL until it reachs 0 or until it finds a peer IP
        list<RouteHop> routeHops;
        bool foundPeer = false;
        while(probeTTL > 0)
        {
            ProbeRecord record = DirectProber::VOID_RECORD;
            try
            {
                record = this->probe(probeDst, probeTTL);
            }
            catch(SocketException &se)
            {
                this->stop();
                return;
            }
            
            InetAddress rplyAddress = record.getRplyAddress();
            if(rplyAddress == InetAddress(0))
            {
                // Debug message (N.B.: probe() also appends the debug log)
                if(debugMode)
                {
                    this->log += "Retrying at this TTL with twice the initial timeout...\n";
                }
                
                // New probe with twice the timeout period
                prober->setTimeout(usedTimeout * 2);
                
                try
                {
                    record = this->probe(probeDst, probeTTL);
                }
                catch(SocketException &se)
                {
                    this->stop();
                    return;
                }
                
                rplyAddress = record.getRplyAddress();
                
                // Restores default timeout
                prober->setTimeout(usedTimeout);
            }
            
            // Updates the IP dictionary if not anonymous (needed for next steps, like alias res.)
            if(rplyAddress != InetAddress(0))
            {
                IPTableEntry *relEntry = dictionary->lookUp(rplyAddress);
                if(relEntry == NULL)
                {
                    relEntry = dictionary->create(rplyAddress);
                    relEntry->setType(IPTableEntry::SEEN_WITH_TRACEROUTE);
                    relEntry->setTTL(probeTTL);
                }
                else if(!relEntry->hasTTL(probeTTL))
                {
                    relEntry->recordTTL(probeTTL);
                    relEntry->pickMinimumTTL();
                }
            }
                        
            // Did we discover a valid peer ? (+ checks to avoid peer cycling, see PeerScanner.h)
            if(rplyAddress != InetAddress(0) && !encompassingSubnet->contains(rplyAddress)) // Replying IP not from the subnet
                if(targetTrailIP != InetAddress(0) && rplyAddress != targetTrailIP) // Replying IP isn't another echo
                    if(assocAlias == NULL || !assocAlias->hasInterface(rplyAddress)) // Replying IP isn't an alias of the trail
                        if(dictionary->isPotentialPeer(rplyAddress)) // Replying IP is indeed a potential peer
                            foundPeer = true;
            routeHops.push_front(RouteHop(record, foundPeer)); // Also correctly labels anonymous hops
            
            // Stops probing here
            if(foundPeer)
                break;
            
            probeTTL--;
        }
        
        vector<RouteHop> route;
        while(routeHops.size() > 0)
        {
            route.push_back(routeHops.front());
            routeHops.pop_front();
        }
        target->partialRoute = route;
        
        /*
         * Display policy:
         * -laconic mode (default): nothing is printed.
         * -slightly verbose mode: displays a single line to advertise the discovery of a new 
         *  route for a given target IP (pivot of a subnet).
         * -verbose mode: displays the route, preceeded by the target that was used to obtain it.
         * -debug: same as verbose with a bit more spacing.
         */
        
        if(advertiseRoute)
        {
            stringstream routeLog;
            routeLog << "Got a ";
            if(foundPeer)
                routeLog << "partial ";
            else
                routeLog << "full ";
            routeLog << "route to " << targetIP->toStringSimple();
            if(displayFinalRoute)
            {
                routeLog << ":\n";
                for(unsigned short i = 0; i < route.size(); i++)
                    routeLog << (initialTTL - route.size() + 1 + i) << " - " << route[i] << "\n";
                
                // Adding a line break after probe logs to separate from the complete route
                if(debugMode)
                    this->log += "\n";
            }
            else
                routeLog << ".";
            this->log += routeLog.str();
            
            // Displays the log, which can be a complete sequence of probe as well as a single line
            Environment::consoleMessagesMutex.lock();
            ostream *out = env.getOutputStream();
            (*out) << this->log << endl;
            Environment::consoleMessagesMutex.unlock();
            
            // Clears the log for next target
            this->log = "";
        }
        
        // Some delay before handling next target (same idea as in scanning/LocationTask.cpp)
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
}
