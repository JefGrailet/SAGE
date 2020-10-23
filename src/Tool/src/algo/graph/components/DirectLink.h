/*
 * DirectLink.h
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Models a direct link. A direct link denotes an edge between two consecutive neighborhoods (or 
 * vertices), with the first of them containing a subnet encompassing a trail used to denote the 
 * second.
 *
 * A similar class already existed in SAGE v1 (originally written in November 2017).
 */

#ifndef DIRECTLINK_H_
#define DIRECTLINK_H_

#include "Edge.h"

class DirectLink : public Edge
{
public:

    DirectLink(Vertex *tail, Vertex *head, Subnet *medium);
    ~DirectLink();
    
    inline Subnet* getMedium() { return medium; }
    
    string toString(bool verbose = false);

protected:
    
    Subnet *medium;
    
};

#endif /* DIRECTLINK_H_ */
