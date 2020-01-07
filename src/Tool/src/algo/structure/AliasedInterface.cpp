/*
 * AliasedInterface.cpp
 *
 *  Created on: Oct 28, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in AliasedInterface.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "AliasedInterface.h"

AliasedInterface::AliasedInterface(IPTableEntry *ip, unsigned short aliasMethod)
{
    this->ip = ip;
    this->aliasMethod = aliasMethod;
}

AliasedInterface::~AliasedInterface()
{
}

bool AliasedInterface::smaller(AliasedInterface &i1, AliasedInterface &i2)
{
    return IPTableEntry::compare(i1.ip, i2.ip);
}
