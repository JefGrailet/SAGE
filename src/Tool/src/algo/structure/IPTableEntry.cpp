/*
 * IPTableEntry.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in IPTableEntry.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "IPTableEntry.h"

AliasHints IPTableEntry::VOID_HINTS = AliasHints();
Trail IPTableEntry::VOID_TRAIL = Trail();

IPTableEntry::IPTableEntry(InetAddress ip) : InetAddress(ip)
{
    TTL = NO_KNOWN_TTL;
    preferredTimeout = TimeVal(DEFAULT_TIMEOUT_SECONDS, TimeVal::HALF_A_SECOND);
    type = RESPONSIVE_TARGET;
    
    timeExceedediTTL = 0;
    
    trail = Trail(); // Empty, "void" trail
    
    trailIP = false;
    warping = false;
    echoing = false;
    flickering = false;
    
    denotingNeighborhood = false;
    blindspot = false;
}

IPTableEntry::~IPTableEntry()
{
    ARHints.clear();
}

/*** General methods ***/

bool IPTableEntry::hasTTL(unsigned char t)
{
    if(t == TTL)
        return true;
    for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); it++)
        if((*it) == t)
            return true;
    return false;
}

void IPTableEntry::recordTTL(unsigned char t)
{
    if(TTLs.size() == 0 && TTL != NO_KNOWN_TTL)
        TTLs.push_back(TTL);
        
    for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); it++)
        if((*it) == t)
            return;
    TTLs.push_back(t);
    TTLs.sort(compareTTLs);
}

void IPTableEntry::pickMinimumTTL()
{
    if(TTLs.size() > 0)
        TTL = TTLs.front();
}

bool IPTableEntry::compare(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1->getULongAddress() < ip2->getULongAddress())
        return true;
    return false;
}

bool IPTableEntry::compareWithTTL(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1->getTTL() < ip2->getTTL())
        return true;
    else if(ip1->getULongAddress() < ip2->getULongAddress())
        return true;
    return false;
}

bool IPTableEntry::compareLattestFingerprints(IPTableEntry *ip1, IPTableEntry *ip2)
{
    AliasHints &ah1 = ip1->getLattestHints();
    AliasHints &ah2 = ip2->getLattestHints();
    if(!ah1.isEmpty() && !ah2.isEmpty() && ah1.isLattestStage() && ah2.isLattestStage())
        return AliasHints::compare(ah1, ah2);
    else if(ah1.isEmpty() && !ah2.isEmpty())
        return true;
    return false;
}

string IPTableEntry::toString()
{
    stringstream ss;
    
    // Only seen at pre-scanning
    if(TTL == NO_KNOWN_TTL)
    {
        ss << (*this);
        if(type == UNSUCCESSFULLY_SCANNED)
            ss << " - Scan failure";
        return ss.str();
    }
    
    // IP - TTL
    ss << (*this) << " - " << (unsigned short) TTL;
    
    if(type == SUCCESSFULLY_SCANNED)
    {
        // Trail (if any)
        if(TTL > 1 && !trail.isVoid())
            ss << " " << trail;
        else if(TTL == 1)
            ss << " [This computer]";
        
        if(TTLs.size() > 1)
        {
            ss << " - Also observed at ";
            bool guardian = true;
            for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); ++it)
            {
                if((*it) == TTL)
                    continue;
                if(guardian)
                    guardian = false;
                else
                    ss << ", ";
                ss << (unsigned short) (*it);
            }
        }
    }
    // Only seen in a trail/with traceroute
    else if(type == SEEN_IN_TRAIL || type == SEEN_WITH_TRACEROUTE)
    {
        if(type == SEEN_IN_TRAIL)
            ss << " - Part of a trail";
        else
            ss << " - Part of a route";
        if(TTLs.size() > 1)
        {
            ss << ", also observed at ";
            bool guardian = true;
            for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); ++it)
            {
                if((*it) == TTL)
                    continue;
                if(guardian)
                    guardian = false;
                else
                    ss << ", ";
                ss << (unsigned short) (*it);
            }
        }
    }
    // Another scan failure (trail not available)
    else
        ss << " - Scan failure";
    
    // Special IP: part of trail + has a unique behaviour
    if(trailIP && (warping || echoing || flickering))
    {
        ss << " | ";
        if(warping)
        {
            ss << "Warping";
            if(echoing || flickering)
                ss << " + ";
        }
        if(echoing)
        {
            ss << "Echoing";
            if(flickering)
                ss << " + ";
        }
        if(flickering)
        {
            ss << "Flickering with ";
            list<InetAddress>::iterator i;
            for(i = flickeringPeers.begin(); i != flickeringPeers.end(); ++i)
            {
                if(i != flickeringPeers.begin())
                    ss << ", ";
                ss << (*i);
            }
        }
    }
    
    if(blindspot)
        ss << " || Blindspot (initially unidentified peer)";
    
    /*
     * Remark (October 2019): (partial) route details and inferred "Time exceeded" initial TTL are 
     * not given in the text output of the IP dictionary, as these details are used to infer data 
     * that is either already present (e.g. the trail) or which will be shown in other output 
     * (e.g. fingerprints for alias resolution).
     */
        
    return ss.str();
}

