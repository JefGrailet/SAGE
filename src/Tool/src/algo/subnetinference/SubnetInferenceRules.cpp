/*
 * SubnetInferenceRules.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetInferenceRules.h (see this file to learn further about 
 * the goals of such a class).
 */

#include "SubnetInferenceRules.h"

// Trail rule

bool SubnetInferenceRules::rule1(IPTableEntry *pivot, IPTableEntry *candidate)
{
    Trail &t1 = pivot->getTrail();
    Trail &t2 = candidate->getTrail();
    if(!t1.isVoid() && !t2.isVoid() && t1 == t2)
        return true;
    return false;
}

// TTL + timeout rule

bool SubnetInferenceRules::rule2(IPTableEntry *pivot, IPTableEntry *candidate, IPTableEntry **newPivot)
{
    if(pivot->getTTL() != candidate->getTTL())
        return false;
    
    Trail &t1 = pivot->getTrail();
    Trail &t2 = candidate->getTrail();
    if(!t1.isVoid() && !t2.isVoid() && t1.getNbAnomalies() != t2.getNbAnomalies())
    {
        // Updates pivot if candidate IP has less anomalies
        if(t2.getNbAnomalies() < t1.getNbAnomalies() && newPivot != NULL)
            (*newPivot) = candidate;
        return true;
    }
    return false;
}

// Echoes rule

bool SubnetInferenceRules::rule3(IPTableEntry *pivot, IPTableEntry *candidate)
{
    if(pivot->getTTL() != candidate->getTTL())
        return false;
    
    Trail &t1 = pivot->getTrail();
    Trail &t2 = candidate->getTrail();
    if(!t1.isVoid() && !t2.isVoid() && t1.isEchoing() && t2.isEchoing())
        return true;
    return false;
}

// Aliased flickering IPs rule

bool SubnetInferenceRules::rule4(IPTableEntry *pivot, IPTableEntry *candidate, AliasSet *aliases)
{
    if(pivot->getTTL() != candidate->getTTL() || aliases == NULL)
        return false;
    
    Trail &t1 = pivot->getTrail();
    Trail &t2 = candidate->getTrail();
    if(!t1.isVoid() && !t2.isVoid() && t1.isFlickering() && t2.isFlickering())
    {
        InetAddress t1IP = t1.getLastValidIP();
        InetAddress t2IP = t2.getLastValidIP();
        
        Alias *toCheck = aliases->findAlias(t1IP);
        if(toCheck != NULL && toCheck->hasInterface(t2IP))
            return true;
    }
    return false;
}

// Aliased IPs rule (generalization of rule 4; applies to warping AND flickering IPs)

bool SubnetInferenceRules::rule5(IPTableEntry *pivot, IPTableEntry *candidate, AliasSet *aliases)
{
    if(aliases == NULL)
        return false;
    
    Trail &t1 = pivot->getTrail();
    Trail &t2 = candidate->getTrail();
    if(t1.isVoid() || t2.isVoid() || t1.getNbAnomalies() > 0 || t2.getNbAnomalies() > 0)
        return false;
        
    InetAddress t1IP = t1.getLastValidIP();
    InetAddress t2IP = t2.getLastValidIP();
    
    Alias *toCheck = aliases->findAlias(t1IP);
    if(toCheck != NULL && toCheck->hasInterface(t2IP))
        return true;
    return false;
}
