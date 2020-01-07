/*
 * SubnetInterface.cpp
 *
 *  Created on: Nov 27, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetInterface.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "SubnetInterface.h"

SubnetInterface::SubnetInterface(IPTableEntry *ip, unsigned short status)
{
    this->ip = ip;
    this->status = status;
}

SubnetInterface::SubnetInterface()
{
    this->ip = NULL;
    this->status = VOID;
}

SubnetInterface::~SubnetInterface()
{
}

bool SubnetInterface::smaller(SubnetInterface &i1, SubnetInterface &i2)
{
    return IPTableEntry::compare(i1.ip, i2.ip);
}

string SubnetInterface::routeToString()
{
    Trail &trail = ip->getTrail();
    if(partialRoute.size() == 0 || trail.isVoid())
        return "";

    stringstream ss;
    ss << ip->toStringSimple() << " - ";
    unsigned short maxTTL = (unsigned short) (ip->getTTL() - trail.getLengthInTTL());
    for(unsigned short i = 0; i < partialRoute.size(); i++)
    {
        if(i > 0)
            ss << ", ";
        ss << partialRoute[i] << " (TTL=" << (maxTTL - partialRoute.size() + i) << ")";
    }
    return ss.str();
}
