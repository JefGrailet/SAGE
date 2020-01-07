/*
 * AliasSet.cpp
 *
 *  Created on: Nov 26, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in AliasSet.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
using std::ofstream;
#include <sys/stat.h> // For CHMOD edition

#include "AliasSet.h"

AliasSet::AliasSet(unsigned short when)
{
    this->when = when;
}

AliasSet::~AliasSet()
{
    for(list<Alias*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
        delete (*i);
}

void AliasSet::addAlias(Alias *newAlias)
{
    // Adds the new alias to the list
    aliases.push_back(newAlias);
    aliases.sort(Alias::compare);
    
    // Adds the new alias to the map
    list<AliasedInterface> *interfaces = newAlias->getInterfacesList();
    for(list<AliasedInterface>::iterator i = interfaces->begin(); i != interfaces->end(); ++i)
    {
        InetAddress needle = (InetAddress) *(i->ip);
        IPsToAliases.insert(pair<InetAddress, Alias*>(needle, newAlias));
        
        /*
         * N.B.: there is a minor risk of collision (e.g., if two aliases happen to share a same 
         * IP). If this happens, the default behaviour of a C/C++ map is to keep the first 
         * inserted pair. Keeping track of collisions isn't vital for now, so this default 
         * behaviour is kept.
         */
    }
}

void AliasSet::addAliases(list<Alias*> newAliases, bool purgeSingleIPs)
{
    while(newAliases.size() > 0)
    {
        Alias *newAlias = newAliases.front();
        newAliases.pop_front();
        if(purgeSingleIPs && newAlias->getNbInterfaces() < 2)
        {
            delete newAlias;
            continue;
        }
        aliases.push_back(newAlias);
        
        // Adds the new alias to the map in the same manner as in addAlias()
        list<AliasedInterface> *interfaces = newAlias->getInterfacesList();
        for(list<AliasedInterface>::iterator i = interfaces->begin(); i != interfaces->end(); ++i)
        {
            InetAddress needle = (InetAddress) (*(i->ip));
            IPsToAliases.insert(pair<InetAddress, Alias*>(needle, newAlias));
        }
    }
    aliases.sort(Alias::compare);
}

Alias* AliasSet::findAlias(InetAddress interface)
{
    map<InetAddress, Alias*>::iterator res = IPsToAliases.find(interface);
    if(res != IPsToAliases.end())
        return res->second;
    return NULL;
}

void AliasSet::output(string filename, bool verbose)
{
    if(aliases.size() == 0)
        return;

    string output = "";
    for(list<Alias*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
    {
        Alias *curAlias = (*i);
        if(verbose)
            output += curAlias->toStringVerbose() + "\n";
        else
            output += curAlias->toStringSemiVerbose() + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
