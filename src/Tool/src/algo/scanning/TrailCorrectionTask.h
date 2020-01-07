/*
 * TrailCorrectionTask.h
 *
 *  Created on: Sep 19, 2018
 *      Author: jefgrailet
 *
 * This class, inheriting Runnable, probes a list of target IPs which were already probed via 
 * LocationTask threads but which the trail features a strictly positive amount of anomalies. The 
 * task re-probes all IPs at the TTL at which they were located previously minus 1, 2... up to the 
 * amount of anomalies found in the trail. Some delays ensure the different target IPs are not 
 * re-probed too aggressively, because doing so on IPs that are close to each other address-wise 
 * might trigger more timeouts and prevent the correction of trails.
 */

#ifndef TRAILCORRECTIONTASK_H_
#define TRAILCORRECTIONTASK_H_

#include "../Environment.h"
#include "../../common/thread/Runnable.h"
#include "../../common/thread/Mutex.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "../../prober/udp/DirectUDPWrappedICMPProber.h"
#include "../../prober/tcp/DirectTCPWrappedICMPProber.h"
#include "../../prober/exception/SocketException.h"
#include "../../prober/structure/ProbeRecord.h"

class TrailCorrectionTask : public Runnable
{
public:
    
    // Constructor
    TrailCorrectionTask(Environment &env, 
                        list<IPTableEntry*> targets, 
                        unsigned short lowerBoundICMPid = DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER,
                        unsigned short upperBoundICMPid = DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER,
                        unsigned short lowerBoundICMPseq = DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE,
                        unsigned short upperBoundICMPseq = DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE);
    
    // Destructor, run method and print out method
    ~TrailCorrectionTask();
    void run();
    
private:

    // Pointer to the environment singleton
    Environment &env;
    
    // Pointer to the target list
    list<IPTableEntry*> targets;
    
    // Probing stuff
    DirectProber *prober;
    ProbeRecord probe(IPTableEntry *target, unsigned char TTL);
    
    // "Stop" method (when resources are lacking)
    void stop();
    
    // Verbosity/debug stuff
    bool showProbingDetails, debugMode;
    string log;
    
    // Utility method to convert a TTL (unsigned char) into string format.
    static string TTLToString(unsigned char TTL);
    
};

#endif /* TRAILCORRECTIONTASK_H_ */
