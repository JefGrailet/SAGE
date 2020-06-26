/*
 * PeerScanner.cpp
 *
 *  Created on: Aug 19, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in PeerScanner.h (see this file to learn further about the goals 
 * of such a class).
 */

#include <sys/stat.h> // For CHMOD edition

#include "../../common/thread/Thread.h"
#include "PeerScanner.h"
#include "PeerDiscoveryTask.h"

PeerScanner::PeerScanner(Environment &e): env(e)
{
    list<Subnet*> *subnets = env.getSubnets();
    list<list<pair<Subnet*, SubnetInterface*> > > lists; // List of lists of pairs (see below)
    for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); ++i)
    {
        Subnet *cur = (*i);
        
        /*
         * If the smallest TTL seen in this subnet is 1 (likely a contra-pivot), this means the 
         * subnet is next to the vantage point (i.e., this computer), therefore there's no need 
         * to look for peers.
         */
        
        if(cur->getSmallestTTL() <= 1)
            continue;
        
        unsigned short maxPivots = env.getMaxPeerDiscoveryPivots();
        list<SubnetInterface*> pivots = cur->getPeerDiscoveryPivots(maxPivots);
        list<pair<Subnet*, SubnetInterface*> > pairs;
        for(list<SubnetInterface*>::iterator j = pivots.begin(); j != pivots.end(); j++)
            pairs.push_back(std::make_pair(cur, (*j)));
        lists.push_back(pairs);
    }
    
    /*
     * "lists" is now being processed into a single list. The reason why the initial lists aren't 
     * directly merged into a single one is the following: concatenating them means the probing 
     * threads might probe pivot IPs of a same subnet at the same time (because they would be 
     * consecutive in the list of targets), possibly triggering rate-limiting mechanisms in the 
     * target domain. It is arguably better to spread the target IPs such that pivots of a same 
     * subnet aren't consecutive. Therefore, the following code process all lists to remove one 
     * target at once and put it in the final target list, then re-starts with the remaining 
     * non-empty lists until "lists" is exhausted.
     */
    
    while(lists.size() > 0)
    {
        list<pair<Subnet*, SubnetInterface*> > curList = lists.front();
        lists.pop_front();
        
        pair<Subnet*, SubnetInterface*> target = curList.front();
        curList.pop_front();
        if(curList.size() > 0)
            lists.push_back(curList);
        
        targets.push_back(target);
    }
}

PeerScanner::~PeerScanner()
{
}

