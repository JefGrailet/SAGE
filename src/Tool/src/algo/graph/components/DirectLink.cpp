/*
 * DirectLink.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in DirectLink.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "DirectLink.h"

DirectLink::DirectLink(Vertice *tail, Vertice *head, Subnet *medium) : Edge(tail, head)
{
    this->medium = medium;
}

DirectLink::~DirectLink()
{
}

string DirectLink::toString(bool verbose)
{
    stringstream ss;
    
    if(verbose || tail->getID() == 0)
        ss << tail->getFullLabel() << " to " << head->getFullLabel();
    else
        ss << "N" << tail->getID() << " -> N" << head->getID();
    ss << " via " << medium->getCIDR();
    
    return ss.str();
}
