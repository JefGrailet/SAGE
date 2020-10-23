/*
 * SubnetVertexMapping.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetVertexMapping.h (see this file to learn further about 
 * the goals of such a class).
 */

#include "SubnetVertexMapping.h"

SubnetVertexMapping::SubnetVertexMapping(Subnet *subnet, Vertex *vertex)
{
    this->subnet = subnet;
    this->vertex = vertex;
}

SubnetVertexMapping::SubnetVertexMapping()
{
    subnet = NULL;
    vertex = NULL;
}

SubnetVertexMapping::~SubnetVertexMapping()
{
}

SubnetVertexMapping& SubnetVertexMapping::operator=(const SubnetVertexMapping &other)
{
    this->subnet = other.subnet;
    this->vertex = other.vertex;
    return *this;
}

bool SubnetVertexMapping::compare(SubnetVertexMapping &svm1, SubnetVertexMapping &svm2)
{
    return Subnet::compare(svm1.subnet, svm2.subnet);
}
