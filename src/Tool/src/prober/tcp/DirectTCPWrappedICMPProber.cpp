/*
 * DirectTCPWrappedICMPProber.cpp
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 */

#include "DirectTCPWrappedICMPProber.h"

#include <netinet/ip_icmp.h>

DirectTCPWrappedICMPProber::DirectTCPWrappedICMPProber(string &attentionMessage, 
                                                       int tcpUdpRoundRobinSocketCount, 
                                                       const TimeVal &timeoutPeriod, 
                                                       const TimeVal &probeRegulatorPausePeriod, 
                                                       unsigned short lowerBoundTCPsrcPort, 
                                                       unsigned short upperBoundTCPsrcPort, 
                                                       unsigned short lowerBoundTCPdstPort, 
                                                       unsigned short upperBoundTCPdstPort, 
                                                       bool verbose):
DirectTCPProber(attentionMessage, 
                tcpUdpRoundRobinSocketCount, 
                timeoutPeriod, 
                probeRegulatorPausePeriod, 
                lowerBoundTCPsrcPort, 
                upperBoundTCPsrcPort, 
                lowerBoundTCPdstPort, 
                upperBoundTCPdstPort, 
                verbose)
{

}

DirectTCPWrappedICMPProber::~DirectTCPWrappedICMPProber()
{
}

ProbeRecord DirectTCPWrappedICMPProber::basic_probe(const InetAddress &src, 
                                                     const InetAddress &dst, 
                                                     unsigned short IPIdentifier, 
                                                     unsigned char TTL, 
                                                     bool usingFixedFlowID, 
                                                     unsigned short srcPort, 
                                                     unsigned short dstPort)
{
    ProbeRecord result = DirectTCPProber::basic_probe(src, 
                                                      dst, 
                                                      IPIdentifier, 
                                                      TTL, 
                                                      usingFixedFlowID, 
                                                      srcPort, 
                                                      dstPort);
    
    /*
     * Just like DirectUDPWrappedICMPProber, the code checks the reply we obtained and re-label it 
     * as an echo reply to ease interpretation by other parts of the program. "Pseudo TCP reset" 
     * "port unreachable" reply are both identified as proof the target is live.
     */
    
    if(result.getRplyICMPtype() == DirectProber::PSEUDO_TCP_RESET_ICMP_TYPE || 
       (result.getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE && 
        result.getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE))
    {
        result.setRplyICMPtype(DirectProber::ICMP_TYPE_ECHO_REPLY);
        result.setRplyICMPcode(0);
    }
    return result;
}
