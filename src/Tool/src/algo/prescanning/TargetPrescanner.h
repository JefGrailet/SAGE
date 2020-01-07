/*
 * TargetPrescanner.h
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * TargetPrescanner is a class designed to send probes to a given list of initial targets (one 
 * probe per target) and save the responsive ones in the IP dictionnary (IPLookUpTable). 
 * Unresponsive IPs are listed by the same class as well, in order to perform a second/third/... 
 * opinion probing with different parameters (e.g.: larger timeout period). These parameters are 
 * saved for each responsive IP in the dictionary to ensure getting a response from them in the 
 * subsequent probing steps.
 *
 * N.B.: the class used to be named "NetworkPrescanner" in previous programs using similar 
 * elements (e.g., TreeNET, SAGE v1.0) and hasn't changed much algorithmically since its creation.
 */

#ifndef TARGETPRESCANNER_H_
#define TARGETPRESCANNER_H_

#include <list>
using std::list;

#include "../Environment.h"

class TargetPrescanner
{
public:

    const static unsigned short MINIMUM_TARGETS_PER_THREAD = 2;

    // Constructor, destructor
    TargetPrescanner(Environment &env);
    ~TargetPrescanner();
    
    // Methods to configure the prescanner
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeout = timeout; }
    bool hasUnresponsiveTargets();
    void reloadUnresponsiveTargets();
    
    // Accesser to timeout field (for children threads)
    inline TimeVal getTimeoutPeriod() { return this->timeout; }
    
    // Callback method (for children threads; ensures concurrent access to IP dictionary)
    void callback(list<InetAddress> responsive, list<InetAddress> unresponsive);
    
    // Launches a pre-scanning (i.e. probing once all IPs from targets)
    void probe();
    
    // Performs the whole pre-scanning phase. This slightly simplifies Main.cpp.
    void run(list<InetAddress> targets);
    
private:

    // Pointer to the environment singleton (provides access to the subnet set and prober parameters)
    Environment &env;
    
    // Timeout period for the current scan
    TimeVal timeout;
    
    // List of targets
    list<InetAddress> targets;
    list<InetAddress> unresponsiveTargets; // From a previous probing, thus empty at first
    
    /*
     * N.B.: responsive targets are not stored in a third list, and directly saved in the IP 
     * look-up table/dictionary (which is accessible through env).
     */
}; 

#endif /* TARGETPRESCANNER_H_ */
