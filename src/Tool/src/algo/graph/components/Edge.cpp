/*
 * Edge.cpp
 *
 *  Created on: Dec 2, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Edge.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Edge.h"

Edge::Edge(Vertex *tail, Vertex *head)
{
    this->tail = tail;
    this->head = head;
}

Edge::~Edge()
{
    // Pointed objects will be destroyed elsewhere
}

bool Edge::compare(Edge *e1, Edge *e2)
{
    if(e1->getTail()->getID() == e2->getTail()->getID())
        return Vertex::smallerID(e1->getHead(), e2->getHead());
    return Vertex::smallerID(e1->getTail(), e2->getTail());
}
