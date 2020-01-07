/*
 * SubnetInferrer.h
 *
 *  Created on: Nov 23, 2018
 *      Author: jefgrailet
 *
 * This class reviews the IP dictionary and uses the collected data to infer subnets in the target 
 * domain. The inference algorithm consists in progressively growing a subnet starting with a 
 * single IP (i.e., a /32 subnet) then decrementing its prefix length and checking the newly 
 * encompassed IPs belong to the same subnet, using inference rules. The growth stops when too 
 * many IPs appear as not being on the same subnet or when one or several contra-pivot(s) (max. 5) 
 * have been discovered (if contra-pivot(s) are present, then the subnet is already sound).
 *
 * Starting from July 2019, this class is only responsible for the inference itself: inference 
 * rules are now members of a SubnetInferenceRules static class while subnet post-processing is 
 * the responsibility of a third class.
 */

#ifndef SUBNETINFERRER_H_
#define SUBNETINFERRER_H_

#include "../Environment.h"

class SubnetInferrer
{
public:
    
    SubnetInferrer(Environment &env);
    ~SubnetInferrer();
    
    void process();
    
private:

    // Pointer to the environment singleton
    Environment &env;
    
    /*
     * Private method to shrink a subnet. The reason why this method exists is because the 
     * shrink() of Subnet class only performs a part of the operations to carry out; in this 
     * context, one also has to restore IPs for subnet inference that were removed from the subnet 
     * at the shrinkage (plus contra-pivots interfaces) and restore the pivot of the last 
     * successful expansion (if pivot changed). Moreover, the whole process is called several 
     * times, hence why it was turned into a single method.
     *
     * @param Subnet* sn                         The subnet to shrink
     * @param list<IPTableEntry*>* IPs           List where IPs must be restored
     * @param IPTableEntry* oldPivot             Pivot of the last expansion (NULL if unchanged)
     * @param list<IPTableEntry*>* contrapivots  List of contra-pivot IPs to restore too
     */
    
    static void fullShrinkage(Subnet *sn, 
                              list<IPTableEntry*> *IPs, 
                              IPTableEntry *oldPivot, 
                              list<IPTableEntry*> *contrapivots = NULL);
    
    /*
     * Private method to find the perform an overgrowth test, i.e., a special verification done 
     * when several contra-pivots appear at one expansion in order to ensure we're not overgrowing 
     * the subnet. It consists in finding for each contra-pivot the largest subnet prefix that 
     * doesn't encompass other contrapivots, then removing from a list of interfaces (i.e. the 
     * interfaces that appeared along the contrapivots during an expansion) all interfaces 
     * encompassed by the same prefix. If there's no interface left after processing all potential 
     * contra-pivots, this means it's possible to find smaller and sounder subnets containing the 
     * candidate contra-pivots, therefore suggesting an overgrowth of the current subnet. 
     * Otherwise, keeping the current prefix is okay.
     *
     * @param list<IPTableEntry*> contrapivots  Contra-pivots for which we will search sub-prefixes
     * @param list<IPTableEntry*> interfaces    Interfaces discovered during this expansion
     * @return bool                             True if there's a risk of overgrowth
     */
    
    static bool overgrowthTest(list<IPTableEntry*> contrapivots, 
                               list<IPTableEntry*> interfaces);

}; 

#endif /* SUBNETINFERRER_H_ */
