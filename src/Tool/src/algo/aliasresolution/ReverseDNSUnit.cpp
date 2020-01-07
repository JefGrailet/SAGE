/*
 * ReverseDNSUnit.cpp
 *
 *  Created on: Mar 4, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in ReverseDNSUnit.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "ReverseDNSUnit.h"

ReverseDNSUnit::ReverseDNSUnit(Environment &e, IPTableEntry *IP):
env(e), 
IPToProbe(IP)
{
}

ReverseDNSUnit::~ReverseDNSUnit()
{
}

void ReverseDNSUnit::run()
{
    AliasHints &curHints = IPToProbe->getLattestHints();
    if(curHints.isEmpty()) // Just in case
        return;

    // Gets host name
    InetAddress target((InetAddress) (*IPToProbe));
    string hostName = target.getHostName();
    if(!hostName.empty())
        curHints.setHostName(hostName);
}
