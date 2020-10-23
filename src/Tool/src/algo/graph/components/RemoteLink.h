/*
 * RemoteLink.h
 *
 *  Created on: Dec 5, 2019
 *      Author: jefgrailet
 *
 * Models a remote link. A remote link models an edge connecting two neighborhoods that aren't 
 * direct peers (i.e., no intermediate hop between them). In other words, no intermediate 
 * neighborhood between them could be discovered during the measurements. As such, a remote link 
 * doesn't store a crossed subnet (unlike DirectLink and IndirectLink) and rather stores the 
 * offset, i.e., the amount of additional hops between the two neighborhoods/vertices, along a 
 * list of unique routes (as vectors of RouteHop objects) between both neighborhoods which have 
 * been observed in the measurements.
 *
 * A similar class already existed in SAGE v1 (originally written in November 2017) but was more 
 * complex, as it maintained pointers to a sub-graph of "miscellaneous" hops (observed via 
 * traceroute).
 */

#ifndef REMOTELINK_H_
#define REMOTELINK_H_

#include "Edge.h"
#include "../../structure/RouteHop.h"

class RemoteLink : public Edge
{
public:

    // Note: offset is inferred from the content of "routes".
    RemoteLink(Vertex *tail, Vertex *head, list<vector<RouteHop> > routes);
    ~RemoteLink();
    
    inline unsigned short getOffset() { return offset; }
    inline list<vector<RouteHop> > *getRoutes() { return &routes; }
    
    string toString(bool verbose);
    string routesToString(); // Unique to RemoteLink

protected:
    
    unsigned short offset;
    list<vector<RouteHop> > routes;
    
    /*
     * The "routes" list contains unique routes between tail and head, which are actually copies 
     * of the partial routes collected towards subnet interfaces by the PeerScanner class without 
     * the redundancies.
     */
    
};

#endif /* REMOTELINK_H_ */
