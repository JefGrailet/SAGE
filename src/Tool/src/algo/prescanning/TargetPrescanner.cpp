/*
 * TargetPrescanner.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in TargetPrescanner.h (see this file to learn further about the 
 * goals of such a class).
 */

#include <ostream>
using std::ostream;

#include <unistd.h> // For usleep() function

#include "TargetPrescanner.h"
#include "TargetPrescanningUnit.h"
#include "../../common/thread/Thread.h"

TargetPrescanner::TargetPrescanner(Environment &e): env(e)
{
    timeout = env.getTimeoutPeriod(); // Timeout delay of the first scan
}

TargetPrescanner::~TargetPrescanner()
{
}

bool TargetPrescanner::hasUnresponsiveTargets()
{
    if(unresponsiveTargets.size() > 0)
        return true;
    return false;
}

void TargetPrescanner::reloadUnresponsiveTargets()
{
    targets.clear();
    while(unresponsiveTargets.size() > 0)
    {
        InetAddress cur = unresponsiveTargets.front();
        unresponsiveTargets.pop_front();
        targets.push_back(cur);
    }
}

void TargetPrescanner::callback(list<InetAddress> responsive, list<InetAddress> unresponsive)
{
    IPLookUpTable *dict = env.getIPDictionary();
    for(list<InetAddress>::iterator i = responsive.begin(); i != responsive.end(); ++i)
    {
        InetAddress target = (*i);
        IPTableEntry *newEntry = dict->create(target);
        if(newEntry != NULL)
            newEntry->setPreferredTimeout(timeout);
    }
    unresponsiveTargets.splice(unresponsiveTargets.end(), unresponsive);
}

void TargetPrescanner::probe()
{
    unsigned short maxThreads = env.getMaxThreads();
    unsigned long nbTargets = (unsigned long) targets.size();
    if(nbTargets == 0)
        return;
    unsigned short nbThreads = 1;
    unsigned long targetsPerThread = (unsigned long) TargetPrescanner::MINIMUM_TARGETS_PER_THREAD;
    unsigned long lastTargets = 0;
    
    // Computes amount of threads and amount of targets being probed per thread
    if(nbTargets > TargetPrescanner::MINIMUM_TARGETS_PER_THREAD)
    {
        if((nbTargets / targetsPerThread) > (unsigned long) maxThreads)
        {
            unsigned long factor = 2;
            while((nbTargets / (targetsPerThread * factor)) > (unsigned long) maxThreads)
                factor++;
            
            targetsPerThread *= factor;
        }
        
        nbThreads = (unsigned short) (nbTargets / targetsPerThread);
        lastTargets = nbTargets % targetsPerThread;
        if(lastTargets > 0)
            nbThreads++;
    }
    else
        lastTargets = nbTargets;

    // Prepares and launches threads
    unsigned short range = (DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID - DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID) / nbThreads;
    Thread *th[nbThreads];
    for(unsigned short i = 0; i < nbThreads; i++)
    {
        list<InetAddress> targetsSubset;
        unsigned long toRead = targetsPerThread;
        if(lastTargets > 0 && i == (nbThreads - 1))
            toRead = lastTargets;
        
        for(unsigned long j = 0; j < toRead; j++)
        {
            InetAddress addr(targets.front());
            targetsSubset.push_back(addr);
            targets.pop_front();
        }

        Runnable *task = NULL;
        try
        {
            task = new TargetPrescanningUnit(env, 
                                             (*this), 
                                             targetsSubset, // By design, always at least one target
                                             DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range), 
                                             DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + (i * range) + range - 1, 
                                             DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                             DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);

            th[i] = new Thread(task);
        }
        catch(SocketException &se)
        {
            // Cleaning remaining threads (if any is set)
            for(unsigned short k = 0; k < nbThreads; k++)
                delete th[k];
            
            throw StopException();
        }
        catch(ThreadException &te)
        {
            ostream *out = env.getOutputStream();
            (*out) << "Unable to create more threads." << endl;
                
            delete task;
        
            // Same as above
            for(unsigned short k = 0; k < nbThreads; k++)
                delete th[k];
            
            throw StopException();
        }
    }

    // Launches thread(s) then waits for completion
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th[i]->start();
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
    
    for(unsigned int i = 0; i < nbThreads; i++)
    {
        th[i]->join();
        delete th[i];
    }
    
    // Might happen because of SocketSendException thrown within a unit
    if(env.isStopping())
    {
        throw StopException();
    }
}

void TargetPrescanner::run(list<InetAddress> targets)
{
    ostream *out = env.getOutputStream();
    IPLookUpTable *dict = env.getIPDictionary();
    bool prescanThirdOpinion = env.usingPrescanningThirdOpinion();
        
    (*out) << "Pre-scanning with initial timeout... " << std::flush;
    this->targets = targets;
    this->probe();
    (*out) << "done." << endl;
    
    unsigned int nbIPs = dict->getTotalIPs();
    if(nbIPs > 0)
    {
        (*out) << "Found a total of ";
        if(nbIPs > 1)
            (*out) << nbIPs << " responsive IPs." << endl;
        else
            (*out) << "one reponsive IP." << endl;
    }
    else
        (*out) << "Didn't find any responsive IP." << endl;
    
    if(this->hasUnresponsiveTargets())
    {
        TimeVal timeout2 = env.getTimeoutPeriod() * 2;
        (*out) << "Second opinion with twice the timeout (" << timeout2 << ")... " << std::flush;
        timeout = timeout2;
        this->reloadUnresponsiveTargets();
        this->probe();
        (*out) << "done." << endl;
        
        unsigned int newTotal = dict->getTotalIPs();
        if(newTotal > nbIPs)
        {
            unsigned int diff = newTotal - nbIPs;
            (*out) << "Found a total of ";
            if(diff > 1)
                (*out) << diff << " additional responsive IPs." << endl;
            else
                (*out) << "one additional reponsive IP." << endl;
        }
        else
            (*out) << "Didn't find any additional responsive IP." << endl;
        
        if(this->hasUnresponsiveTargets() && prescanThirdOpinion)
        {
            TimeVal timeout3 = env.getTimeoutPeriod() * 4;
            (*out) << "Third opinion with 4 times the timeout (" << timeout3 << ")... " << std::flush;
            timeout = timeout3;
            this->reloadUnresponsiveTargets();
            this->probe();
            (*out) << "done." << endl;
            
            nbIPs = newTotal; // Total of IPs at the end of second opinion
            newTotal = dict->getTotalIPs();
            if(newTotal > nbIPs)
            {
                unsigned int diff = newTotal - nbIPs;
                (*out) << "Found a total of ";
                if(diff > 1)
                    (*out) << diff << " additional responsive IPs." << endl;
                else
                    (*out) << "one additional reponsive IP." << endl;
            }
            else
                (*out) << "Didn't find any additional responsive IP." << endl;
        }
    }
    else
    {
        (*out) << "All probed IPs were responsive.\n" << endl;
    }
}
