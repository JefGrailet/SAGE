/*
 * GraphPeers.cpp
 *
 *  Created on: Nov 28, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in GraphPeers.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "GraphPeers.h"

GraphPeers::GraphPeers(Aggregate *aggregate)
{
    list<Peer*> *fPeers = aggregate->getFinalPeers();
    peers.insert(peers.end(), fPeers->begin(), fPeers->end());
    offset = aggregate->getPeersOffset();
}

GraphPeers::GraphPeers(list<Aggregate*> aggregates)
{
    // Finds smallest peers offset
    offset = 255;
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        unsigned short curOffset = (*i)->getPeersOffset();
        if(curOffset < offset)
            offset = curOffset;
        if(curOffset == 0)
            break;
    }
    
    // Lists peers at that offset
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *cur = (*i);
        if(cur->getPeersOffset() != offset)
            continue;
        
        list<Peer*> *fPeers = cur->getFinalPeers();
        peers.insert(peers.end(), fPeers->begin(), fPeers->end());
    }
    
    // Sorts and remove duplicate peers
    peers.sort(Peer::compare);
    Peer *prev = NULL;
    for(list<Peer*>::iterator i = peers.begin(); i != peers.end(); ++i)
    {
        Peer *cur = (*i);
        if(cur == prev)
        {
            peers.erase(i--);
            continue;
        }
        prev = cur;
    }
}

GraphPeers::~GraphPeers()
{
    // Nothing deleted here; every other pointed object is destroyed elsewhere
}
