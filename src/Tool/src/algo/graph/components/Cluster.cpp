/*
 * Cluster.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Cluster.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <map>
using std::map;

#include "Cluster.h"

Cluster::Cluster(list<Aggregate*> aggregates) : Vertice()
{
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *cur = (*i);
        
        // Gets the subnets
        list<Subnet*> *aggSubnets = cur->getSubnets();
        subnets.insert(subnets.end(), aggSubnets->begin(), aggSubnets->end());
        
        // Gets the trails
        list<Trail> *aggTrails = cur->getTrails();
        trails.insert(trails.end(), aggTrails->begin(), aggTrails->end());
    }
    
    subnets.sort(Subnet::compare);
    trails.sort(Trail::compare);
    
    // Removes possible duplicate trails (just in case)
    Trail prev = Trail();
    for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
    {
        Trail cur = (*i);
        if(cur == prev)
        {
            trails.erase(i--);
            continue;
        }
        prev = cur;
    }
}

Cluster::~Cluster()
{
}

void Cluster::addBlindspots(list<InetAddress> IPs)
{
    blindspots.insert(blindspots.end(), IPs.begin(), IPs.end());
}

void Cluster::addFlickeringAliases(AliasSet *aliases)
{
    // Creates a temp map to quickly check an IP is in the cluster and has an alias
    map<InetAddress, Alias*> mapAliases; 
    
    bool foundSomething = false;
    for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
    {
        InetAddress curIP = i->getLastValidIP();
        Alias *assocAlias = aliases->findAlias(curIP);
        if(assocAlias != NULL)
            foundSomething = true;
        mapAliases.insert(pair<InetAddress, Alias*>(curIP, assocAlias));
    }
    
    // No alias found at all: nothing to do here
    if(!foundSomething)
        return;
    
    // Otherwise, makes a census of all aliased IPs
    for(map<InetAddress, Alias*>::iterator i = mapAliases.begin(); i != mapAliases.end(); ++i)
    {
        Alias *assocAlias = i->second;
        if(assocAlias == NULL)
            continue;
        
        list<AliasedInterface> *IPs = assocAlias->getInterfacesList();
        for(list<AliasedInterface>::iterator j = IPs->begin(); j != IPs->end(); ++j)
            flickering.push_back((InetAddress) *(j->ip));
    }
    
    // Sorts and removes duplicates
    flickering.sort(InetAddress::smaller);
    InetAddress prev(0);
    for(list<InetAddress>::iterator i = flickering.begin(); i != flickering.end(); ++i)
    {
        InetAddress cur = (*i);
        if(cur == prev)
        {
            flickering.erase(i--);
            continue;
        }
        prev = cur;
    }
    
    // Finally, removes IPs that were in the temporary map (because already in the cluster)
    for(list<InetAddress>::iterator i = flickering.begin(); i != flickering.end(); ++i)
    {
        map<InetAddress, Alias*>::iterator inCluster = mapAliases.find((*i));
        if(inCluster != mapAliases.end())
            flickering.erase(i--);
    }
}

string Cluster::getFullLabel()
{
    stringstream label;
    
    if(ID == 0)
        label << "Cluster";
    else
        label << "N" << ID;
    label << " - {";
    
    // Trail IPs
    for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
    {
        if(i != trails.begin())
            label << ", ";
        label << i->getLastValidIP();
    }
    
    // Blindspots (if any)
    if(blindspots.size() > 0)
        for(list<InetAddress>::iterator i = blindspots.begin(); i != blindspots.end(); ++i)
            label << ", " << (*i) << " (B)";
    
    // Flickering IPs previously aliased during subnet inference
    if(flickering.size() > 0)
        for(list<InetAddress>::iterator i = flickering.begin(); i != flickering.end(); ++i)
            label << ", " << (*i) << " (F)";
    
    label << "}";
    if(ID != 0)
        label << " (cluster)";
    return label.str();
}

string Cluster::toString()
{
    stringstream ss; // stringstream included indirectly, via SubnetSite.h
    
    ss << this->getFullLabel() << ":\n";
    
    // Shows subnets
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        Subnet *cur = (*i);
        ss << cur->getAdjustedCIDR();
        
        // By design, Cluster vertices have subnets with differing trails: trails of each are shown
        list<InetAddress> cTrailIPs = cur->getDirectTrailIPs();
        if(cTrailIPs.size() > 1)
        {
            ss << " (trails: ";
            for(list<InetAddress>::iterator j = cTrailIPs.begin(); j != cTrailIPs.end(); ++j)
            {
                if(j != cTrailIPs.begin())
                    ss << ", ";
                ss << (*j);
            }
        }
        else
            ss << " (trail: " << cTrailIPs.front();
        ss << ")\n";
    }
    
    // Shows peer vertices (method inherited from parent class Vertice)
    ss << this->peersToString();

    return ss.str();
}
