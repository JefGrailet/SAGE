/*
 * TargetScanner.cpp
 *
 *  Created on: Sep 7, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in TargetScanner.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "TargetScanner.h"
#include "../../common/thread/Thread.h"
#include "../aliasresolution/AliasHintsCollector.h" // For flickering IPs alias resolution
#include "../aliasresolution/AliasResolver.h" // Ditto

TargetScanner::TargetScanner(Environment &e): env(e)
{
    targets = env.getIPDictionary()->listTargetEntries();
}

TargetScanner::~TargetScanner()
{
}

unsigned int TargetScanner::countBadEntries()
{
    unsigned int nb = 0;
    for(list<IPTableEntry*>::iterator it = targets.begin(); it != targets.end(); it++)
    {
        IPTableEntry *cur = (*it);
        if(cur->getTTL() == IPTableEntry::NO_KNOWN_TTL)
            continue;
        
        Trail &t = cur->getTrail();
        if(!t.isVoid() && (t.getNbAnomalies() > 0 || t.getLastValidIP() == (InetAddress) (*cur)))
            nb++;
    }
    return nb;
}

unsigned int TargetScanner::countEntriesWithTTL()
{
    unsigned int nb = 0;
    for(list<IPTableEntry*>::iterator it = targets.begin(); it != targets.end(); it++)
    {
        IPTableEntry *cur = (*it);
        if(cur->getTTL() != IPTableEntry::NO_KNOWN_TTL)
            nb++;
    }
    return nb;
}

unsigned int TargetScanner::estimateSplit(list<IPTableEntry*> ls)
{
    unsigned int minTargets = (unsigned int) env.getScanningMinTargetsPerThread();
    if(ls.size() < 2 * minTargets)
        return 0;
    
    unsigned short splitThreshold = env.getScanningListSplitThreshold();
    unsigned int halfSize = ls.size() / 2;
    
    /*
     * First attempt at split just looks at the size of the list. If the size of the list is 
     * greater than twice the splitThreshold value, we just split the list in two.
     */
    
    if(halfSize >= splitThreshold)
        return halfSize;

    /*
     * Second attempt at split bases itself on the "distance" between IPs with respect to the IPv4 
     * scope; i.e., if there's a large gap between two consecutive IPs, it's less likely probing 
     * the interface before each will amount to probing the same interface. To find the most 
     * balanced split, we evaluate possible splits around the middle of the initial list.
     */
    
    list<IPTableEntry*>::iterator it1 = ls.begin();
    for(unsigned int i = 0; i < halfSize - 1; i++) // -1 because it1 is already the first item
        it1++;
    list<IPTableEntry*>::iterator it2 = it1;
    it2++;
    
    unsigned long it1AsInt = (*it1)->getULongAddress();
    unsigned long it2AsInt = (*it2)->getULongAddress();
    if((it2AsInt - it1AsInt) >= splitThreshold)
        return halfSize; // The middle is already a good split (best scenario)
    
    // Finds the first split at left
    IPTableEntry *prev = NULL;
    unsigned int leftSplit = 0;
    while(it1 != ls.begin())
    {
        IPTableEntry *cur = (*it1);
        if(prev != NULL)
        {
            unsigned long prevAsInt = prev->getULongAddress();
            unsigned long curAsInt = cur->getULongAddress();
            if((prevAsInt - curAsInt) >= splitThreshold) // Assumes the IPs are in "ascending" order
                break;
        }
        prev = cur;
        it1--;
        leftSplit++;
    }
    if(it1 == ls.begin())
        leftSplit = 0;
    
    // Finds the first split at right
    prev = NULL;
    unsigned int rightSplit = 0;
    while(it2 != ls.end())
    {
        IPTableEntry *cur = (*it2);
        if(prev != NULL)
        {
            unsigned long prevAsInt = prev->getULongAddress();
            unsigned long curAsInt = cur->getULongAddress();
            if((curAsInt - prevAsInt) >= splitThreshold) // Assumes the IPs are in "ascending" order
                break;
        }
        prev = cur;
        it2++;
        rightSplit++;
    }
    if(it2 == ls.end())
        rightSplit = 0;
    
    if((leftSplit == 0 && rightSplit == 0) || (halfSize + rightSplit) < minTargets || (halfSize - leftSplit) < minTargets)
        return 0;
    else if(leftSplit == 0)
        return halfSize + rightSplit;
    else if(rightSplit == 0)
        return halfSize - leftSplit;
    
    // Returns the most balanced solution
    if(leftSplit > rightSplit)
        return halfSize + rightSplit;
    return halfSize - leftSplit;
}

