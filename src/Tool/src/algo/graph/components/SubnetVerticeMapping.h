/*
 * SubnetVerticeMapping.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * A simple class that basically amounts to a mapping between a subnet and a vertice (in a network 
 * graph) in the vicinity of which it appears. It is used primarily to implement a "subnet map" 
 * which will give the vertice the subnet belongs to. Such map is implemented in the same way as 
 * the IP dictionnary and operates a time/memory trade-off for fast look-up of an entry 
 * corresponding to a subnet encompassing a given IP.
 * 
 * This class already existed under the name "SubnetMapEntry" in SAGE v1 (originally written in 
 * November 2017), which was itself taken from an older tool (TreeNET; circa 2016).
 */

#ifndef SUBNETVERTICEMAPPING_H_
#define SUBNETVERTICEMAPPING_H_

#include "../../structure/Subnet.h"
#include "Vertice.h"

class SubnetVerticeMapping
{
public:

    SubnetVerticeMapping(Subnet *subnet, Vertice *vertice);
    SubnetVerticeMapping(); // Empty mapping
    ~SubnetVerticeMapping();
    
    inline bool isEmpty() { return (subnet == NULL || vertice == NULL); }
    
    Subnet *subnet;
    Vertice *vertice;
    
    // Normally implicitely added by compiler; declared to signal it's being used
    SubnetVerticeMapping &operator=(const SubnetVerticeMapping &other);
    
    static bool compare(SubnetVerticeMapping &svm1, SubnetVerticeMapping &svm2);
    
};

#endif /* SUBNETVERTICEMAPPING_H_ */
