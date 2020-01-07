/*
 * IndirectLink.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in IndirectLink.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "IndirectLink.h"

IndirectLink::IndirectLink(Vertice *tail, 
                           Vertice *head, 
                           SubnetVerticeMapping &med) : Edge(tail, head), medium(med)
{
}

IndirectLink::IndirectLink(Vertice *tail, Vertice *head) : Edge(tail, head)
{
    medium = SubnetVerticeMapping(); // Empty mapping
}

IndirectLink::~IndirectLink()
{
}

string IndirectLink::toString(bool verbose)
{
    stringstream ss;
    
    if(verbose || tail->getID() == 0)
        ss << tail->getFullLabel() << " to " << head->getFullLabel();
    else
        ss << "N" << tail->getID() << " -> N" << head->getID();
    if(!medium.isEmpty())
    {
        ss << " via " << medium.subnet->getCIDR();
        ss << " (from N" << medium.vertice->getID() << ")";
    }
    else
        ss << " (unknown medium)";
    
    return ss.str();
}