void PeerScanner::scan()
{
    IPLookUpTable *dictionary = env.getIPDictionary();
    list<Subnet*> *subnets = env.getSubnets();
    
    /*
     * NEIGHBORHOOD DISCOVERY PREPARATION
     *
     * The subnets are processed in order to record in the IP dictionary what are the IPs that 
     * appear in direct trails of subnet pivot IPs. Taking any IP that appears in a trail (even 
     * without anomaly) can be problematic because trails of contra-pivot IPs might not be 
     * associated to any neighborhood. Another potential issue is that the IP of a direct trail of 
     * a contra-pivot can reappear at the end of the (partial) route towards pivot IPs of the same 
     * subnet. It's problematic for the same reason as the previously mentioned problem.
     */
    
    for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); i++)
    {
        Subnet *curSubnet = (*i);
        list<InetAddress> dTrailIPs = curSubnet->getDirectTrailIPs();
        if(dTrailIPs.size() == 0)
            continue;
        
        for(list<InetAddress>::iterator j = dTrailIPs.begin(); j != dTrailIPs.end(); ++j)
        {
            IPTableEntry *trailIPEntry = dictionary->lookUp((*j));
            if(trailIPEntry != NULL)
                trailIPEntry->setAsDenotingNeighborhood();
        }
    }

    ostream *out = env.getOutputStream();
    unsigned short nbThreads = env.getMaxThreads();
    unsigned short displayMode = env.getDisplayMode();

    /*
     * PEER DISCOVERY
     *
     * In order to locate neighborhoods with respect to each other in subsequent steps, one needs 
     * to collect additional traceroute data which will allow to discover "peers" of a given 
     * neighborhood. Given a neighborhood (aggregate of subnets with the same trail or an 
     * equivalent trail), a peer is another neighborhood that is located next to it. To detect it, 
     * one has to look at the interface(s) that appear just before the trail(s) of the aggregated 
     * subnets and see if any correspond to the trail of another neighborhood.
     *
     * As the scanning (i.e., the probing phase occurring after pre-scanning and before subnet 
     * inference) collects just enough data to infer correct trails and the distance in terms of 
     * TTL of a given target IP, one needs to do some additional traceroute probing to get 
     * additional hops that might satisfy the definition of "peer". In an ideal situation, only 
     * one additional probe per target should be enough, but more probes (because of lack of peer, 
     * intermediate hops, cycling/anonymous hops, etc.) might be needed in some situations.
     *
     * The additional probing is done with multiple PeerDiscoveryTask threads (see the header of 
     * the corresponding class for more details) that are scheduled in the same manner as threads 
     * involved in (pre-)scanning.
     */
    
    (*out) << "Collecting additional data to discover and locate neighborhoods.\n" << endl;
    
    // Size of the thread array and amount of targets per thread
    unsigned short sizeThreadArray = 0;
    unsigned long targetsPerThread = 1;
    if((unsigned long) targets.size() > (unsigned long) nbThreads)
    {
        sizeThreadArray = nbThreads;
        while((targetsPerThread * (unsigned long) sizeThreadArray) < targets.size())
            targetsPerThread++;
    }
    else
        sizeThreadArray = (unsigned short) targets.size();
    
    string plural = "";
    if(targets.size() > 1)
        plural = "s";
    (*out) << targets.size() << " IP" << plural << " will be used as target" << plural;
    (*out) << " for traceroute-like probing, using " << sizeThreadArray << " thread";
    if(sizeThreadArray > 1)
        (*out) << "s";
    (*out) << " ";
    if(targetsPerThread == 1)
        (*out) << "(one target per thread)";
    else
        (*out) << "(up to " << targetsPerThread << " targets per thread)";
    (*out) << "." << endl;
    
    // Prepares the thread(s)
    Thread *th[sizeThreadArray];
    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / sizeThreadArray;
    list<pair<Subnet*, SubnetInterface*> > scheduled(targets); // Copy of the targets
    
    (*out) << "Probing the " << targets.size() << " target IP" << plural << "...";
    if(displayMode >= Environment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        (*out) << endl;
    else
        (*out) << " " << std::flush;
    
    // Schedules the threads
    for(unsigned short i = 0; i < sizeThreadArray; i++)
    {
        list<pair<Subnet*, SubnetInterface*> > targetsSubset;
        unsigned long toRead = targetsPerThread;
        while(scheduled.size() > 0 && toRead > 0)
        {
            targetsSubset.push_back(scheduled.front());
            scheduled.pop_front();
            toRead--;
        }
        
        Runnable *task = NULL;
        try
        {
            task = new PeerDiscoveryTask(env, 
                                         targetsSubset, // By design, always at least one target
                                         DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                         DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                         DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                         DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

            th[i] = new Thread(task);
        }
        catch(const SocketException &se)
        {
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < sizeThreadArray; k++)
                delete th[k];
            
            throw StopException();
        }
        catch(const ThreadException &te)
        {
            ostream *out = env.getOutputStream();
            (*out) << "Unable to create more threads." << endl;
                
            delete task;
        
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < sizeThreadArray; k++)
                delete th[k];
            
            throw StopException();
        }
    }

    // Starts thread(s) then waits for completion
    for(unsigned int i = 0; i < sizeThreadArray; i++)
    {
        th[i]->start();
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
    
    for(unsigned int i = 0; i < sizeThreadArray; i++)
    {
        th[i]->join();
        delete th[i];
    }
    
    if(env.isStopping())
        throw StopException();
    
    (*out) << "Done." << endl;
}

void PeerScanner::output(string filename)
{
    list<pair<Subnet*, SubnetInterface*> > copy(targets); // Copy is made, in case of re-probing scenario
    list<SubnetInterface> interfaces; // Sorted list of SubnetInterface objects
    for(list<pair<Subnet*, SubnetInterface*> >::iterator i = copy.begin(); i != copy.end(); i++)
        interfaces.push_back(*(*i).second);
    interfaces.sort(SubnetInterface::smaller);
    
    string output = "";
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
    {
        // Can happen, e.g. if the target is close to the vantage point
        if(i->partialRoute.size() == 0)
            continue;
        output += i->routeToString() + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
