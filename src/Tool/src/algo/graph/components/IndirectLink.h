/*
 * IndirectLink.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Models an indirect link. An indirect link is an edge that connects two consecutive 
 * neighborhoods (or vertices) for which the medium (i.e., the subnet encompassing a trail used to 
 * denote the second neighborhood) either could not be found, either is located remotely - that 
 * is, in the vicinity of another vertex/neighborhood. Contrary to DirectLink, the subnet is 
 * modeled by a SubnetVertexMapping object (void object if no medium).
 *
 * A similar class already existed in SAGE v1 (originally written in November 2017).
 */

#ifndef INDIRECTLINK_H_
#define INDIRECTLINK_H_

#include "Edge.h"
#include "SubnetVertexMapping.h"

class IndirectLink : public Edge
{
public:

    IndirectLink(Vertex *tail, Vertex *head, SubnetVertexMapping &medium);
    IndirectLink(Vertex *tail, Vertex *head); // To use when no medium could be found
    ~IndirectLink();
    
    inline bool hasMedium() { return !medium.isEmpty(); }
    inline SubnetVertexMapping &getMedium() { return medium; }
    
    string toString(bool verbose);

protected:
    
    SubnetVertexMapping medium;
    
};

#endif /* INDIRECTLINK_H_ */
