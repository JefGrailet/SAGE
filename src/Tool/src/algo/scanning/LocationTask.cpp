/*
 * LocationTask.cpp
 *
 *  Created on: Sep 7, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in LocationTask.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "../../common/thread/Thread.h"
#include "LocationTask.h"

LocationTask::LocationTask(Environment &e, 
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

LocationTask::~LocationTask()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

ProbeRecord LocationTask::probe(IPTableEntry *target, unsigned char TTL)
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
    
        // N.B.: fixed flow is ALWAYS used (i.e. "Paris" traceroute) to mitigate fake paths.
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
    
    return record;
}

void LocationTask::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

bool LocationTask::forwardProbing(IPTableEntry *target, unsigned char initTTL)
{
    unsigned char probeTTL = initTTL;
    unsigned char consecutiveAnonymous = 0;
    list<RouteHop> timeExceededReplies;
    ProbeRecord rec = DirectProber::VOID_RECORD;
    while(rec.isVoidRecord() && probeTTL <= MAX_TTL_ALLOWED)
    {
        try
        {
            rec = this->probe(target, probeTTL);
        }
        catch(const SocketException &se)
        {
            throw;
        }
        
        // Checks consecutive amount of anonymous replies (timeouts)
        if(rec.getRplyAddress() == InetAddress(0))
            consecutiveAnonymous++;
        else
            consecutiveAnonymous = 0;
        
        if(consecutiveAnonymous >= MAX_CONSECUTIVE_ANONYMOUS_HOPS)
        {
            rec = DirectProber::VOID_RECORD;
            return false;
        }
        
        // Checks the type of the reply; keeps the record if it's an "echo" reply
        if(rec.getRplyICMPtype() != DirectProber::ICMP_TYPE_ECHO_REPLY)
        {
            timeExceededReplies.push_back(RouteHop(rec));
            rec = DirectProber::VOID_RECORD;
            probeTTL++;
        }
    }
    
    if(probeTTL >= MAX_TTL_ALLOWED)
        return false;
    
    target->setTTL(probeTTL);
    if(probeTTL == 1)
        return true;
    
    // Initializes and fills the route with the discovered interfaces
    target->initRoute();
    vector<RouteHop> &newRoute = target->getRoute();
    if(newRoute.size() > 0)
    {
        for(unsigned short i = (unsigned short) initTTL - 1; i < newRoute.size(); i++)
        {
            newRoute[i] = timeExceededReplies.front();
            timeExceededReplies.pop_front();
        }
    }
    
    return true;
}

bool LocationTask::backwardProbing(IPTableEntry *target)
{
    unsigned char initTTL = target->getTTL();
    char probeTTL = (char) initTTL;
    ProbeRecord rec = DirectProber::VOID_RECORD;
    probeTTL--;
    while(rec.isVoidRecord() && probeTTL > 0)
    {
        try
        {
            rec = this->probe(target, (unsigned char) probeTTL);
        }
        catch(const SocketException &se)
        {
            throw;
        }
        
        if(rec.getRplyICMPtype() == 255) // Timeout
            return false;
        
        if(rec.getRplyICMPtype() != DirectProber::ICMP_TYPE_TIME_EXCEEDED)
        {
            rec = DirectProber::VOID_RECORD;
            probeTTL--;
        }
    }
    probeTTL++; // To correct minimum TTL to reach target
    
    /*
     * As the program is designed to never probe IPs that are on the LAN of the vantage point 
     * computer, reaching probeTTL = 1 is not an ill scenario but just means the target IP is 
     * simply one hop away.
     */
    
    // Means there's no need to adjust the route length
    if((unsigned char) probeTTL == initTTL)
        return true;
    
    // Resets the route
    target->setTTL(probeTTL);
    target->initRoute(); // Clears existing route if any
    return true;
}
    
void LocationTask::setTrail(IPTableEntry *target)
{
    vector<RouteHop> &route = target->getRoute();
    if(route.size() == 0) // Occurs with interfaces that are one hop away from vantage point
        return;
    
    // Finds the index of last measured interface
    unsigned short lastMeasured = 0;
    while(lastMeasured < route.size() && route[lastMeasured].isUnset())
        lastMeasured++;
    if(lastMeasured == 0)
    {
        target->setTrail();
        return;
    }
    
    // If trail can't be set, will probe one hop at a time, backwards, to get the required data
    short index = ((short) lastMeasured) - 1;
    while(!(target->setTrail()) && index >= 0)
    {
        unsigned char probeTTL = ((unsigned char) index) + 1;
        ProbeRecord rec = DirectProber::VOID_RECORD;
        try
        {
            rec = this->probe(target, probeTTL);
        }
        catch(const SocketException &se)
        {
            throw;
        }
        
        route[index] = RouteHop(rec);
        index--;
    }
}