list<list<IPTableEntry*> > TargetScanner::splitList(list<IPTableEntry*> ls, unsigned int splitNb)
{
    list<list<IPTableEntry*> > res;
    
    list<IPTableEntry*> firstList;
    list<IPTableEntry*>::iterator it = ls.begin();
    for(unsigned int i = 0; i < splitNb; i++)
    {
        firstList.push_back((*it));
        it++;
    }
    res.push_back(firstList);
    
    list<IPTableEntry*> secondList;
    while(it != ls.end())
    {
        secondList.push_back((*it));
        it++;
    }
    res.push_back(secondList);
    
    return res;
}

bool TargetScanner::compareLists(list<IPTableEntry*> &ls1, list<IPTableEntry*> &ls2)
{
    if(ls1.size() < ls2.size())
        return true;
    return false;
}

list<list<IPTableEntry*> > TargetScanner::reschedule()
{
    unsigned short maxThreads = env.getMaxThreads();
    list<list<IPTableEntry*> > res;
    
    // 1) Filters dictionnary to list problematic entries
    list<IPTableEntry*> badTrails;
    for(list<IPTableEntry*>::iterator it = targets.begin(); it != targets.end(); it++)
    {
        IPTableEntry *cur = (*it);
        if(cur->getTTL() == IPTableEntry::NO_KNOWN_TTL) // No TTL means we can't do much here
            continue;
        
        Trail &t = cur->getTrail();
        if(!t.isVoid() && (t.getNbAnomalies() > 0 || t.getLastValidIP() == (InetAddress) (*cur)))
            badTrails.push_back(cur);
    }
    badTrails.sort(IPTableEntry::compareWithTTL);
    
    if(badTrails.size() == 0)
        return res;
    
    // 2) Isolates entries in separate lists depending on their respective TTL.
    list<IPTableEntry*> curList;
    unsigned char prevTTL = 0;
    for(list<IPTableEntry*>::iterator i = badTrails.begin(); i != badTrails.end(); i++)
    {
        IPTableEntry *curItem = (*i);
        if(curItem->getTTL() != prevTTL)
        {
            if(prevTTL != 0)
            {
                res.push_back(curList);
                curList.clear();
            }
            prevTTL = curItem->getTTL();
        }
        curList.push_back(curItem);
    }
    res.push_back(curList);
    res.sort(TargetScanner::compareLists);
    
    // 3.1) Split when there's a gap larger than listSplitThresh until res.size() == maxThreads
    if(res.size() < maxThreads)
    {
        while(res.size() < maxThreads)
        {
            unsigned int largestSplit = 0;
            list<IPTableEntry*> toSplit;
            for(list<list<IPTableEntry*> >::iterator i = res.begin(); i != res.end(); i++)
            {
                list<IPTableEntry*> curList = (*i);
                
                unsigned int curSplit = this->estimateSplit(curList);
                if(curSplit > largestSplit)
                {
                    largestSplit = curSplit;
                    toSplit = curList;
                }
            }
            
            // No possible split
            if(largestSplit == 0)
                break;
            
            for(list<list<IPTableEntry*> >::iterator i = res.begin(); i != res.end(); i++)
            {
                if((*i) == toSplit)
                {
                    res.erase(i);
                    break;
                }
            }
            
            list<list<IPTableEntry*> > newLists = this->splitList(toSplit, largestSplit);
            res.push_back(newLists.front());
            newLists.pop_front();
            res.push_back(newLists.front());
            
            res.sort(TargetScanner::compareLists);
        }
    }
    // 3.2) Merge two smallest lists together, repeat until res.size() == maxThreads
    else if(res.size() > maxThreads)
    {
        while(res.size() > maxThreads)
        {
            res.sort(TargetScanner::compareLists);
            
            list<IPTableEntry*> first, second;
            first = res.front();
            res.pop_front();
            second = res.front();
            res.pop_front();
            
            first.merge(second);
            res.push_front(first);
        }
    }
    
    return res;
}

