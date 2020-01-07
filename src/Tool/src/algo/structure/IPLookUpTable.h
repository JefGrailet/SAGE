/*
 * IPLookUpTable.h
 *
 *  Created on: Sep 29, 2015
 *      Author: jefgrailet
 *
 * IPLookUpTable is a particular structure built to quickly look up for an IP address and obtain 
 * an object storing data about it, such as alias resolution hints or the timeout value that 
 * should be preferred when probing this IP. The structure should only contain IPs discovered to 
 * be responsive at pre-scanning.
 *
 * The structure is made of an array of lists which are indexed from 0 to 2^X. An IP is stored in 
 * the structure by adding it to the list at the index which matches its X first bits. This 
 * implies that each list can contain at most 2^(32 - X) items, thanks to which the storage and 
 * look-up can be achieved in O(1) as long as X is great enough.
 *
 * The choice of X is theoretically free. For now, it is set to 20, which leads to a 8 Mio array 
 * of lists where each lists can contain up to 4096 elements. Using as much memory for the array is 
 * still reasonable and should not bother any of today's computers.
 */

#ifndef IPLOOKUPTABLE_H_
#define IPLOOKUPTABLE_H_

#include <list>
using std::list;

#include "IPTableEntry.h"

class IPLookUpTable
{
public:
    
    // X = 20
    const static unsigned int SIZE_TABLE = 1048576;
    
    // Constructor, destructor
    IPLookUpTable();
    ~IPLookUpTable();
    
    // Methods to check the content of the dictionary
    bool isEmpty();
    bool hasAliasResolutionData();
    unsigned int getTotalIPs();
    
    // Creation and look-up methods
    IPTableEntry *create(InetAddress needle); // NULL if already existed
    IPTableEntry *lookUp(InetAddress needle); // NULL if not found
    
    // Methods to list specific types of entries
    list<IPTableEntry*> listTargetEntries(); // Targets in order (i.e., w.r.t. the IPv4 scope)
    list<IPTableEntry*> listFlickeringIPs();
    list<IPTableEntry*> listScannedIPs(); // Lists, in order, successfully scanned IPs
    
    void reviewScannedIPs(); // Labels IPs as (un)successfully scanned
    void reviewSpecialIPs(unsigned short maxDelta); // Finds warping, echoing and flickering IPs
    
    // Special method used during peer discovery; tells if the IP has been seen as direct trail
    bool isPotentialPeer(InetAddress needle);
    
    // Output methods
    void outputDictionary(string filename);
    void outputAliasHints(string filename);
    void outputFingerprints(string filename);
    
private:
    
    list<IPTableEntry*> *haystack;
    
    /*
     * Investigates a trio of IPs (as IPTableEntry objects) to see if their trail IPs are 
     * flickering. If there's no flickering, it will return 0, otherwise it returns the "delta", 
     * i.e., the sum of the difference between cur and prev and the difference between prev and 
     * prevPrev (as IPs, re-interpreted as unsigned long). In an ideal flickering scenario, the 
     * delta is close to or equal to 2.
     */
    
    static unsigned short isFlickering(IPTableEntry *cur, 
                                       IPTableEntry *prev, 
                                       IPTableEntry *prevPrev);

};

#endif /* IPLOOKUPTABLE_H_ */
