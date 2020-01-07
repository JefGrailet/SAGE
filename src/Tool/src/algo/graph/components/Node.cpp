/*
 * Node.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Node.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Node.h"

Node::Node(Aggregate *a) : Vertice()
{
    list<Subnet*> *aggSubnets = a->getSubnets();
    subnets.insert(subnets.end(), aggSubnets->begin(), aggSubnets->end());
    
    list<Trail> *aggTrails = a->getTrails();
    if(aggTrails->size() == 0)
    {
        list<InetAddress> *aggPreEchoing = a->getPreEchoing();
        if(aggPreEchoing->size() > 0) // No pre-echoing ~= "{This computer}"
        {
            preEchoingOffset = a->getPreEchoingOffset();
            preEchoing.insert(preEchoing.end(), aggPreEchoing->begin(), aggPreEchoing->end());
        }
    }
    else
        trails.insert(trails.end(), aggTrails->begin(), aggTrails->end());
}

Node::~Node()
{
}

string Node::getFullLabel()
{
    stringstream label;
    
    if(ID == 0)
        label << "Node - ";
    else
        label << "N" << ID << " - ";
    label << "{";
    
    if(trails.size() == 0 && subnets.front()->getPivotTTL() <= 2)
    {
        label << "This computer}";
        return label.str();
    }
    
    if(trails.size() > 1) // Flickering trails
    {
        for(list<Trail>::iterator i = trails.begin(); i != trails.end(); ++i)
        {
            if(i != trails.begin())
                label << ", ";
            label << i->getLastValidIP();
        }
    }
    else if(trails.size() == 0) // Echoing trails (best effort strategy used instead)
    {
        label << "Echo, TTL=" << (unsigned short) subnets.front()->getPivotTTL();
        label << ", Pre-echoing=";
        for(list<InetAddress>::iterator i = preEchoing.begin(); i != preEchoing.end(); ++i)
        {
            if(i != preEchoing.begin())
                label << ", ";
            label << (*i);
        }
        if(preEchoingOffset > 0)
            label << " (offset=" << preEchoingOffset << ")";
    }
    else // Any other case (direct or incomplete trail common to all aggregated subnets)
    {
        Trail &curTrail = trails.front();
        label << curTrail.getLastValidIP();
        if(curTrail.getNbAnomalies() > 0)
            label << " | " << curTrail.getNbAnomalies();
    }
    
    label << "}";
    
    return label.str();
}

string Node::toString()
{
    stringstream ss; // stringstream included indirectly
    
    ss << this->getFullLabel() << ":\n";
    
    // Shows subnets
    for(list<Subnet*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        Subnet *cur = (*i);
        ss << cur->getAdjustedCIDR();
        
        // Aggregated subnets have flickering trails: show those present in each
        if(trails.size() > 1)
        {
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
            ss << ")";
        }
        // Subnets were aggregated on the basis of 2+ pre-echoing IPs: show those seen with each
        else if(trails.size() == 0 && preEchoing.size() > 1)
        {
            ss << " (pre-echoing: ";
            list<InetAddress> preEchoes = cur->getPreTrailIPs();
            for(list<InetAddress>::iterator j = preEchoes.begin(); j != preEchoes.end(); ++j)
            {
                if(j != preEchoes.begin())
                    ss << ", ";
                ss << (*j);
            }
            ss << ")";
        }
        
        ss << "\n";
    }
    
    // Shows peer vertices (method inherited from parent class Vertice)
    ss << this->peersToString();

    return ss.str();
}