string LocationTask::TTLToString(unsigned char TTL)
{
    stringstream ss;
    ss << (unsigned short) TTL;
    return ss.str();
}

void LocationTask::run()
{
    unsigned char prevTTL = 0;
    for(list<IPTableEntry*>::iterator it = targets.begin(); it != targets.end(); it++)
    {
        IPTableEntry *curTarget = (*it);
        
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
            log += "Locating " + curTarget->toStringSimple() + "...\n";
        
        // startTTL depends of whether "prevTTL" is set or not
        unsigned char startTTL = env.getStartTTL();
        if(prevTTL != 0)
        {
            startTTL = prevTTL;
            if(showProbingDetails)
            {
                log += "Will first probe target IP with the TTL of the previous target (TTL = ";
                log += LocationTask::TTLToString(prevTTL) + ").\n";
            }
        }
        else if(showProbingDetails)
        {
            log += "Will conduct full forward probing due to no previous TTL (first TTL ";
            log += "value = " + LocationTask::TTLToString(startTTL) + ").\n";
        }
        
        try
        {
            // 1) Forward probing
            bool res = this->forwardProbing(curTarget, startTTL);
            if(res == false)
            {
                if(showProbingDetails)
                {
                    log += "Reached maximum allowed TTL value (= ";
                    log += LocationTask::TTLToString(MAX_TTL_ALLOWED) + ") or maximum amount of ";
                    log += "consecutive anonymous hops (= ";
                    log += LocationTask::TTLToString(MAX_CONSECUTIVE_ANONYMOUS_HOPS) + ") ";
                    log += "while probing. Skipping this target IP.\n\n";
                }
                
                if(timeoutChanged)
                    prober->setTimeout(initialTimeout);
                
                Thread::invokeSleep(env.getProbingThreadDelay());
                continue;
            }
            else if(showProbingDetails)
            {
                log += curTarget->toStringSimple() + " first replied at TTL = ";
                log += LocationTask::TTLToString(curTarget->getTTL()) + ".\n";
            }
            
            // 2) Backward probing if found TTL equals startTTL or if the last hop is anonymous
            unsigned char foundTTL = curTarget->getTTL();
            bool replyAtStartTTL = (foundTTL == startTTL && (unsigned short) foundTTL > 1);
            bool problematicEnding = curTarget->anonymousEndOfRoute();
            if(replyAtStartTTL || problematicEnding)
            {
                if(showProbingDetails)
                {
                    if(replyAtStartTTL)
                    {
                        log += "Will conduct backward probing due to discovered TTL being equal ";
                        log += "to the initial TTL value used for probing.\n";
                    }
                    else
                    {
                        log += "Will conduct backward probing due to the last hop in the route ";
                        log += "discovered during forward probing being anonymous.\n";
                    }
                }
                
                bool res = this->backwardProbing(curTarget);
                if(res && showProbingDetails)
                {
                    unsigned char newTTL = curTarget->getTTL();
                    if(newTTL != foundTTL)
                        log += "Corrected TTL is " + LocationTask::TTLToString(newTTL) + ".\n";
                    else
                        log += "Minimum TTL was correct from the start.\n";
                }
                else if(showProbingDetails)
                {
                    log += "Couldn't probe backwards because of several consecutive timeouts.\n";
                }
            }
            
            // 3) Computes the trail and completes the route in the process if needed
            this->setTrail(curTarget);
        }
        catch(const SocketException &se)
        {
            this->stop();
            return;
        }
        
        // In case of slightly verbose mode, appends the discovered route
        if(showProbingDetails)
        {
            log += "\nDiscovered route";
            if(!curTarget->hasCompleteRoute())
                log += " (partial)";
            log += ":\n" + curTarget->routeToString() + "\n";
        }
        
        // Puts back the initial timeout (for next target IP)
        if(timeoutChanged)
            prober->setTimeout(initialTimeout);
        
        prevTTL = curTarget->getTTL(); // Initial probe TTL for the next target
        Thread::invokeSleep(env.getProbingThreadDelay()); // Some delay before probing next target
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
