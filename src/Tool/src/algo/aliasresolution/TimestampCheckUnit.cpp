/*
 * TimestampCheckUnit.cpp
 *
 *  Created on: April 13, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in TimestampCheckUnit.h (see this file to learn more about the 
 * goals of such a class).
 */

#include "TimestampCheckUnit.h"
#include "../../common/thread/Thread.h"

TimestampCheckUnit::TimestampCheckUnit(Environment &e, 
                                       IPTableEntry *IP, 
                                       unsigned short lbii, 
                                       unsigned short ubii, 
                                       unsigned short lbis, 
                                       unsigned short ubis):
env(e), 
IPToProbe(IP)
{
    // Initial timeout
    baseTimeout = env.getTimeoutPeriod();
    
    // If a higher timeout is suggested for this IP, uses it
    TimeVal suggestedTimeout = IPToProbe->getPreferredTimeout();
    if(suggestedTimeout > baseTimeout)
        baseTimeout = suggestedTimeout;

    // Instantiates ICMP probing object (necessarily ICMP)
    try
    {
        prober = new DirectICMPProber(env.getAttentionMessage(), 
                                      baseTimeout, 
                                      env.getProbeRegulatingPeriod(), 
                                      lbii, 
                                      ubii, 
                                      lbis, 
                                      ubis, 
                                      env.debugMode());
        
        ((DirectICMPProber*) prober)->useTimestampRequests();
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
}

TimestampCheckUnit::~TimestampCheckUnit()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

void TimestampCheckUnit::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

ProbeRecord TimestampCheckUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env.getLocalIPAddress();
    ProbeRecord record = DirectProber::VOID_RECORD;
    
    // Never using fixedFlowID in these circumstances
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, false);
    }
    catch(const SocketException &e)
    {
        throw;
    }

    return record;
}

void TimestampCheckUnit::run()
{
    InetAddress target((InetAddress) (*IPToProbe));
    
    ProbeRecord newProbe = DirectProber::VOID_RECORD;
    try
    {
        newProbe = this->probe(target, PROBE_TTL);
    }
    catch(const SocketException &se)
    {
        this->stop();
        return;
    }
    
    if(newProbe.getRplyICMPtype() == DirectProber::ICMP_TYPE_TS_REPLY)
    {
        // Sometimes, the replying address is not the target: we consider the target does not reply
        if(newProbe.getRplyAddress() == target)
        {
            // TODO ? Check if replying address is actually an alias of the target or not
            
            AliasHints &curHints = IPToProbe->getLattestHints();
            if(!curHints.isEmpty()) // Just in case
                curHints.setReplyingToTSRequest(true);
        }
    }
}
