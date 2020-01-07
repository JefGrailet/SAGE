/*
 * UDPUnreachablePortUnit.h
 *
 *  Created on: April 15, 2016
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes a single IP with a UDP probe, targetting an unlikely 
 * high destination port number. The subsequent ICMP Port Unreachable message (if received) should 
 * contain a source address belonging to the router which bears the initial destination. This 
 * address is not always the same as the initial destination, hence the relevance of the whole 
 * mechanism for alias resolution.
 */

#ifndef UDPUNREACHABLEPORTUNIT_H_
#define UDPUNREACHABLEPORTUNIT_H_

#include "../Environment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/date/TimeVal.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"

class UDPUnreachablePortUnit : public Runnable
{
public:

    // TTL value used in all packets
    static const unsigned char PROBE_TTL = 64;
    
    // Constructor
    UDPUnreachablePortUnit(Environment &env, 
                           IPTableEntry *IPToProbe, 
                           unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                           unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                           unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                           unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE);
    
    // Destructor and run method
    ~UDPUnreachablePortUnit();
    void run();
    
    /*
     * Gets the debug log for this thread. As only one probe is carried out per thread of this 
     * class, there is no need to maintain a log and the method is an alias for the 
     * getAndClearLog() from the prober class.
     */
    
    inline string getDebugLog() { return this->prober->getAndClearLog(); }
    
private:

    // Pointer to the environment object (=> probing parameters)
    Environment &env;
    
    // IP to check
    IPTableEntry *IPToProbe;

    // Probing stuff
    DirectProber *prober;
    ProbeRecord probe(const InetAddress &dst, unsigned char TTL);
    TimeVal baseTimeout;

    // Stop method (when loss of network connectivity)
    void stop();
    
};

#endif /* UDPUNREACHABLEPORTUNIT_H_ */
