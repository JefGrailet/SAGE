/*
 * Graph.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Graph.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Graph.h"

SubnetVerticeMapping Graph::VOID_MAPPING = SubnetVerticeMapping();

Graph::Graph()
{
    nbVertices = 0; // To be set later (via "Pioneer" voyager)
    subnetMap = new list<SubnetVerticeMapping>[SIZE_SUBNET_MAP];
}

Graph::~Graph()
{
    delete[] subnetMap;
    gates.clear(); // Nodes will be deleted via the "Mariner" voyager
}

void Graph::createSubnetMappingsTo(Vertice *v)
{
    list<Subnet*> *subnets = v->getSubnets();
    for(list<Subnet*>::iterator i = subnets->begin(); i != subnets->end(); ++i)
    {
        unsigned long index = ((*i)->getLowerBorder().getULongAddress() >> 12);
        subnetMap[index].push_back(SubnetVerticeMapping((*i), v));
    }
}

void Graph::sortMappings()
{
    for(unsigned int i = 0; i < SIZE_SUBNET_MAP; i++)
        if(subnetMap[i].size() > 1)
            subnetMap[i].sort(SubnetVerticeMapping::compare);
}

SubnetVerticeMapping &Graph::getSubnetContaining(InetAddress needle)
{
    /*
     * Remark on implementation: "subnetList" must be at least a reference, because otherwise 
     * we use a copy of the list from "subnetMap" array, itself providing copies of the mappings 
     * rather than the stored mappings. As a consequence, the selected object would disappear from 
     * the memory once this method returns a reference to it.
     */

    unsigned long index = (needle.getULongAddress() >> 12);
    list<SubnetVerticeMapping> &subnetList = subnetMap[index];
    for(list<SubnetVerticeMapping>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
        if(i->subnet->contains(needle))
            return (*i);
    return VOID_MAPPING;
}
