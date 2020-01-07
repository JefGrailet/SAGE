/*
 * AliasResolver.h
 *
 *  Created on: Oct 20, 2015
 *      Author: jefgrailet
 *
 * This class, originating from TreeNET (hence why it was first created in late 2015), is 
 * responsible for actually performing alias resolution on given sets of IPs for which hints have 
 * been previously collected via the AliasHintsCollector class. The class evolved over time, 
 * progressively incorporating more state-of-the-art techniques while being slightly simplified.
 */

#ifndef ALIASRESOLVER_H_
#define ALIASRESOLVER_H_

#include "../Environment.h"
#include "../structure/Alias.h"
#include "../structure/AliasHints.h"

class AliasResolver
{
public:

    // Possible results for Ally method
    enum AllyResults
    {
        ALLY_NO_SEQUENCE, // A token sequence could not be found
        ALLY_REJECTED, // The sequence exists, but Ally rejects it
        ALLY_ACCEPTED // The sequences exists and Ally acknowledges the association
    };

    // Constructor/destructor
    AliasResolver(Environment &env);
    ~AliasResolver();
    
    /*
     * Resolves a list of IPTableEntry objects. AliasHints objects are retrieved via one method 
     * from the IPTableEntry class.
     *
     * @param list<IPTableEntry*> IPs  The IPs on which alias resolution must be conducted
     * @param bool strict              To set to true if we want to only use IP-ID-based and 
     *                                 UDP-based methods, i.e., strict mode (default: false)
     * @return list<Alias*>            The obtained aliases
     */
    
    list<Alias*> resolve(list<IPTableEntry*> IPs, bool strict = false);
    
private:

    // Pointer to the environment object (grants access to some parameters and IP dictionary)
    Environment &env;
    
    /*
     * Method to perform Ally method for alias resolution, i.e., if for 2 distinct IPs, we have
     * 3 IP-IDs with increasing tokens which are also increasing themselves and close in values, 
     * 2 of them being from the first IP and the last one from the second, they are likely from 
     * the same router.
     *
     * @param IPTableEntry* ip1       The first IP of the alleged alias pair
     * @param IPTableEntry* ip2       The second IP of the alleged alias pair
     * @param unsigned short maxDiff  The largest gap to be accepted between 2 consecutive IDs
     * @return unsigned short         One of the results listed in "enum AllyResults"
     */
    
    unsigned short Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff);
    
    /* 
     * Method to perform Ally method between each member of a group of aliased IPs and a given, 
     * isolated IP. The purpose of this method is to check that all IPs grouped or a growing 
     * alias are compatible with the additionnal IP (there must be no possible rejection by Ally).
     * 
     * @param IPTableEntry* isolatedIP    The isolated IP
     * @param list<IPTableEntry*> group   The group of already aliased IPs
     * @param unsigned short maxDiff      The largest accepted gap (just like for Ally())
     * @return unsigned short             One of the results listed in "enum AllyResults"
     */
    
    unsigned short groupAlly(IPTableEntry *isolatedIP, 
                             list<IPTableEntry*> group, 
                             unsigned short maxDiff);
    
    /*
     * Method to check if the velocity ranges of two distinct IPs (given as IPTableEntry objects) 
     * overlap, if which case both IPs should be aliased together.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if both ranges overlap (if they exist), false otherwise
     */
    
    bool velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2);
    
    /* 
     * Just like groupAlly(), next method checks an isolated IP is compatible with a growing alias 
     * of IPs associated with the velocity-based approach. The purpose is to check that all 
     * IPs grouped in the growing alias are compatible with the additional IP velocity-wise.
     * 
     * @param Fingerprint isolatedIP    The isolated IP
     * @param list<Fingerprint> group   The group of already aliased IPs
     * @return unsigned short           True if the isolated IP is compatible
     */
    
    bool groupVelocity(IPTableEntry *isolatedIP, list<IPTableEntry*> group);
    
    /*
     * Method to check if two distinct IPs (given as IPTableEntry objects) can be associated 
     * with the reverse DNS method.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if association is possible, false otherwise
     */
    
    bool reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2);
    
    /*
     * Method to apply alias resolution to a group of IPs, modeled by a list of IPs provided along 
     * lists of Alias objects. This lists will contain the resulting Alias objects.
     *
     * A fourth optional parameter (set by default to false) can be set to true to enable "strict" 
     * mode. In this mode, discover() behaves the same way but skips some alias resolution methods 
     * that are considered too optimistic (such as group by fingerprint, unless DNS does not match 
     * at all). It also avoids creating Router objects consisting of a single interface.
     *
     * @param list<IPTableEntry*>  IPs      The IPs to alias
     * @param list<Alias*>*        results  The list that will contain resulting Alias objects
     * @param bool                 strict   To set to true if we want to only use IP-ID-based and 
     *                                      UDP-based methods, i.e., strict mode
     */
    
    void discover(list<IPTableEntry*> interfaces, list<Alias*> *results, bool strict = false);

};

#endif /* ALIASRESOLVER_H_ */
