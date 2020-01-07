/*
 * RouterInterface.h
 *
 *  Created on: Oct 28, 2015
 *      Author: jefgrailet
 *
 * A simple class to represent a single interface that is part of an alias.
 *
 * It used to be named "RouterInterface" in former tools (i.e., TreeNET and SAGE v1.0), but has 
 * been refreshed on November 23, 2018 for the needs of a new tool.
 */

#ifndef ALIASEDINTERFACE_H_
#define ALIASEDINTERFACE_H_

#include "IPTableEntry.h"

class AliasedInterface
{
public:

    // Possible methods being used when this IP is associated to a router
    enum AliasMethod
    {
        FIRST_IP, 
        FLICKERING_FINGERPRINT, 
        UDP_PORT_UNREACHABLE, 
        ALLY, 
        IPID_VELOCITY, 
        REVERSE_DNS, 
        GROUP_ECHO, 
        GROUP_ECHO_DNS, 
        GROUP_RANDOM, 
        GROUP_RANDOM_DNS
    };

    AliasedInterface(IPTableEntry *ip, unsigned short aliasMethod);
    ~AliasedInterface();
    
    IPTableEntry *ip;
    unsigned short aliasMethod;
    
    // Comparison method
    static bool smaller(AliasedInterface &i1, AliasedInterface &i2);
};

#endif /* ALIASEDINTERFACE_H_ */
