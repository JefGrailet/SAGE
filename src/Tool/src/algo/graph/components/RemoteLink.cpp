/*
 * RemoteLink.cpp
 *
 *  Created on: Dec 5, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in RemoteLink.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "RemoteLink.h"

RemoteLink::RemoteLink(Vertex *tail, 
                       Vertex *head, 
                       list<vector<RouteHop> > r) : Edge(tail, head), routes(r)
{
    if(routes.size() > 0)
        offset = routes.front().size() - 1; // Routes have one hop more for the peer IP
    else
        offset = 0; // Just a "security", shouldn't occur by design
}

RemoteLink::~RemoteLink()
{
}

string RemoteLink::toString(bool verbose)
{
    stringstream ss;
    
    if(verbose || tail->getID() == 0)
        ss << tail->getFullLabel() << " to " << head->getFullLabel();
    else
        ss << "N" << tail->getID() << " -> N" << head->getID();
    ss << " through ";
    if(offset > 1)
        ss << offset << " intermediate hops";
    else if(offset == 1)
        ss << "one intermediate hop";
    else
        ss << "an unknown amount of intermediate hops"; // Just in case
    
    return ss.str();
}

string RemoteLink::routesToString()
{
    if(routes.size() == 0) // Just in case
        return "";

    stringstream ss, headerStream;
    
    if(tail->getID() == 0)
        headerStream << tail->getFullLabel() << " to " << head->getFullLabel();
    else
        headerStream << "N" << tail->getID() << " -> N" << head->getID();
    string header = headerStream.str();
    
    unsigned short routeNb = 1;
    for(list<vector<RouteHop> >::iterator i = routes.begin(); i != routes.end(); ++i)
    {
        vector<RouteHop> curRoute = (*i);
        ss << header;
        if(routes.size() > 1) // Displays " (X)" when there are multiple routes
            ss << " (" << routeNb << ")";
        ss << ": ";
        for(unsigned short j = 1; j < curRoute.size(); j++) // +1 because first hop is a peer IP
        {
            if(j > 1)
                ss << ", ";
            ss << curRoute[j]; 
        }
        ss << "\n";
        routeNb++;
    }
    
    return ss.str();
}
