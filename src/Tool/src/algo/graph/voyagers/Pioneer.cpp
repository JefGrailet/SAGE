/*
 * Pioneer.cpp
 *
 *  Created on: Dec 9, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Pioneer.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Pioneer.h"

Pioneer::Pioneer(Environment &env) : Voyager(env)
{
    counter = 1; // First vertice will be "N1"
}

Pioneer::~Pioneer()
{
}

void Pioneer::visit(Graph *g)
{
    list<Vertice*> *gates = g->getGates();
    for(list<Vertice*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
    
    g->setNbVertices(counter - 1);
}

void Pioneer::visitRecursive(Vertice *v)
{
    if(v->getID() != 0)
        return;

    v->setID(counter);
    counter++;
    
    list<Edge*> *next = v->getEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}
