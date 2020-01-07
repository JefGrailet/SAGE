/*
 * Alias.h
 *
 *  Created on: Mar 1, 2015
 *      Author: jefgrailet
 *
 * A simple class to represent an alias. It simply consists in a list of aliased interfaces (as 
 * AliasedInterface objects).
 * 
 * In former tools (i.e., TreeNET and SAGE v1.0), it used to be named "Router".
 */

#ifndef ALIAS_H_
#define ALIAS_H_

#include "AliasedInterface.h"

class Alias
{
public:
    
    Alias();
    Alias(IPTableEntry *firstInterface);
    ~Alias();
    
    // Accessor to the list
    inline list<AliasedInterface> *getInterfacesList() { return &interfaces; }

    // Method to add one or several interfaces to this alias (with a same method)
    void addInterface(IPTableEntry *IP, unsigned short aliasMethod);
    void addInterfaces(list<IPTableEntry*> *IPs, unsigned short aliasMethod);
    
    // Method to get the amount of interfaces of this alias
    unsigned short getNbInterfaces();
    
    // Method to check a given IP is an interface of this alias
    bool hasInterface(InetAddress interface);
    
    /*
     * Method to get an interface fitting this criteria:
     * -aliased with the UDP-based method.
     * -has a "healthy" IP-ID counter.
     * Such an interface is a "merging pivot", i.e., the interface that will be used to merge an 
     * existing alias solely obtained via UDP with interfaces aliased through the Ally method.
     */
    
    IPTableEntry *getMergingPivot();
    
    // Converts the Alias object to an alias in string format
    string toString();
    
    // Similar methods, but with more aliasing details (i.e. with which method an IP was aliased)
    string toStringSemiVerbose();
    string toStringVerbose();
    
    // Third toString() method, this one displays up to 3 interfaces then "..." (+ # interfaces)
    string toStringMinimalist();
    
    // Methods to sort and compare aliases
    static bool compare(Alias *a1, Alias *a2);
    static bool compareBis(Alias *a1, Alias *a2);
    bool equals(Alias *other);
    
private:

    // Interfaces are stored with a list
    list<AliasedInterface> interfaces;
};

#endif /* ALIAS_H_ */
