/*
 * DirectUDPWrappedICMPProber.cpp
 *
 *  Created on: Jan 11, 2010
 *      Author: root
 */

#include "DirectUDPWrappedICMPProber.h"
#include "../../common/thread/Thread.h"

#include <netinet/ip_icmp.h>

DirectUDPWrappedICMPProber::DirectUDPWrappedICMPProber(string &attentionMessage, 
                                                       int tcpUdpRoundRobinSocketCount, 
                                                       const TimeVal &timeoutPeriod, 
                                                       const TimeVal &probeRegulatorPausePeriod, 
                                                       unsigned short lowerBoundUDPsrcPort, 
                                                       unsigned short upperBoundUDPsrcPort, 
                                                       unsigned short lowerBoundUDPdstPort, 
                                                       unsigned short upperBoundUDPdstPort, 
                                                       bool verbose):
DirectUDPProber(attentionMessage, 
                tcpUdpRoundRobinSocketCount, 
                timeoutPeriod, 
                probeRegulatorPausePeriod, 
                lowerBoundUDPsrcPort, 
                upperBoundUDPsrcPort, 
                lowerBoundUDPdstPort, 
                upperBoundUDPdstPort, 
                verbose)
{
    usingHighPortNumber = false;
}

DirectUDPWrappedICMPProber::~DirectUDPWrappedICMPProber()
{
}

ProbeRecord DirectUDPWrappedICMPProber::basic_probe(const InetAddress &src, 
                                                    const InetAddress &dst, 
                                                    unsigned short IPIdentifier, 
                                                    unsigned char TTL, 
                                                    bool usingFixedFlowID, 
                                                    unsigned short srcPort, 
                                                    unsigned short dstPort)
{
    unsigned short dstPortBis = dstPort;
    if(usingHighPortNumber)
        dstPortBis = 65535;

    ProbeRecord result = DirectUDPProber::basic_probe(src, 
                                                      dst, 
                                                      IPIdentifier, 
                                                      TTL, 
                                                      usingFixedFlowID, 
                                                      srcPort, 
                                                      dstPortBis);
    
    /*
     * Addition by J.-F. Grailet: in order to use UDP as an alias resolution tool (i.e., an 
     * unlikely high port number is used to get a Port Unreachable message, which contains an IP 
     * of the router who responded), a flag has been added to prevent the usual probe record 
     * edition, such that the original reply can be analyzed. The original library replaces the 
     * values because it identifies the port unreachable reply as a sign the target is live, and 
     * the replaced values eases interpretation of the probe record in other parts of the program.
     */
    
    if(!usingHighPortNumber)
    {
        if(result.getRplyICMPtype() == DirectProber::ICMP_TYPE_DESTINATION_UNREACHABLE)
        {
            if(result.getRplyICMPcode() == DirectProber::ICMP_CODE_PORT_UNREACHABLE)
            {
                result.setRplyICMPtype(DirectProber::ICMP_TYPE_ECHO_REPLY);
                result.setRplyICMPcode(0);
            }
        }
    }
    return result;
}
