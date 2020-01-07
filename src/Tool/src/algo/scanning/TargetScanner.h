/*
 * TargetScanner.h
 *
 *  Created on: Sep 7, 2018
 *      Author: jefgrailet
 *
 * This class schedules the LocationTask threads to evaluate the TTL distance and trail of each 
 * target IP that passed the pre-scanning. It then schedules TrailCorrectionTask threads, if 
 * needed, to improve the trails for IPs with which timeout occurred while evaluating them.
 */

#ifndef TARGETSCANNER_H_
#define TARGETSCANNER_H_

#include "../Environment.h"
#include "LocationTask.h"
#include "TrailCorrectionTask.h"

class TargetScanner
{
public:
    
    // Constructor, destructor
    TargetScanner(Environment &env);
    ~TargetScanner();
    
    void scan(); // Scans IP found in the dictionary
    void finalize(); // Finalizes by detecting problematic trails and aliasing flickering IPs
    
private:

    // Pointer to the environment singleton (provides access to probing parameters)
    Environment &env;
    
    list<IPTableEntry*> targets;
    
    // Counts the amount of IP entries with incomplete trails and IP entries with an estimated TTL
    unsigned int countBadEntries();
    unsigned int countEntriesWithTTL();
    
    /*
     * Filters the entries of the IP dictionnary after the first probing round to list those which 
     * have an incomplete trail or for which the backward probing couldn't be properly completed.
     * It then sorts these entries by TTL then address, and splits them in several lists that can 
     * be probed by distinct threads. The returned list is empty when there's nothing to re-probe.
     *
     * @return list<list<IPTableEntry*> >  List of lists of IP entries to re-probe concurrently
     */
    
    list<list<IPTableEntry*> > reschedule();
    
    /*
     * Utility methods for reschedule():
     * -estimateSplit() checks if a reasonable split if possible, and if yes, picks the split 
     *  which the resulting lists are the closest in size to the size of the initial list / 2, and 
     *  returns the size of the first list (i.e., with the "smaller" IPs) after the split.
     * -splitList() performs the split and returns a list containing the two resulting lists. It 
     *  takes as parameter the return value of estimateSplit() to simplify.
     * -compareLists() is used to sort lists of entries by increasing size. 
     */
    
    unsigned int estimateSplit(list<IPTableEntry*> ls);
    list<list<IPTableEntry*> > splitList(list<IPTableEntry*> ls, unsigned int splitNb);
    static bool compareLists(list<IPTableEntry*> &ls1, list<IPTableEntry*> &ls2);
    
    /*
     * Utility method to build potential aliases with the flickering IPs listed in the dictionary; 
     * called by the finalize() method.
     */
    
    static void addFlickeringPeers(IPTableEntry *IP, 
                                   list<IPTableEntry*> *alias, 
                                   list<IPTableEntry*> *flickIPs);
    
}; 

#endif /* TARGETSCANNER_H_ */
