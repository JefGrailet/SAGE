/*
 * SubnetVerticeMapping.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetVerticeMapping.h (see this file to learn further about 
 * the goals of such a class).
 */

#include "SubnetVerticeMapping.h"

SubnetVerticeMapping::SubnetVerticeMapping(Subnet *subnet, Vertice *vertice)
{
    this->subnet = subnet;
    this->vertice = vertice;
}

SubnetVerticeMapping::SubnetVerticeMapping()
{
    subnet = NULL;
    vertice = NULL;
}

SubnetVerticeMapping::~SubnetVerticeMapping()
{
}

SubnetVerticeMapping& SubnetVerticeMapping::operator=(const SubnetVerticeMapping &other)
{
    this->subnet = other.subnet;
    this->vertice = other.vertice;
    return *this;
}

bool SubnetVerticeMapping::compare(SubnetVerticeMapping &svm1, SubnetVerticeMapping &svm2)
{
    return Subnet::compare(svm1.subnet, svm2.subnet);
}