void TargetScanner::scan()
{
    ostream *out = env.getOutputStream();
    unsigned int minTargets = env.getScanningMinTargetsPerThread();
    (*out) << "Starting the estimation of distance of all target IPs (in TTL)." << endl;
    
    // 1) Probes all target IPs with LocationTask to discover their distances in TTL and trails
    unsigned int nbTargets = targets.size();
    unsigned int maxThreads = (unsigned int) env.getMaxThreads();
    unsigned int targetsPerThread = nbTargets / maxThreads;
    if(targetsPerThread < minTargets)
        targetsPerThread = minTargets;
    unsigned short nbThreads = (unsigned short) (nbTargets / targetsPerThread);
    unsigned int lastTargets = nbTargets % targetsPerThread;
    if(lastTargets > 0)
        nbThreads++;
    
    // Advertise the amount of threads + maximum amount of targets for each thread
    if(nbThreads == 1)
    {
        (*out) << "To do this, a single thread will be scheduled." << endl;
    }
    else
    {
        (*out) << "To do this, " << nbThreads << " threads will be scheduled ";
        (*out) << "(up to " << targetsPerThread << " target IPs per thread)." << endl;
    }
    if(env.getDisplayMode() >= Environment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        (*out) << endl;
    
    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    Thread *th1[nbThreads];
    list<IPTableEntry*>::iterator targetIt = targets.begin();
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        list<IPTableEntry*> targetsSubset;
        unsigned int toRead = targetsPerThread;
        if(lastTargets > 0 && i == (nbThreads - 1))
            toRead = lastTargets;
        
        for(unsigned int j = 0; j < toRead; j++)
        {
            if(targetIt == targets.end())
                break;
            IPTableEntry *next = (*targetIt);
            targetsSubset.push_back(next);
            targetIt++;
        }

        Runnable *task = NULL;
        try
        {
            task = new LocationTask(env, 
                                    targetsSubset, 
                                    DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                    DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                    DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                    DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

            th1[i] = new Thread(task);
        }
        catch(SocketException &se)
        {
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < nbThreads; k++)
                delete th1[k];
            
            throw StopException();
        }
        catch(ThreadException &te)
        {
            (*out) << "Unable to create more threads." << endl;
                
            delete task;
        
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < nbThreads; k++)
                delete th1[k];
            
            throw StopException();
        }
    }

    // Launches thread(s) then waits for completion
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th1[i]->start();
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
    
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th1[i]->join();
        delete th1[i];
    }
    
    // Might happen because of SocketException thrown within a LocationTask thread
    if(env.isStopping())
    {
        throw StopException();
    }
    
    unsigned int nbBadTrails = this->countBadEntries();
    unsigned int withTTL = this->countEntriesWithTTL();
    double ratioReprobe = ((double) nbBadTrails / (double) withTTL) * 100;
    if(nbBadTrails > 0)
    {
        (*out) << nbBadTrails << " out of " << withTTL << " IPs with an estimated TTL (";
        (*out) << ratioReprobe << "%) need to be re-probed to improve trails." << endl;
    }
    
    // 2) Re-probes target IPs with incomplete trails, if any
    list<list<IPTableEntry*> > rescheduled = this->reschedule();
    if(rescheduled.size() > 0)
    {
        (*out) << "Starting re-probing target IPs with incomplete trails." << endl;
        if(env.getDisplayMode() >= Environment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
            (*out) << endl;
    
        unsigned short nbReprobing = env.getScanningNbReprobing();
        for(unsigned short i = 0; i < nbReprobing; i++)
        {
            (*out) << "Starting re-probing n°" << (i + 1) << "." << endl;
            
            nbThreads = (unsigned short) rescheduled.size();
            Thread *th2[nbThreads];
            range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
            
            // Just like for the first step, advertises the amount of threads + targets for each.
            
            if(nbThreads == 1)
            {
                (*out) << "To do this, a single thread will be scheduled." << endl;
            }
            else
            {
                unsigned int minTargets = rescheduled.front().size();
                unsigned int maxTargets = rescheduled.back().size();
                (*out) << "To do this, " << nbThreads << " threads will be scheduled ";
                (*out) << "(from " << minTargets << " to " << maxTargets << " target IPs per thread)." << endl;
            }
            
            for(unsigned short j = 0; j < nbThreads; j++)
            {
                list<IPTableEntry*> targetsSubset = rescheduled.front();
                rescheduled.pop_front();
                
                Runnable *task = NULL;
                try
                {
                    task = new TrailCorrectionTask(env, 
                                                   targetsSubset, 
                                                   DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range), 
                                                   DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (j * range) + range - 1, 
                                                   DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                                   DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

                    th2[j] = new Thread(task);
                }
                catch(SocketException &se)
                {
                    // Cleaning remaining threads (if any is set)
                    for(unsigned short k = 0; k < nbThreads; k++)
                        delete th2[k];
                    
                    throw StopException();
                }
                catch(ThreadException &te)
                {
                    (*out) << "Unable to create more threads." << endl;
                    
                    delete task;
                    
                    // Cleaning remaining threads (if any is set)
                    for(unsigned short k = 0; k < nbThreads; k++)
                        delete th2[k];
                    
                    throw StopException();
                }
            }

            // Launches thread(s) then waits for completion
            for(unsigned int j = 0; j < nbThreads; j++)
            {
                th2[j]->start();
                Thread::invokeSleep(env.getProbingThreadDelay());
            }
            
            for(unsigned int j = 0; j < nbThreads; j++)
            {
                th2[j]->join();
                delete th2[j];
            }
            
            // Prepares list for next round (if necessary)
            if(i < (nbReprobing - 1))
            {
                nbBadTrails = this->countBadEntries();
                if(nbBadTrails == 0)
                {
                    (*out) << "There is no more incomplete trails. Probing for target ";
                    (*out) << "scanning will stop here." << endl;
                    break;
                }
                else
                {
                    ratioReprobe = ((double) nbBadTrails / (double) withTTL) * 100;
                    (*out) << nbBadTrails << " out of " << withTTL << " IPs with an estimated ";
                    (*out) << "TTL (" << ratioReprobe << "%) need to be re-probed to improve ";
                    (*out) << "trails." << endl;
                    
                    rescheduled = this->reschedule();
                }
            }
        }
    }
    
    // 3) We're done with the scanning itself (finalization is done with another method).
}

