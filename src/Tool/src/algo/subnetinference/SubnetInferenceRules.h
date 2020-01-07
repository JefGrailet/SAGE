/*
 * SubnetInferenceRules.h
 *
 *  Created on: Jul 2, 2019
 *      Author: jefgrailet
 *
 * Class exclusively providing constants for subnet inference and static methods. These methods 
 * correspond to the five inference rules used for subnet discovery. They are isolated from the 
 * rest of the code because they are involved in both the inference itself and the 
 * post-processing, which are separate steps conducted by different classes. Subnet inference 
 * rules used to be implemented as private methods of the SubnetInferrer class.
 */

#ifndef SUBNETINFERENCERULES_H_
#define SUBNETINFERENCERULES_H_

#include "../Environment.h"

class SubnetInferenceRules
{
public:

    const static unsigned short MINIMUM_PREFIX_LENGTH = 20;
    const static unsigned short MAXIMUM_NB_CONTRAPIVOTS = 5;

    /*
     * Subnet inference rules. Note how some methods present additional parameters.
     * -Rule 2 can lead to the discovery of a better pivot interface, which is passed by the 
     *  newPivot double pointer when a better pivot is discovered.
     * -Rule 4 and 5 need the current alias set in order to check whether the IPs of the trails 
     *  for the pivot IP and the candidate IP are aliases of each other.
     */
    
    static bool rule1(IPTableEntry *pivot, IPTableEntry *candidate);
    static bool rule2(IPTableEntry *pivot, IPTableEntry *candidate, IPTableEntry **newPivot);
    static bool rule3(IPTableEntry *pivot, IPTableEntry *candidate);
    static bool rule4(IPTableEntry *pivot, IPTableEntry *candidate, AliasSet *aliases);
    static bool rule5(IPTableEntry *pivot, IPTableEntry *candidate, AliasSet *aliases);

};

#endif /* SUBNETINFERENCERULES_H_ */
