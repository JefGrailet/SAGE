/*
 * IPLookUpTable.cpp
 *
 *  Created on: Sep 29, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in IPLookUpTable.h (see this file to learn further about the goals 
 * of such a class).
 */
 
#include <fstream>
using std::ofstream;
#include <sys/stat.h> // For CHMOD edition

#include "IPLookUpTable.h"

IPLookUpTable::IPLookUpTable()
{
    haystack = new list<IPTableEntry*>[SIZE_TABLE];
}

/*
 * General remark: in all methods, when a specific list of the dictionary is selected, typically 
 * named "IPList", a reference is used because "IPList" otherwise denotes a copy of the list 
 * rather than the stored list. This prevents the recording of a new entry in the case of the 
 * create() method, and other cases, an unnecessary copy is made.
 */

IPLookUpTable::~IPLookUpTable()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
            delete (*j);
        haystack[i].clear();
    }
    delete[] haystack;
}

bool IPLookUpTable::isEmpty()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        if(IPList.size() > 0)
            return true;
    }
    return false;
}

bool IPLookUpTable::hasAliasResolutionData()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        if(IPList.size() > 0)
            for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
                if((*j)->getNbHints() > 0)
                    return true;
    }
    return false;
}

unsigned int IPLookUpTable::getTotalIPs()
{
    unsigned int total = 0;
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        total += IPList.size();
    }
    return total;
}

IPTableEntry *IPLookUpTable::create(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<IPTableEntry*> &IPList = haystack[index];
    
    for(list<IPTableEntry*>::iterator i = IPList.begin(); i != IPList.end(); ++i)
        if((*i)->getULongAddress() == needle.getULongAddress())
            return NULL;
    
    IPTableEntry *newEntry = new IPTableEntry(needle);
    IPList.push_back(newEntry);
    IPList.sort(IPTableEntry::compare);
    return newEntry;
}

IPTableEntry *IPLookUpTable::lookUp(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<IPTableEntry*> &IPList = haystack[index];
    
    for(list<IPTableEntry*>::iterator i = IPList.begin(); i != IPList.end(); ++i)
        if((*i)->getULongAddress() == needle.getULongAddress())
            return (*i);
    
    return NULL;
}

list<IPTableEntry*> IPLookUpTable::listTargetEntries()
{
    list<IPTableEntry*> res;
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            unsigned short type = cur->getType();
            if(type == IPTableEntry::SEEN_WITH_TRACEROUTE || type == IPTableEntry::SEEN_IN_TRAIL)
                continue;
            res.push_back(cur);
        }
    }
    return res;
}

list<IPTableEntry*> IPLookUpTable::listFlickeringIPs()
{
    list<IPTableEntry*> res;
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            if(cur->isFlickering())
                res.push_back(cur);
        }
    }
    return res;
}

list<IPTableEntry*> IPLookUpTable::listScannedIPs()
{
    list<IPTableEntry*> res;
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            if(cur->getType() == IPTableEntry::SUCCESSFULLY_SCANNED)
                res.push_back(cur);
        }
    }
    return res;
}

void IPLookUpTable::reviewScannedIPs()
{
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
        {
            IPTableEntry *cur = (*j);
            unsigned char TTL = cur->getTTL();
            if(TTL == IPTableEntry::NO_KNOWN_TTL || (TTL > 1 && cur->getTrail().isVoid()))
                cur->setType(IPTableEntry::UNSUCCESSFULLY_SCANNED);
            else
                cur->setType(IPTableEntry::SUCCESSFULLY_SCANNED);
        }
    }
}

unsigned short IPLookUpTable::isFlickering(IPTableEntry *cur, 
                                           IPTableEntry *prev, 
                                           IPTableEntry *prevPrev)
{
    // TTLs must be the same
    if(cur->getTTL() != prev->getTTL() || cur->getTTL() != prevPrev->getTTL())
        return 0;
    
    // All IPs must have trails
    if(cur->getTrail().isVoid() || prev->getTrail().isVoid() || prevPrev->getTrail().isVoid())
        return 0;
    
    Trail &t1 = cur->getTrail();
    Trail &t2 = prev->getTrail();
    Trail &t3 = prevPrev->getTrail();
    
    // Flickering analysis only makes sense with trails without anomalies
    if(t1.getNbAnomalies() > 0 || t2.getNbAnomalies() > 0 || t3.getNbAnomalies() > 0)
        return 0;
    
    // To continue, t1 IP must be the same as t3 while differing from t2
    InetAddress refIP = t1.getLastValidIP();
    if(refIP != t3.getLastValidIP() || refIP == t2.getLastValidIP())
        return 0;
    
    unsigned long ulIP1 = ((InetAddress) (*cur)).getULongAddress();
    unsigned long ulIP2 = ((InetAddress) (*prev)).getULongAddress();
    unsigned long ulIP3 = ((InetAddress) (*prevPrev)).getULongAddress();
    
    unsigned short delta = (unsigned short) ((ulIP1 - ulIP2) + (ulIP2 - ulIP3));
    return delta;
}

