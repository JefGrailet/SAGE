/*
 * Peer.cpp
 *
 *  Created on: Nov 20, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Peer.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Peer.h"

Peer::Peer(InetAddress peerIP)
{
    interfaces.push_back(peerIP);
}

Peer::Peer(Alias *alias)
{
    list<AliasedInterface> *IPs = alias->getInterfacesList();
    for(list<AliasedInterface>::iterator it = IPs->begin(); it != IPs->end(); ++it)
        interfaces.push_back((InetAddress) *(it->ip));
    interfaces.sort(InetAddress::smaller);
}

Peer::~Peer()
{
}

bool Peer::contains(InetAddress interface)
{
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if((*i) == interface)
            return true;
    return false;
}

bool Peer::compare(Peer *p1, Peer *p2)
{
    list<InetAddress> *IPs1 = p1->getInterfaces();
    list<InetAddress> *IPs2 = p2->getInterfaces();
    if(IPs1->front() < IPs2->front())
        return true;
    return false;
}

string Peer::toString()
{
    stringstream ss; // stringstream included indirectly via Alias.h
    ss << "[";
    for(list<InetAddress>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            ss << ", ";
        ss << (*it);
    }
    ss << "]";
    return ss.str();
}
