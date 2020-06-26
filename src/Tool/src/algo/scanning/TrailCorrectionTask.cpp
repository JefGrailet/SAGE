/*
 * TrailCorrectionTask.cpp
 *
 *  Created on: Sep 19, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in TrailCorrectionTask.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "../../common/thread/Thread.h"
#include "TrailCorrectionTask.h"

TrailCorrectionTask::TrailCorrectionTask(Environment &e, 
                                         list<IPTableEntry*> tl, 
                                         unsigned short lbii, 
                                         unsigned short ubii, 
                                         unsigned short lbis, 
                                         unsigned short ubis):
env(e), 
targets(tl)
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
    catch(const SocketException &se)
    {
        ostream *out = env.getOutputStream();
        Environment::consoleMessagesMutex.lock();
        (*out) << "Caught an exception because no new socket could be opened." << endl;
        Environment::consoleMessagesMutex.unlock();
        this->stop();
        throw;
    }
    
    // Verbosity/debug stuff
    showProbingDetails = false; // Default
    debugMode = false; // Default
    unsigned short displayMode = env.getDisplayMode();
    if(displayMode >= Environment::DISPLAY_MODE_SLIGHTLY_VERBOSE)
        showProbingDetails = true;
    if(displayMode >= Environment::DISPLAY_MODE_DEBUG)
        debugMode = true;
}

TrailCorrectionTask::~TrailCorrectionTask()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

ProbeRecord TrailCorrectionTask::probe(IPTableEntry *target, unsigned char TTL)
{
    InetAddress localIP = env.getLocalIPAddress();
    unsigned short maxRetries = env.getMaxRetries();
    InetAddress probeDst = (InetAddress) (*target);
    ProbeRecord record = DirectProber::VOID_RECORD;
    unsigned short retries = 0;
    
    while(retries < maxRetries)
    {
        if(!record.isVoidRecord())
            record = DirectProber::VOID_RECORD;
    
        try
        {
            record = prober->singleProbe(localIP, probeDst, TTL, true);
        }
        catch(const SocketException &se)
        {
            throw;
        }
        
        if(record.getRplyICMPtype() != 255)
            break;
        
        retries++;
        Thread::invokeSleep(env.getRetryDelay());
    }
    
    if(debugMode)
        log += prober->getAndClearLog();
    
    // N.B.: fixed flow is ALWAYS used (i.e. "Paris" traceroute) to mitigate fake paths in routes.
    return record;
}

void TrailCorrectionTask::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

string TrailCorrectionTask::TTLToString(unsigned char TTL)
{
    stringstream ss;
    ss << (unsigned short) TTL;
    return ss.str();
}

void TrailCorrectionTask::run()
{
    for(list<IPTableEntry*>::iterator it = targets.begin(); it != targets.end(); it++)
    {
        IPTableEntry *curTarget = (*it);
        vector<RouteHop> &route = curTarget->getRoute();
        Trail &curTrail = curTarget->getTrail();
        
        // Takes account of preferred timeout for probing this IP (decided at pre-scanning)
        TimeVal initialTimeout = prober->getTimeout();
        TimeVal preferredTimeout = curTarget->getPreferredTimeout();
        bool timeoutChanged = false;
        if(preferredTimeout > initialTimeout)
        {
            timeoutChanged = true;
            prober->setTimeout(preferredTimeout);
        }
        
        if(showProbingDetails)
            log += "Trying to fix trail for " + curTarget->toStringSimple() + "...\n";
        
        // probeTTL starts at the TTL at which the IP was first located minus 1
        unsigned char initTTL = curTarget->getTTL();
        unsigned char probeTTL = initTTL - 1;
        
        unsigned short curNbAnomalies = curTrail.getNbAnomalies();
        bool changedTTL = false;
        while(curNbAnomalies >= (unsigned short) (initTTL - probeTTL))
        {
            ProbeRecord rec = DirectProber::VOID_RECORD;
            try
            {
                rec = this->probe(curTarget, probeTTL);
            }
            catch(const SocketException &se)
            {
                this->stop();
                return;
            }
            
            // Echo reply: TTL was overestimated. TTL of target needs to be fixed.
            if(rec.getRplyICMPtype() == DirectProber::ICMP_TYPE_ECHO_REPLY)
            {
                curTarget->setTTL(probeTTL);
                changedTTL = true;
                if(showProbingDetails)
                {
                    log += "Got an echo reply at TTL = " + TTLToString(probeTTL);
                    log += ", meaning previous TTL distance was overestimated.\n";
                }
            }
            // Time exceeded reply: contains the IP we are looking for
            else if(rec.getRplyICMPtype() == DirectProber::ICMP_TYPE_TIME_EXCEEDED)
            {
                InetAddress replyingIP = rec.getRplyAddress();
                if(replyingIP != InetAddress(0))
                {
                    route[((unsigned short) probeTTL) - 1] = RouteHop(rec);
                    if(showProbingDetails)
                    {
                        log += "Found a non-anonymous interface at TTL = " + TTLToString(probeTTL);
                        log += ": " + replyingIP.getHumanReadableRepresentation() + ".\n";
                    }
                }
            }
            
            probeTTL--;
        }
        
        // Computes and print the fixed trail
        curTarget->setTrail(); // Should always return true, by design
        if(showProbingDetails)
        {
            stringstream ss;
            if(changedTTL)
            {
                ss << "New TTL for " << (InetAddress) (*curTarget) << " is ";
                ss << TTLToString(curTarget->getTTL()) << ".\n";
            }
            ss << "New trail for " << (InetAddress) (*curTarget) << " is ";
            ss << curTarget->getTrail() << ".\n";
            log += ss.str();
        }
        
        // Puts back the initial timeout (for next target IP)
        if(timeoutChanged)
            prober->setTimeout(initialTimeout);
        
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
    
    // Displays the log if one was requested
    if(showProbingDetails)
    {
        Environment::consoleMessagesMutex.lock();
        ostream *out = env.getOutputStream();
        (*out) << this->log << std::flush;
        Environment::consoleMessagesMutex.unlock();
    }
}