void IPLookUpTable::reviewSpecialIPs(unsigned short maxDelta)
{
    /*
     * Next piece of code ensures that all IPs appearing in trails will be in the dictionary as 
     * well. It also implements a specific policy regarding TTL: for all target IPs, the "main" 
     * TTL is the one observed with scanning (it can have other TTL values if it's seen among 
     * trails), but for an IP that wasn't part of the initial targets and was discovered via trail 
     * detection, the "main" TTL is supposed to be the shortest, and therefore, TTL is changed 
     * each time a shorter TTL is found.
     */

    list<IPTableEntry*> filtTargets = this->listTargetEntries();
    for(list<IPTableEntry*>::iterator i = filtTargets.begin(); i != filtTargets.end(); ++i)
    {
        IPTableEntry *target = (*i);
        if(target->getType() != IPTableEntry::SUCCESSFULLY_SCANNED)
            continue;
        
        // Creates trail IP in dictionary (if needed) then marks it as such
        Trail &trail = target->getTrail();
        if(!trail.isVoid() && trail.getLastValidIP() != InetAddress(0))
        {
            InetAddress trailIP = trail.getLastValidIP();
            IPTableEntry *assocEntry = this->lookUp(trailIP);
            // If already in dictionary: checks TTL, records it if different (and not an echo trail)
            if(assocEntry != NULL)
            {
                unsigned short trailAnomalies = trail.getNbAnomalies();
                if(trailAnomalies != 0 || trailIP != (InetAddress) (*target))
                {
                    unsigned char trailIPTTL = target->getTTL() - 1 - (unsigned char) trailAnomalies;
                    if(!assocEntry->hasTTL(trailIPTTL))
                    {
                        assocEntry->recordTTL(trailIPTTL);
                        unsigned short type = assocEntry->getType();
                        if(type == IPTableEntry::SEEN_IN_TRAIL || type == IPTableEntry::SEEN_WITH_TRACEROUTE)
                            assocEntry->pickMinimumTTL();
                    }
                }
                
                /*
                 * Checks we don't have a different "Time exceeded" initial TTL if one exists. The 
                 * 0 value means we don't have an initial TTL to begin with. If there are two 
                 * non-zero values that differs, the value "42" is used to advertise there are 
                 * conflicting initial TTL values (none will be used during alias resolution).
                 */
                
                unsigned char TEiTTL = assocEntry->getTimeExceedediTTL();
                if(TEiTTL != 0 && TEiTTL != (unsigned char) 42)
                {
                    if(assocEntry->getTimeExceedediTTL() != trail.getLastValidIPiTTL())
                        assocEntry->setTimeExceedediTTL((unsigned char) 42);
                }
                else if(TEiTTL == 0)
                {
                    assocEntry->setTimeExceedediTTL(trail.getLastValidIPiTTL());
                }
            }
            // Otherwise, creates the dictionary entry
            else
            {
                unsigned short trailAnomalies = trail.getNbAnomalies();
                unsigned char trailIPTTL = target->getTTL() - 1 - (unsigned char) trailAnomalies;
                
                assocEntry = this->create(trailIP);
                assocEntry->setAsTrailIP();
                assocEntry->setTTL(trailIPTTL);
                assocEntry->setType(IPTableEntry::SEEN_IN_TRAIL);
                assocEntry->setTimeExceedediTTL(trail.getLastValidIPiTTL());
            }
        }
    }
    
    // Second visit to mark warping/echoing IPs and trails, and detect flickering
    IPTableEntry *prevPrev = NULL, *prev = NULL;
    for(list<IPTableEntry*>::iterator i = filtTargets.begin(); i != filtTargets.end(); ++i)
    {
        IPTableEntry *target = (*i);
        if(target->getType() != IPTableEntry::SUCCESSFULLY_SCANNED)
            continue;
        
        // Marks trail IP
        Trail &trail = target->getTrail();
        if(!trail.isVoid() && trail.getLastValidIP() != InetAddress(0))
        {
            InetAddress trailIP = trail.getLastValidIP();
            IPTableEntry *assocEntry = this->lookUp(trailIP);
            if(assocEntry == NULL) // Shouldn't occur due to previous loop
                continue;
            
            /*
             * Remark for next lines of code: depending on the situation, a same IP might be 
             * "marked" several times (as trail IP, as warping IP, etc.) during the execution of 
             * this loop. However, since the "marking" methods just set a boolean to true and 
             * receive no parameter, successive calls will be harmless in practice.
             */
            
            // Trail IP
            assocEntry->setAsTrailIP();
            
            // Warping
            if(assocEntry->getNbTTLs() > 0)
            {
                assocEntry->setAsWarping();
                trail.setAsWarping();
            }
            
            // Echoing
            if(trail.getNbAnomalies() == 0 && trailIP == (InetAddress) (*target))
            {
                assocEntry->setAsEchoing();
                trail.setAsEchoing();
            }
            
            // Flickering
            if(prev != NULL && prevPrev != NULL)
            {
                unsigned short delta = IPLookUpTable::isFlickering(target, prev, prevPrev);
                if(delta > 0 && delta <= maxDelta) // Yup, it's flickering
                {
                    // Everything should be available because they checked in isFlickering()
                    Trail &otherTrail = prev->getTrail();
                    InetAddress otherTrailIP = otherTrail.getLastValidIP();
                    
                    IPTableEntry *otherAssocEntry = this->lookUp(otherTrailIP);
                    if(otherAssocEntry == NULL) // Should not occur (just in case)
                        continue;
                    
                    assocEntry->setAsFlickering();
                    assocEntry->addFlickeringPeer(otherTrailIP);
                    otherAssocEntry->setAsFlickering();
                    otherAssocEntry->addFlickeringPeer(trailIP);
                }
            }
        }
        
        prevPrev = prev;
        prev = target;
    }
    
    // Third and last visit to mark flickering trails
    for(list<IPTableEntry*>::iterator i = filtTargets.begin(); i != filtTargets.end(); ++i)
    {
        IPTableEntry *target = (*i);
        if(target->getType() != IPTableEntry::SUCCESSFULLY_SCANNED)
            continue;
        
        Trail &trail = target->getTrail();
        if(!trail.isVoid())
        {
            InetAddress trailIP = trail.getLastValidIP();
            if(trailIP != InetAddress(0) && trail.getNbAnomalies() == 0)
            {
                IPTableEntry *assocEntry = this->lookUp(trailIP);
                if(assocEntry == NULL) // Shouldn't occur due to first loop
                    continue;
                if(assocEntry->isFlickering())
                    trail.setAsFlickering();
            }
        }
    }
}