string IPTableEntry::toStringSimple()
{
    return (*this).getHumanReadableRepresentation();
}

/*** Route and trail ***/

void IPTableEntry::initRoute()
{
    if(TTL == NO_KNOWN_TTL)
        return;
    
    // Clears previous route (if any)
    if(route.size() > 0)
        route.clear();
    
    if(TTL > 1)
        for(unsigned short i = 0; i < ((unsigned short) TTL) - 1; i++)
            route.push_back(RouteHop());
}

bool IPTableEntry::anonymousEndOfRoute()
{
    if(route.size() == 0)
        return false;
    return (route[route.size() - 1].ip == InetAddress(0));
}

bool IPTableEntry::setTrail()
{
    if(TTL == 1)
        return true; // No trail needed

    if(route.size() == 0)
        return false;
    
    // Moves backward in the route to find measured, non-anonymous hops
    unsigned short trueRouteLen = ((unsigned short) TTL) - 1; // Because can be adjusted during re-probing
    InetAddress lastValidIP(0);
    unsigned short lastValidIndex = 0;
    for(short i = trueRouteLen - 1; i >= 0; i--)
    {
        if(route[i].isValidHop())
        {
            lastValidIP = route[i].ip;
            lastValidIndex = i;
            break;
        }
    }
    
    if(lastValidIP == InetAddress(0))
    {
        // Exceptional situation where the route only contains anonymous hops
        if(route[0].isAnonymous())
        {
            trail = Trail(trueRouteLen);
            return true;
        }
        return false;
    }
    
    // Checks if there's a (consecutive) cycle, then computes the amount of anomalies
    short j = lastValidIndex - 1;
    while(j > 0 && route[j].ip == lastValidIP)
        j--;
    unsigned short nbAnomalies = trueRouteLen - 2 - j;
    
    /*
     * Additional condition for setting a trail: route[j] must be set (even as an anonymous hop) 
     * or j must be negative, otherwise we're not 100% there's no cycle we're missing.
     */
    
    if(j >= 0 && route[j].isUnset())
        return false;
    
    if(nbAnomalies > 0)
        trail = Trail(route[lastValidIndex], nbAnomalies);
    else
        trail = Trail(route[lastValidIndex]);
    
    return true;
}

bool IPTableEntry::hasCompleteRoute()
{
    if(route.size() == 0)
    {
        if(TTL == 1)
            return true;
        else
            return false;
    }
    return !route[0].isUnset();
}

string IPTableEntry::routeToString()
{
    if(route.size() == 0)
    {
        if(TTL == 1)
            return "No route, the IP is one hop away from the vantage point.";
        else
            return "No route.";
    }

    stringstream ss;
    for(unsigned short i = 0; i < route.size(); i++)
    {
        if(route[i].isUnset())
            continue;
        ss << (i + 1) << " - " << route[i] << "\n";
    }
    return ss.str();
}

/*** Special IPs ***/

void IPTableEntry::addFlickeringPeer(InetAddress peer)
{
    // Checks if peer isn't already recorded
    for(list<InetAddress>::iterator i = flickeringPeers.begin(); i != flickeringPeers.end(); ++i)
        if((*i) == peer)
            return;
    
    // Records it and sorts the list
    flickeringPeers.push_back(peer);
    flickeringPeers.sort(InetAddress::smaller);
}

/*** Alias resolution hints ***/

AliasHints& IPTableEntry::getLattestHints()
{
    if(ARHints.size() == 0)
        return VOID_HINTS; // Empty hints
    return ARHints.back();
}

string IPTableEntry::hintsToString()
{
    stringstream ss;
    for(list<AliasHints>::iterator it = ARHints.begin(); it != ARHints.end(); ++it)
        ss << (*this) << " - " << it->toString() << "\n";
    return ss.str();
}
