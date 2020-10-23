/*
 * Mariner.cpp
 *
 *  Created on: Dec 10, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Mariner.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "Mariner.h"
#include "../components/RemoteLink.h"

Mariner::Mariner(Environment &env) : Voyager(env)
{
}

Mariner::~Mariner()
{
}

void Mariner::visit(Graph *g)
{
    // Fills the "visited" array
    for(unsigned int i = 0; i < g->getNbVertices(); ++i)
        visited.push_back(false);
    
    // Visits the graph
    list<Vertex*> *gates = g->getGates();
    for(list<Vertex*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
    vertices.sort(Vertex::smallerID);
    
    // Clears "visited" in case another graph needed to be explored with the same object
    visited.clear();
}

void Mariner::visitRecursive(Vertex *v)
{
    unsigned int ID = v->getID();
    if(visited[ID - 1])
        return;
    
    vertices.push_back(v);
    visited[ID - 1] = true;
    
    list<Edge*> *next = v->getEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}

void Mariner::outputNeighborhoods(string filename)
{
    string output = "";
    for(list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
        output += (*i)->toString() + "\n";
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void Mariner::outputGraph(string filename)
{
    stringstream ss;
    
    // First, list of neighborhoods (as "full" labels, i.e., mapping ID -> label(s))
    for(list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
    {
        Vertex *v = (*i);
        ss << v->getFullLabel();
        
        /*
         * For the sake of completeness, Mariner also adds whether the peer IP(s) appearing in the 
         * trail(s) of a vertex (Node or Cluster) was/were in the initial targets of the program. 
         * This is interesting in the sense that it could later highlight how the target network 
         * connects with others.
         */
        
        list<Trail> *labels = v->getTrails();
        stringstream subStream;
        bool guardian = false;
        for(list<Trail>::iterator j = labels->begin(); j != labels->end(); ++j)
        {
            InetAddress curLabel = j->getLastValidIP();
            if(!env.initialTargetsEncompass(curLabel))
            {
                if(!guardian)
                    guardian = true;
                else
                    subStream << ", ";
                subStream << curLabel;
            }
        }
        string result = subStream.str();
        if(result.length() > 0)
            ss << " (" << result << " not among targets)";
        
        ss << "\n";
    }
    ss << "\n";
    
    // Second, the edges tying vertices/neighborhoods together ("exit" direction only)
    list<RemoteLink*> remoteLinks;
    for(list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
    {
        Vertex *v = (*i);
        list<Edge*> *edges = v->getEdges();
        
        if(edges->size() == 0)
            continue;
        
        for(list<Edge*>::iterator j = edges->begin(); j != edges->end(); ++j)
        {
            Edge *curEdge = (*j);
            if(RemoteLink *remote = dynamic_cast<RemoteLink*>(curEdge))
                remoteLinks.push_back(remote);
            ss << curEdge->toString() << "\n";
        }
    }
    
    /*
     * Finally, the unique routes observed between remote peers, symbolized in the graph by 
     * "RemoteLink" edges, are displayed at the very end of the output. Displaying these routes is 
     * meant to provide a graph as complete as possible without going as far as creating a 
     * sub-graph based on traceroute (risky nowadays), as done by SAGE v1. On a side note, such 
     * routes could be parsed with scripts (e.g. in Python) to build a sub-graph anyway, if user 
     * is confident with the traceroute data.
     */
    
    if(remoteLinks.size() > 0)
    {
        ss << "\n";
        remoteLinks.sort(Edge::compare);
        for(list<RemoteLink*>::iterator i = remoteLinks.begin(); i != remoteLinks.end(); ++i)
            ss << (*i)->routesToString();
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << ss.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void Mariner::cleanVertices()
{
    for(list<Vertex*>::iterator i = vertices.begin(); i != vertices.end(); ++i)
        delete (*i);
    vertices.clear();
}
