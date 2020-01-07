/*
 * AliasHintsCollector.h
 *
 *  Created on: Feb 27, 2015
 *      Author: jefgrailet
 *
 * Provided a list of IPs, this class spawns a series of threads probing each IP with different 
 * techniques meant to collect data (a.k.a. "alias hints") to eventually fingerprint the target 
 * IPs and alias them together (when possible, and relevant). Among others, some threads will 
 * probe for IP identifiers (or IP-IDs), others will send a single UDP probe to get a reply fit 
 * for the "iffinder" alias resolution method, etc.
 * 
 * It's worth noting the class is also responsible for synchronizing the probing with IP-ID 
 * collection by providing a token counter. Indeed, each probe for an IP-ID will have an 
 * associated token in order to keep track of the probing order and to be able to use the Ally 
 * alias resolution method. The class also implements a mechanism to detect out-of-order replies 
 * (only for IP-ID collection) and re-order them.
 *
 * N.B.: here, the TTL of each probe is a constant.
 */

#ifndef ALIASHINTSCOLLECTOR_H_
#define ALIASHINTSCOLLECTOR_H_

#include <map>
using std::map;
using std::pair;

#include "../Environment.h"
#include "../../prober/DirectProber.h"
#include "../../prober/icmp/DirectICMPProber.h"
#include "IPIDTuple.h"

class AliasHintsCollector
{
public:

    // Constructor, destructor
    AliasHintsCollector(Environment &env);
    ~AliasHintsCollector();
    
    // Setters for the list of IPs to probe
    inline void setIPsToProbe(list<IPTableEntry*> IPsToProbe) { this->IPsToProbe = IPsToProbe; }
    
    // Methods to start the probing (note: this empties the IPsToProbe list)
    void collect();
    
    // Method to get a token (used by IPIDUnit objects)
    unsigned long int getProbeToken();
    
    // Method to let external methods know if every step is announced in the console output.
    inline bool isPrintingSteps() { return this->printSteps; }
    inline bool debugMode() { return this->debug; }
    
private:

    // Pointer to the environment singleton (=> probing parameters)
    Environment &env;

    // Very own private fields
    list<IPTableEntry*> IPsToProbe;
    unsigned long int tokenCounter;
    
    // Debug stuff
    bool printSteps, debug;

};

#endif /* ALIASHINTSCOLLECTOR_H_ */
