/*
 * AliasSet.h
 *
 *  Created on: Nov 26, 2018
 *      Author: jefgrailet
 *
 * A simple class to model a set of aliases. It encapsulates two C/C++ data structures: a list of 
 * all aliases and a map that associates each IP mentioned in all aliases to one alias, the 
 * intention being that one should be able to quickly look-up which alias an IP belongs to in 
 * various algorithmic steps.
 *
 * Another reason why this class exists is because there will be several aliases set with the 
 * upcoming SAGE versions, just like there will be several alias hints for some specific IPs. 
 * Maintaining disjoint sets is a way to keep track of how alias resolution was used during the 
 * complete execution of the tool.
 */

#ifndef ALIASSET_H_
#define ALIASSET_H_

#include <map>
using std::map;
using std::pair;

#include "Alias.h"

class AliasSet
{
public:
    
    // Constants to model probing stages
    const static unsigned short SUBNET_DISCOVERY = 1;
    const static unsigned short GRAPH_BUILDING = 2; // For SAGE v2.0
    const static unsigned short FULL_ALIAS_RESOLUTION = 3; // For SAGE v2.0

    AliasSet(unsigned short when);
    ~AliasSet();
    
    inline unsigned short getWhen() { return when; }
    
    // Accessor to the list of aliases
    inline list<Alias*> *getAliasesList() { return &aliases; }
    inline size_t getNbAliases() { return aliases.size(); }

    // Method to add a new alias to the set
    void addAlias(Alias *newAlias);
    
    // Adds several aliases; the bool should be set to true to remove Alias objects made of one IP
    void addAliases(list<Alias*> newAliases, bool purgeSingleIPs = false);
    
    // Finds the alias encompassing a given IP (if any)
    Alias *findAlias(InetAddress interface);
    
    // Outputs the whole set
    void output(string filename, bool verbose = false);
    
private:

    // Short value to maintain at which algorithmic step this alias set was built
    unsigned short when;

    // Data structures as mentioned in the header of this file
    list<Alias*> aliases;
    map<InetAddress, Alias*> IPsToAliases;

};

#endif /* ALIASSET_H_ */
