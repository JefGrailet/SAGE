/*
 * Alias.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in Alias.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Alias.h"

Alias::Alias()
{
}

Alias::Alias(IPTableEntry *firstInterface)
{
    interfaces.push_back(AliasedInterface(firstInterface, AliasedInterface::FIRST_IP));
}

Alias::~Alias()
{
    interfaces.clear();
}

void Alias::addInterface(IPTableEntry *IP, unsigned short aliasMethod)
{
    interfaces.push_back(AliasedInterface(IP, aliasMethod));
    interfaces.sort(AliasedInterface::smaller);
}

void Alias::addInterfaces(list<IPTableEntry*> *IPs, unsigned short aliasMethod)
{
    if(IPs->size() == 0)
        return;
    while(IPs->size() > 0)
    {
        interfaces.push_back(AliasedInterface(IPs->front(), aliasMethod));
        IPs->pop_front();
    }
    interfaces.sort(AliasedInterface::smaller);
}

unsigned short Alias::getNbInterfaces()
{
    return (unsigned short) interfaces.size();
}

bool Alias::hasInterface(InetAddress interface)
{
    for(list<AliasedInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if((InetAddress) *(i->ip) == interface)
            return true;
    return false;
}

IPTableEntry* Alias::getMergingPivot()
{
    for(list<AliasedInterface>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        AliasedInterface interface = (*it);
        if(interface.aliasMethod == AliasedInterface::UDP_PORT_UNREACHABLE)
        {
            AliasHints &hints = interface.ip->getLattestHints();
            if(hints.getIPIDCounterType() == AliasHints::HEALTHY_COUNTER)
                return interface.ip;
        }
    }
    return NULL;
}

string Alias::toString()
{
    stringstream result;
    for(list<AliasedInterface>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << " ";
        result << *(it->ip);
    }
    return result.str();
}

string Alias::toStringSemiVerbose()
{
    stringstream result;
    unsigned short aliasMethods[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(list<AliasedInterface>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        result << *(it->ip);
        if(it->aliasMethod != AliasedInterface::FIRST_IP)
        {
            switch(it->aliasMethod)
            {
                case AliasedInterface::FLICKERING_FINGERPRINT:
                    aliasMethods[0]++;
                    break;
                case AliasedInterface::UDP_PORT_UNREACHABLE:
                    aliasMethods[1]++;
                    break;
                case AliasedInterface::ALLY:
                    aliasMethods[2]++;
                    break;
                case AliasedInterface::IPID_VELOCITY:
                    aliasMethods[3]++;
                    break;
                case AliasedInterface::REVERSE_DNS:
                    aliasMethods[4]++;
                    break;
                case AliasedInterface::GROUP_ECHO:
                    aliasMethods[5]++;
                    break;
                case AliasedInterface::GROUP_ECHO_DNS:
                    aliasMethods[6]++;
                    break;
                case AliasedInterface::GROUP_RANDOM:
                    aliasMethods[7]++;
                    break;
                case AliasedInterface::GROUP_RANDOM_DNS:
                    aliasMethods[8]++;
                    break;
                default:
                    aliasMethods[9]++;
                    break;
            }
        }
    }
    
    unsigned short maximum = aliasMethods[0], maxIndex = 0, total = aliasMethods[0];
    for(unsigned short i = 1; i < 10; i++)
    {
        if(aliasMethods[i] > maximum)
        {
            maximum = aliasMethods[i];
            maxIndex = i;
        }
        total += aliasMethods[i];
    }
    
    if(total > 0)
    {
        string mainMethod = "";
        switch(maxIndex)
        {
            case 0:
                mainMethod = "flickering with same fingerprint";
                break;
            case 1:
                mainMethod = "UDP-based";
                break;
            case 2:
                mainMethod = "Ally";
                break;
            case 3:
                mainMethod = "IP-ID velocity";
                break;
            case 4:
                mainMethod = "reverse DNS";
                break;
            case 5:
                mainMethod = "group by echo IP-ID counter";
                break;
            case 6:
                mainMethod = "group by echo IP-ID counter/DNS";
                break;
            case 7:
                mainMethod = "group by random IP-ID counter";
                break;
            case 8:
                mainMethod = "group by random IP-ID counter/DNS";
                break;
            default:
                mainMethod = "Unknown";
                break;
        }
    
        if(total == maximum)
        {
            result << " (" << mainMethod << ")";
        }
        else
        {
            float ratio = ((float) maximum / (float) total) * 100;
            result << " (" << mainMethod << ", " << ratio << "%" << " of aliases)";
        }
    }
    
    return result.str();
}

string Alias::toStringVerbose()
{
    stringstream result;
    for(list<AliasedInterface>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        result << *(it->ip);
        if(it->aliasMethod != AliasedInterface::FIRST_IP)
        {
            result << " (";
            switch(it->aliasMethod)
            {
                case AliasedInterface::FLICKERING_FINGERPRINT:
                    result << "Flickering peers";
                    break;
                case AliasedInterface::UDP_PORT_UNREACHABLE:
                    result << "UDP unreachable port";
                    break;
                case AliasedInterface::ALLY:
                    result << "Ally";
                    break;
                case AliasedInterface::IPID_VELOCITY:
                    result << "IP-ID Velocity";
                    break;
                case AliasedInterface::REVERSE_DNS:
                    result << "Reverse DNS";
                    break;
                case AliasedInterface::GROUP_ECHO:
                    result << "Echo group";
                    break;
                case AliasedInterface::GROUP_ECHO_DNS:
                    result << "Echo group & DNS";
                    break;
                case AliasedInterface::GROUP_RANDOM:
                    result << "Random group";
                    break;
                case AliasedInterface::GROUP_RANDOM_DNS:
                    result << "Random group & DNS";
                    break;
                default:
                    break;
            }
            result << ")";
        }
    }
    return result.str();
}

string Alias::toStringMinimalist()
{
    stringstream result;
    result << "[";
    bool shortened = false;
    unsigned short i = 0;
    for(list<AliasedInterface>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        
        if(i == 3)
        {
            shortened = true;
            result << "...";
            break;
        }
        else
            result << *(it->ip);
        i++;
    }
    result << "]";
    if(shortened)
        result << " (" << interfaces.size() << " interfaces)";
    return result.str();
}

bool Alias::compare(Alias *a1, Alias *a2)
{
    unsigned short size1 = a1->getNbInterfaces();
    unsigned short size2 = a2->getNbInterfaces();
    
    bool result = false;
    if(size1 == size2)
    {
        list<AliasedInterface> *list1 = a1->getInterfacesList();
        list<AliasedInterface> *list2 = a2->getInterfacesList();
        
        list<AliasedInterface>::iterator it2 = list2->begin();
        for(list<AliasedInterface>::iterator it1 = list1->begin(); it1 != list1->end(); it1++)
        {
            InetAddress ip1 = (InetAddress) *(it1->ip);
            InetAddress ip2 = (InetAddress) *(it2->ip);
            
            if(ip1 < ip2)
            {
                result = true;
                break;
            }
            else if(ip1 > ip2)
            {
                result = false;
                break;
            }
        
            it2++;
        }
    }
    else if(size1 < size2)
        result = true;
    return result;
}

bool Alias::compareBis(Alias *a1, Alias *a2)
{
    AliasedInterface ai1 = a1->getInterfacesList()->front();
    AliasedInterface ai2 = a2->getInterfacesList()->front();
    return IPTableEntry::compare(ai1.ip, ai2.ip);
}

bool Alias::equals(Alias *other)
{
    unsigned short size1 = (unsigned short) interfaces.size();
    unsigned short size2 = other->getNbInterfaces();
    
    if(size1 == size2)
    {
        list<AliasedInterface> *list2 = other->getInterfacesList();
        list<AliasedInterface>::iterator it2 = list2->begin();
        for(list<AliasedInterface>::iterator it1 = interfaces.begin(); it1 != interfaces.end(); it1++)
        {
            InetAddress ip1 = (InetAddress) *(it1->ip);
            InetAddress ip2 = (InetAddress) *(it2->ip);
            
            if(ip1 < ip2 || ip1 > ip2)
                return false;
        
            it2++;
        }
        return true;
    }
    return false;
}
