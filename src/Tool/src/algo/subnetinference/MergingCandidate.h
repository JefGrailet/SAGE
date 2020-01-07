/*
 * MergingCanditate.h
 *
 *  Created on: Jul 4, 2019
 *      Author: jefgrailet
 *
 * A data structure very similar to SubnetInterface in implementation, which gathers a subnet and 
 * a few integers maintaining the amounts of each type of interface (i.e., pivots, contra-pivots 
 * and outliers) and what merging scenario this subnet fits. Having this data structure is useful 
 * for two reasons:
 * -as subnets will be removed from the various lists only after an expansion has been validated, 
 *  one can keep track of why a subnet is okay for the merging scenario without modifying the 
 *  content of the lists.
 * -knowing that a subnet is only compatible through contra-pivot(s)/outliers allows the 
 *  post-processing to force a pivot switch on the subnet before doing the final merging and 
 *  tell accurately why the new pivots are indeed on the subnet.
 * Some utility methods are also available.
 */

#ifndef MERGINGCANDIDATE_H_
#define MERGINGCANDIDATE_H_

#include "../structure/Subnet.h"

class MergingCandidate
{
public:

    enum Compatibility
    {
        UNMERGEABLE, // Used for a return value of a private method of SubnetPostProcessor
        COMPATIBLE_PIVOT, // Reference pivot and pivot interfaces are compatible
        COMPATIBLE_OUTLIER, // Reference pivot is compatible, but with outliers
        COMPATIBLE_CONTRAPIVOT, // Reference pivot is compatible, but with contra-pivot(s)
        OUTLIERS // Subnet has few pivot interfaces (<= 2) which would be outliers
    };
    
    MergingCandidate(); // Creates an "UNMERGEABLE" candidate
    MergingCandidate(Subnet *candidate, unsigned short compatibility);
    MergingCandidate(const MergingCandidate &other);
    ~MergingCandidate();
    
    // Normally implicitely declared by compiler; provided to signal explicitely it's relevant
    MergingCandidate &operator=(const MergingCandidate &other);
    
    inline bool isUnmergeable() { return compatibility == UNMERGEABLE; }
    
    Subnet *candidate;
    unsigned short compatibility;
    unsigned short nbPivots, nbContrapivots, nbOutliers;
    
    // Tells in a single message whether this candidate is compatible or not
    inline bool isCompatible() { return !(compatibility == OUTLIERS); }
    
    // Returns the TTL of the (new) pivots if it's always the same, 255 otherwise
    unsigned char getPivotTTL();
    
    // Gets the smallest TTL value witness among interfaces of this candidate
    inline unsigned char getSmallestTTL() { return candidate->getSmallestTTL(); }
    
    // toString() only used for debug
    string toString();
    
    // Comparison method
    static bool smaller(MergingCandidate &c1, MergingCandidate &c2);

};

#endif /* MERGINGCANDIDATE_H_ */
