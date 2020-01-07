/*
 * IPClusterer.cpp
 *
 *  Created on: Aug 30, 2019
 *      Author: jefgrailet
 *
 * Implements the class defined in IPClusterer.h (see this file to learn further about the goals 
 * of such a class).
 */

#include <utility>
using std::pair;
#include <sstream>
using std::stringstream;

#include "IPClusterer.h"

IPClusterer::IPClusterer()
{
}

IPClusterer::~IPClusterer()
{
    while(haystack.size() > 0)
    {
        // Gets the list, removes associated entries and deletes it
        map<InetAddress, list<InetAddress>* >::iterator it = haystack.begin();
        list<InetAddress> *ls = it->second;
        for(list<InetAddress>::iterator i = ls->begin(); i != ls->end(); ++i)
        {
            InetAddress curIP = (*i);
            map<InetAddress, list<InetAddress>* >::iterator res = haystack.find(curIP);
            if(res != haystack.end())
                haystack.erase(res);
        }
        delete ls;
    }
}

void IPClusterer::mergeClusters(list<InetAddress> *c1, list<InetAddress> *c2)
{
    // Inserts all IPs from c2 in c1
    for(list<InetAddress>::iterator i = c2->begin(); i != c2->end(); ++i)
        c1->push_back((*i));
    delete c2;
    
    // Sorts c1 then removes duplicates
    c1->sort(InetAddress::smaller);
    InetAddress prev(0);
    for(list<InetAddress>::iterator i = c1->begin(); i != c1->end(); ++i)
    {
        InetAddress cur = (*i);
        if(prev == cur)
            c1->erase(i--);
        else
            prev = cur;
    }
    
    // Updates haystack
    for(list<InetAddress>::iterator i = c1->begin(); i != c1->end(); ++i)
    {
        InetAddress curIP = (*i);
        map<InetAddress, list<InetAddress>* >::iterator res = haystack.find(curIP);
        if(res != haystack.end())
            haystack.erase(res);
        haystack.insert(pair<InetAddress, list<InetAddress>* >(curIP, c1));
    }
}

void IPClusterer::update(list<InetAddress> cluster)
{
    if(cluster.size() == 0)
        return; // Just in case
    
    // 1) Checks if any IP is matched to an existing list
    list<InetAddress> *refList = NULL;
    for(list<InetAddress>::iterator i = cluster.begin(); i != cluster.end(); ++i)
    {
        map<InetAddress, list<InetAddress>* >::iterator res = haystack.find((*i));
        if(res != haystack.end())
        {
            refList = res->second;
            break;
        }
    }
    
    // New list needs to be created
    if(refList == NULL)
    {
        list<InetAddress> *newList = new list<InetAddress>();
        for(list<InetAddress>::iterator i = cluster.begin(); i != cluster.end(); ++i)
        {
            InetAddress curIP = (*i);
            newList->push_back(curIP);
            haystack.insert(pair<InetAddress, list<InetAddress>* >(curIP, newList));
        }
        newList->sort(InetAddress::smaller);
        return;
    }
    
    // 2) Checks if refList is the only matching list
    bool check = true;
    while(check)
    {
        check = false;
        for(list<InetAddress>::iterator i = cluster.begin(); i != cluster.end(); ++i)
        {
            map<InetAddress, list<InetAddress>* >::iterator res = haystack.find((*i));
            if(res != haystack.end())
            {
                list<InetAddress> *cmpList = res->second;
                if(refList != cmpList)
                {
                    check = true;
                    this->mergeClusters(refList, cmpList);
                }
            }
        }
    }
    
    // 3) Adds the missing IPs in the clusterer
    unsigned short newIPs = 0;
    for(list<InetAddress>::iterator i = cluster.begin(); i != cluster.end(); ++i)
    {
        InetAddress curIP = (*i);
        map<InetAddress, list<InetAddress>* >::iterator res = haystack.find(curIP);
        if(res == haystack.end())
        {
            refList->push_back(curIP);
            haystack.insert(pair<InetAddress, list<InetAddress>* >(curIP, refList));
            newIPs++;
        }
    }
    
    // Sorts refList if new IPs were added
    if(newIPs > 0)
        refList->sort(InetAddress::smaller);
}

list<InetAddress> IPClusterer::getCluster(InetAddress needle)
{
    map<InetAddress, list<InetAddress>* >::iterator res = haystack.find(needle);
    if(res == haystack.end())
    {
        list<InetAddress> empty;
        return empty;
    }
    return *(res->second);
}

string IPClusterer::getClusterString(InetAddress needle)
{
    list<InetAddress> cluster = this->getCluster(needle);
    if(cluster.size() == 0)
        return "";
    stringstream ss;
    for(list<InetAddress>::iterator i = cluster.begin(); i != cluster.end(); ++i)
    {
        if(i != cluster.begin())
            ss << ", ";
        ss << (*i);
    }
    return ss.str();
}

list<list<InetAddress> > IPClusterer::listClusters()
{
    list<InetAddress> needles;
    list<list<InetAddress> > res;
    
    // Re-uses the strategy of deletion with a copy of haystack
    map<InetAddress, list<InetAddress>* > copy(haystack);
    while(copy.size() > 0)
    {
        map<InetAddress, list<InetAddress>* >::iterator it = copy.begin();
        list<InetAddress> *ls = it->second;
        needles.push_back(it->first);
        for(list<InetAddress>::iterator i = ls->begin(); i != ls->end(); ++i)
        {
            InetAddress curIP = (*i);
            map<InetAddress, list<InetAddress>* >::iterator res = copy.find(curIP);
            if(res != copy.end())
                copy.erase(res);
        }
    }
    
    // Using "needles", can provide a kind of sorted list of clusters
    needles.sort(InetAddress::smaller);
    for(list<InetAddress>::iterator i = needles.begin(); i != needles.end(); i++)
        res.push_back(this->getCluster((*i)));
    return res;
}
