/*
 * TargetPrescanningUnit.h
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes successively each IP it is given (via a list), with a 
 * very large TTL (at this point, all we want is to known if the IP is responsive). It is the 
 * thread class used by the target pre-scanning phase (see TargetPrescanner class).
 */

#ifndef TARGETPRESCANNINGUNIT_H_
#define TARGETPRESCANNINGUNIT_H_

#include <list>
using std::list;

#include "../Environment.h"
#include "../../common/thread/Runnable.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"
#include "TargetPrescanner.h"

class TargetPrescanningUnit : public Runnable
{
public:

    static const unsigned char VIRTUALLY_INFINITE_TTL = (unsigned char) 255;

    // Mutual exclusion object used when accessing TargetPrescanner
    static Mutex prescannerMutex;
    
    // Constructor
    TargetPrescanningUnit(Environment &env, 
                          TargetPrescanner &parent, 
                          list<InetAddress> IPsToProbe, 
                          unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID, 
                          unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID, 
                          unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                          unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
    
    // Destructor, run method
    ~TargetPrescanningUnit();
    void run();
    
private:
    
    // Pointer to environment singleton (with prober parameters)
    Environment &env;
    
    // Private fields
    TargetPrescanner &parent;
    list<InetAddress> IPsToProbe;
    
    // Results for this thread; will be sent to the parent at the end
    list<InetAddress> responsive, unresponsive;

    // Prober object and probing methods (no TTL asked, since it is here virtually infinite)
    DirectProber *prober;
    ProbeRecord probe(const InetAddress &dst);
    
    // "Stop" method (when resources are lacking)
    void stop();

};

#endif /* TARGETPRESCANNINGUNIT_H_ */
