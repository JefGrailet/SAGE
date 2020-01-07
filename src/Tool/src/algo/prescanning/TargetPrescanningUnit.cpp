/*
 * TargetPrescanningUnit.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in TargetPrescanningUnit.h (see this file to learn further about 
 * the goals of such a class).
 */

#include "TargetPrescanningUnit.h"

Mutex TargetPrescanningUnit::prescannerMutex(Mutex::ERROR_CHECKING_MUTEX);

TargetPrescanningUnit::TargetPrescanningUnit(Environment &e, 
                                             TargetPrescanner &p, 
                                             list<InetAddress> IPs, 
                                             unsigned short lbii, 
                                             unsigned short ubii, 
                                             unsigned short lbis, 
                                             unsigned short ubis):
env(e), 
parent(p), 
IPsToProbe(IPs)
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
                                                    parent.getTimeoutPeriod(), 
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
                                                    parent.getTimeoutPeriod(), 
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
                                          parent.getTimeoutPeriod(), 
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
    
    if(env.debugMode())
    {
        Environment::consoleMessagesMutex.lock();
        ostream *out = env.getOutputStream();
        (*out) << prober->getAndClearLog();
        Environment::consoleMessagesMutex.unlock();
    }
}

TargetPrescanningUnit::~TargetPrescanningUnit()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

ProbeRecord TargetPrescanningUnit::probe(const InetAddress &dst)
{
    InetAddress localIP = env.getLocalIPAddress();
    bool usingFixedFlow = env.usingFixedFlowID();
    unsigned char TTL = VIRTUALLY_INFINITE_TTL;

    ProbeRecord record = DirectProber::VOID_RECORD;
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow);
    }
    catch(SocketException &se)
    {
        throw;
    }
    
    // Debug log
    if(env.debugMode())
    {
        Environment::consoleMessagesMutex.lock();
        ostream *out = env.getOutputStream();
        (*out) << prober->getAndClearLog();
        Environment::consoleMessagesMutex.unlock();
    }

    return record;
}

void TargetPrescanningUnit::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

void TargetPrescanningUnit::run()
{
    for(list<InetAddress>::iterator it = IPsToProbe.begin(); it != IPsToProbe.end(); ++it)
    {
        InetAddress curIP = (*it);
        ProbeRecord probeRecord = DirectProber::VOID_RECORD;
        
        try
        {
            probeRecord = this->probe(curIP);
        }
        catch(SocketException &se)
        {
            this->stop();
            return;
        }
        
        if(probeRecord.isVoidRecord()) // Just in case
        {
            unresponsive.push_back(curIP);
            continue;
        }
        
        InetAddress replyingIP = probeRecord.getRplyAddress();
        unsigned char replyType = probeRecord.getRplyICMPtype();
        if(!probeRecord.isAnonymousRecord() && replyType == DirectProber::ICMP_TYPE_ECHO_REPLY && replyingIP == curIP)
            responsive.push_back(curIP);
        else
            unresponsive.push_back(curIP);
        
        if(env.isStopping())
            return;
    }
    
    prescannerMutex.lock();
    parent.callback(responsive, unresponsive);
    prescannerMutex.unlock();
}
