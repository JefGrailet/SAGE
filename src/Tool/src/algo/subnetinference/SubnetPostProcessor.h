/*
 * SubnetPostProcessor.h
 *
 *  Created on: Jul 3, 2019
 *      Author: jefgrailet
 *
 * This class is responsible for post-processing the subnets discovered with the methodology 
 * implemented by the SubnetInferrer class. Indeed, the inferred subnets can be undergrown under 
 * some circonstances, mainly when the contra-pivot(s) is (are) located in an unusual part of the 
 * address space. Indeed, the "direction" of inference assumes contra-pivots usually appear among 
 * the first IPs of the address space, because it's a very common practice with large subnets. 
 * Therefore, if the contra-pivot(s) rather appear among the last IPs of the subnet, then the 
 * subnet might not be fully inferred (inference stops when a contra-pivot is discovered) and can 
 * appear in chunks, the "lower" parts of it being advertised as undergrown.
 *
 * The post-processing phase aims at processing subnets in the opposite "direction" w.r.t. the 
 * inference (i.e., it starts with the smallest prefix w.r.t. the IP scope) to re-construct 
 * subnets that might be chunked in several parts.
 */

#ifndef SUBNETPOSTPROCESSOR_H_
#define SUBNETPOSTPROCESSOR_H_

#include "../Environment.h"
#include "MergingCandidate.h"

class SubnetPostProcessor
{
public:
    
    static MergingCandidate NOT_MERGEABLE;
    
    SubnetPostProcessor(Environment &env);
    ~SubnetPostProcessor();
    
    void process();
    
private:

    // Pointer to the environment singleton
    Environment &env;
    
    /*** Private utility methods ***/
    
    /*
     * Checks that two pivots are compatible together, i.e., a least one rule of inference shows 
     * they are on the same subnet. The reference pivot is given as a pointer of a pointer since 
     * it can change due to rule 2 (see SubnetInferenceRules class).
     *
     * @param IPTableEntry** refPivot  Reference pivot for merging
     * @param IPTableEntry* cmpPivot   Pivot to compare to the reference
     * @return bool                    True if pivots are compatible
     */
    
    bool compatiblePivots(IPTableEntry **refPivot, IPTableEntry *cmpPivot);
    
    /*
     * Checks that a bunch of hypothetical pivots (given as a list) are compatible together, 
     * re-using the above method by using the first pivot of the bunch as a reference. This method 
     * is useful for the case where there are several contra-pivots or several outliers that are 
     * suspected to be actually pivots of a larger subnet (hence why one needs to ensure they 
     * belong to the same subnet through the inference rules).
     *
     * @param list<IPTableEntry*> pivots  List of hypothetical pivots to check
     * @return bool                       True of listed pivots are on the same subnet
     */
    
    bool compatiblePivots(list<IPTableEntry*> pivots);
    
    /*
     * Given a reference pivot, checks that a candidate subnet is compatible with it, producing 
     * a MergingCandidate object in the end which will maintain the reason why the candidate 
     * subnet could be a valid merging candidate. Of course, decision to actually perform the 
     * merge operation can only be taken after considering all encompassed subnets after the 
     * expansion of one initial undergrown subnet.
     *
     * @param IPTableEntry** refPivot  Reference pivot for merging
     * @param Subnet* candidate        Subnet to evaluate for a merging scenario
     * @return MergingCandidate*       A new MergingCandidate object keeping track of the 
     *                                 motivation for merging, or a "UNMERGEABLE" object is 
     *                                 merging is out of question
     */
    
    MergingCandidate testCandidate(IPTableEntry **refPivot, Subnet *candidate);

    /*
     * Given a merging candidate, checks if it's compatibly through contra-pivots or outliers, or 
     * just outliers, then proceed to re-label the interfaces (i.e. their status or rule of 
     * inference showing that they indeed belong to the subnet). It's worth noting such an 
     * operation only occurs right before the actual merging, when such a merging operation is 
     * already decided.
     *
     * @param MergingCandidate& candidate  The candidate for which re-labeling IPs might be needed
     */    
    
    void relabel(MergingCandidate &candidate);

}; 

#endif /* SUBNETPOSTPROCESSOR_H_ */