bool IPLookUpTable::isPotentialPeer(InetAddress needle)
{
    IPTableEntry *found = this->lookUp(needle);
    if(found == NULL)
        return false;
    return found->denotesNeighborhood();
}

void IPLookUpTable::outputDictionary(string filename)
{
    string output = "";
    
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
            output += (*j)->toString() + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766); // File must be accessible to all
}

void IPLookUpTable::outputAliasHints(string filename)
{
    string output = "";
    
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
            if((*j)->getNbHints() > 0)
                output += (*j)->hintsToString();
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void IPLookUpTable::outputFingerprints(string filename)
{
    string output = "";
    
    for(unsigned long i = 0; i < SIZE_TABLE; i++)
    {
        list<IPTableEntry*> &IPList = haystack[i];
        for(list<IPTableEntry*>::iterator j = IPList.begin(); j != IPList.end(); ++j)
            if((*j)->getNbHints() > 0)
                output += (*j)->toStringSimple() + " - " + (*j)->getLattestHints().fingerprintToString() + "\n";
    }
    
    /*
     * N.B.: only the last fingerprint is printed. Why ? Because the fingerprint isn't supposed to 
     * change over time, unlike alias resolution hints which should be renewed whenever we want to 
     * perform alias resolution, at the very least to get the right sequences of IP-IDs (and 
     * perhaps other hints, in the future, which also rely on the momentum to infer aliases).
     */
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
