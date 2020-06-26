/*
 * UDPUnreachablePortUnit.cpp
 *
 *  Created on: April 15, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in UDPUnreachablePortUnit.h (see this file to learn more about the 
 * goals of such a class).
 */

#include "UDPUnreachablePortUnit.h"
#include "../../common/thread/Thread.h"

UDPUnreachablePortUnit::UDPUnreachablePortUnit(Environment &e, 
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

    // Instantiates UDP probing object
    try
    {
        int roundRobinSocketCount = DirectProber::DEFAULT_TCP_UDP_ROUND_ROBIN_SOCKET_COUNT;
        if(env.usingFixedFlowID())
            roundRobinSocketCount = 1;
        
        prober = new DirectUDPWrappedICMPProber(env.getAttentionMessage(), 
                                                roundRobinSocketCount, 
                                                baseTimeout, 
                                                env.getProbeRegulatingPeriod(), 
                                                lbii, 
                                                ubii, 
                                                lbis, 
                                                ubis, 
                                                env.debugMode());
        
        ((DirectUDPWrappedICMPProber*) prober)->useHighPortNumber();
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

UDPUnreachablePortUnit::~UDPUnreachablePortUnit()
{
    if(prober != NULL)
    {
        env.updateProbeAmounts(prober);
        delete prober;
    }
}

void UDPUnreachablePortUnit::stop()
{
    Environment::emergencyStopMutex.lock();
    env.triggerStop();
    Environment::emergencyStopMutex.unlock();
}

ProbeRecord UDPUnreachablePortUnit::probe(const InetAddress &dst, unsigned char TTL)
{
    InetAddress localIP = env.getLocalIPAddress();
    bool usingFixedFlow = env.usingFixedFlowID();
    
    ProbeRecord record = DirectProber::VOID_RECORD;
    try
    {
        record = prober->singleProbe(localIP, dst, TTL, usingFixedFlow);
    }
    catch(const SocketException &e)
    {
        throw;
    }

    return record;
}

void UDPUnreachablePortUnit::run()
{
    InetAddress target((InetAddress) (*IPToProbe));
    
    // Tries to get a "Port unreachable error"
    ProbeRecord newProbe = DirectProber::VOID_RECORD;
    try
    {
        newProbe = probe(target, PROBE_TTL);
    }
    catch(const SocketException &se)
    {
        this->stop();
        return;
    }
    
    // We register something only if we get an ICMP code "PORT UNREACHABLE"
    if(newProbe.getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE)
    {
        InetAddress replyingAddress = newProbe.getRplyAddress();
        AliasHints &curHints = IPToProbe->getLattestHints();
        if(!curHints.isEmpty()) // Just in case
            curHints.setPortUnreachableSrcIP(replyingAddress);
    }
}