void TargetScanner::addFlickeringPeers(IPTableEntry *IP, 
                                       list<IPTableEntry*> *alias, 
                                       list<IPTableEntry*> *flickIPs)
{
    list<InetAddress> *peers = IP->getFlickeringPeers();
    for(list<InetAddress>::iterator i = peers->begin(); i != peers->end(); ++i)
    {
        InetAddress curIP = (*i);
        
        // Finds corresponding entry in flickIPs, does a recursive call and erases it from flickIPs
        for(list<IPTableEntry*>::iterator j = flickIPs->begin(); j != flickIPs->end(); ++j)
        {
            IPTableEntry *curEntry = (*j);
            if((InetAddress) (*curEntry) == curIP)
            {
                alias->push_back(curEntry);
                flickIPs->erase(j);
                addFlickeringPeers(curEntry, alias, flickIPs);
                break;
            }
        }
    }
}

void TargetScanner::finalize()
{
    IPLookUpTable *dict = env.getIPDictionary();
    
    // Detects problematic IPs
    dict->reviewScannedIPs();
    dict->reviewSpecialIPs(env.getScanningMaxFlickeringDelta());
    
    /*
     * We now build potential aliases, using a list of flickering IPs. For each item, we will:
     * 1) init a new list curAlias with the IP itself, 
     * 2) then, for each flickering peer:
     *    -add the peer to curAlias, 
     *    -remove it from the flickIPs list, 
     *    -perform the same operation recursively on its own peers;
     * 3) after, the final list is sorted, duplicate IPs are removed and curAlias is appended to 
     *    the list of alleged aliases.
     */
    
    list<list<IPTableEntry*> > aliases; // Only alleged, at this point
    list<IPTableEntry*> flickIPs = dict->listFlickeringIPs();
    while(flickIPs.size() > 0)
    {
        // 1) Inits the alias
        list<IPTableEntry*> curAlias;
        IPTableEntry *first = flickIPs.front();
        curAlias.push_back(first);
        flickIPs.pop_front();
        
        // 2) Adds flickering peers to the alias
        addFlickeringPeers(first, &curAlias, &flickIPs);
        
        // 3) Remove duplicate IPs and append to the alleged aliases
        curAlias.sort(IPTableEntry::compare);
        IPTableEntry *prev = NULL;
        for(list<IPTableEntry*>::iterator i = curAlias.begin(); i != curAlias.end(); ++i)
        {
            if(i == curAlias.begin())
            {
                prev = (*i);
                continue;
            }
            if((*i) == prev)
                curAlias.erase(i--);
            else
                prev = (*i);
        }
        aliases.push_back(curAlias);
    }
    
    if(aliases.size() == 0)
        return;
    
    ostream *out = env.getOutputStream();
    (*out) << "\nThere are flickering IPs that could lead to the discovery of ";
    if(aliases.size() > 1)
        (*out) << aliases.size() << " aliases.\n";
    else
        (*out) << "one alias.\n";
    (*out) << "Now starting alias hints collection...\n";
    
    /*
     * An AliasHintsCollector object is created to collect alias hints on all alleged aliases, 
     * in order to check the odds that flickering IPs indeed belong to a same device (or not).
     */
    
    AliasHintsCollector ahc(env);
    for(list<list<IPTableEntry*> >::iterator i = aliases.begin(); i != aliases.end(); ++i)
    {
        list<IPTableEntry*> hypothesis = (*i);
        (*out) << "Collecting hints for ";
        for(list<IPTableEntry*>::iterator j = hypothesis.begin(); j != hypothesis.end(); ++j)
        {
            if(j != hypothesis.begin())
                (*out) << ", ";
            (*out) << *(*j);
        }
        (*out) << "... " << std::flush;
        ahc.setIPsToProbe(hypothesis);
        ahc.collect();
    }
    
    // Builds the alias set, using the AliasResolver class.
    (*out) << "Resolving... " << std::flush;
    AliasSet *set = new AliasSet(AliasSet::SUBNET_DISCOVERY);
    AliasResolver ar(env);
    for(list<list<IPTableEntry*> >::iterator i = aliases.begin(); i != aliases.end(); ++i)
    {
        list<Alias*> newAliases = ar.resolve((*i), env.usingStrictAliasResolution());
        set->addAliases(newAliases, true); // Only actual aliases (#IPs >= 2) should appear
    }
    env.addAliasSet(set);
    (*out) << "Done." << endl;
}
