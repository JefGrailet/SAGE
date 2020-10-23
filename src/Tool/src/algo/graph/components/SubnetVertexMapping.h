/*
 * SubnetVertexMapping.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * A simple class that basically amounts to a mapping between a subnet and a vertex (in a network 
 * graph) in the vicinity of which it appears. It is used primarily to implement a "subnet map" 
 * which will give the vertex the subnet belongs to. Such map is implemented in the same way as 
 * the IP dictionnary and operates a time/memory trade-off for fast look-up of an entry 
 * corresponding to a subnet encompassing a given IP.
 * 
 * This class already existed under the name "SubnetMapEntry" in SAGE v1 (originally written in 
 * November 2017), which was itself taken from an older tool (TreeNET; circa 2016).
 */

#ifndef SUBNETVERTEXMAPPING_H_
#define SUBNETVERTEXMAPPING_H_

#include "../../structure/Subnet.h"
#include "Vertex.h"

class SubnetVertexMapping
{
public:

    SubnetVertexMapping(Subnet *subnet, Vertex *vertex);
    SubnetVertexMapping(); // Empty mapping
    ~SubnetVertexMapping();
    
    inline bool isEmpty() { return (subnet == NULL || vertex == NULL); }
    
    Subnet *subnet;
    Vertex *vertex;
    
    // Normally implicitely added by compiler; declared to signal it's being used
    SubnetVertexMapping &operator=(const SubnetVertexMapping &other);
    
    static bool compare(SubnetVertexMapping &svm1, SubnetVertexMapping &svm2);
    
};

#endif /* SUBNETVERTEXMAPPING_H_ */
