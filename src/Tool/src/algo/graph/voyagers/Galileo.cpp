/*
 * Galileo.cpp
 *
 *  Created on: Dec 19, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in Galileo.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "../../../common/thread/Thread.h" // For invokeSleep()
#include "Galileo.h"

Galileo::Galileo(Environment &env) : Voyager(env), ahc(env), ar(env)
{
}

Galileo::~Galileo()
{
}

void Galileo::visit(Graph *g)
{
    // Clears previous list of vertices with aliases, in case we visited another graph
    if(withAliases.size() > 0)
        withAliases.clear();

    // Fills the "visited" array
    for(unsigned int i = 0; i < g->getNbVertices(); ++i)
        visited.push_back(false);
    
    // Visits the graph
    list<Vertex*> *gates = g->getGates();
    for(list<Vertex*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
    withAliases.sort(Vertex::smallerID);
    
    // Clears "visited" in case another graph needed to be explored with the same object
    visited.clear();
}

void Galileo::visitRecursive(Vertex *v)
{
    /*
     * Checks if we already evaluated this vertex. If we did, we stop here. Checking the list of 
     * aliases of the vertex is empty is not enough, because there can be vertices with no alias 
     * at all, which would therefore be re-evaluated for nothing.
     */
    
    unsigned int ID = v->getID();
    if(visited[ID - 1])
        return;
    visited[ID - 1] = true;
    
    /*
     * Otherwise, we check that we have some valid alias candidates and perform said alias 
     * resolution on them before going further in the graph.
     */
    
    list<IPTableEntry*> aliasCandidates = this->getAliasCandidates(v);
    if(aliasCandidates.size() > 0)
    {
        ostream *out = env.getOutputStream();
        
        (*out) << "Conducting alias resolution on " << v->getFullLabel();
        (*out) << "... " << std::flush;
        
        if(ahc.isPrintingSteps())
        {
            (*out) << endl;
            if(ahc.debugMode()) // Additionnal line break for harmonious display
                (*out) << endl;
        }
    
        ahc.setIPsToProbe(aliasCandidates);
        ahc.collect();
        
        list<Alias*> results = ar.resolve(aliasCandidates);
        if(results.size() > 0)
        {
            v->storeAliases(results);
            withAliases.push_back(v);
        }
        
        // Small delay before analyzing next vertex (typically quarter of a second)
        Thread::invokeSleep(env.getProbingThreadDelay());
    }
    
    list<Edge*> *next = v->getEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}

list<IPTableEntry*> Galileo::getAliasCandidates(Vertex *v)
{
    IPLookUpTable *dict = env.getIPDictionary();

    list<IPTableEntry*> fullList = v->getAliasCandidates();
    list<Trail> *trails = v->getTrails();
    
    // Gets IP entries linked to the trails
    for(list<Trail>::iterator i = trails->begin(); i != trails->end(); ++i)
    {
        if(i->getNbAnomalies() > 0)
            continue;
        InetAddress curIP = i->getLastValidIP();
        IPTableEntry *curEntry = dict->lookUp(curIP);
        if(curEntry == NULL)
            continue;
        fullList.push_back(curEntry);
    }
    
    // Sorts and removes potential duplicates (there can be trails that are also contra-pivots)
    fullList.sort(IPTableEntry::compare);
    IPTableEntry *prev = NULL;
    for(list<IPTableEntry*>::iterator i = fullList.begin(); i != fullList.end(); ++i)
    {
        IPTableEntry *cur = (*i);
        if(cur == prev)
            fullList.erase(i--);
        prev = cur;
    }
    
    return fullList;
}

void Galileo::figaro(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    
    for(list<Vertex*>::iterator i = withAliases.begin(); i != withAliases.end(); ++i)
    {
        Vertex *cur = (*i);
        list<Alias*> *aliases = cur->getAliases();
        newFile << cur->getFullLabel() << ":\n";
        for(list<Alias*>::iterator j = aliases->begin(); j != aliases->end(); ++j)
            newFile << (*j)->toStringSemiVerbose() << "\n";
        newFile << endl;
    }
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
