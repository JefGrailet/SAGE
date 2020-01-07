/*
 * Subnet.cpp
 *
 *  Created on: Nov 27, 2018
 *      Author: jefgrailet
 *
 * Implements the class defined in Subnet.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Subnet.h"

SubnetInterface Subnet::VOID_INTERFACE = SubnetInterface();

Subnet::Subnet(IPTableEntry *pivot)
{
    initialPivot = (InetAddress) (*pivot);
    prefix = (InetAddress) (*pivot);
    prefixLength = 32;
    interfaces.push_back(SubnetInterface(pivot, SubnetInterface::SELECTED_PIVOT));
    
    stopDescription = "";
    toPostProcess = false;
    postProcessed = false;
    
    // Default values for adjusted prefix (set at the end of post-processing)
    adjustedPrefix = InetAddress(0);
    adjustedPrefixLength = 0;
    
    preTrailOffset = 0;
}

Subnet::Subnet(list<Subnet*> subnets)
{
    stringstream ss1;
    string goodSubnet = ""; // If subnets were merged with a sound subnet, will be mentioned apart
    
    // Creates the final list of SubnetInterface objects while listing, in a string, the CIDR's
    bool guardian = true;
    unsigned short aggregated = 0;
    while(subnets.size() > 0)
    {
        Subnet *curSub = subnets.front();
        subnets.pop_front();
        
        if(curSub->hasContrapivots())
        {
            goodSubnet = curSub->getCIDR();
        }
        else
        {
            if(!guardian)
                ss1 << ", ";
            else
                guardian = false;
            ss1 << curSub->getCIDR();
            aggregated++;
        }
        
        list<SubnetInterface> *curIPs = curSub->getInterfacesList();
        while(curIPs->size() > 0)
        {
            interfaces.push_back(curIPs->front());
            curIPs->pop_front();
        }
        delete curSub;
    }
    interfaces.sort(SubnetInterface::smaller); // By design, the list will not be empty
    
    // Finds the prefix that accomodates all interfaces
    SubnetInterface first = interfaces.front();
    SubnetInterface last = interfaces.back();
    
    InetAddress firstIP = (InetAddress) *(first.ip);
    InetAddress lastIP = (InetAddress) *(last.ip);
    
    prefixLength = 32;
    unsigned long uintPrefix = firstIP.getULongAddress();
    unsigned long size = 1;
    while(uintPrefix + size < lastIP.getULongAddress())
    {
        prefixLength--;
        size *= 2;
        uintPrefix = uintPrefix >> (32 - prefixLength);
        uintPrefix = uintPrefix << (32 - prefixLength);
    }
    prefix = InetAddress(uintPrefix);
    
    // Gets the first selected pivot to act as the initial one (just to have a correct pointer)
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &curIP = (*i);
        if(curIP.status == SubnetInterface::SELECTED_PIVOT)
        {
            initialPivot = (InetAddress) *(curIP.ip);
            break;
        }
    }
    
    // Writes the description telling how the subnet was grown
    stringstream ss2;
    ss2 << "aggregate of undergrown subnet";
    if(aggregated > 1)
        ss2 << "s";
    ss2 << " " << ss1.str();
    if(goodSubnet.length() > 0)
        ss2 << " with " << goodSubnet << " (has contra-pivot(s))";
    ss2 << ".";
    stopDescription = ss2.str();
    toPostProcess = false;
    postProcessed = true;
    
    // Default values for adjusted prefix (set at the end of post-processing)
    adjustedPrefix = InetAddress(0);
    adjustedPrefixLength = 0;
    
    preTrailOffset = 0;
}

Subnet::~Subnet()
{
    this->clearInterfaces();
}

void Subnet::addInterface(IPTableEntry *interface, unsigned short subnetRule)
{
    interfaces.push_back(SubnetInterface(interface, subnetRule));
    interfaces.sort(SubnetInterface::smaller);
}

void Subnet::updateInterface(IPTableEntry *interface, unsigned short subnetRule)
{
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if(i->ip == interface)
            i->status = subnetRule;
}

SubnetInterface& Subnet::getSelectedSubnetInterface()
{
    IPTableEntry *selected = NULL;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if(i->status == SubnetInterface::SELECTED_PIVOT)
        {
            IPTableEntry *curIP = i->ip;
            if(curIP == NULL)
                continue;
            Trail &curTrail = curIP->getTrail();
            
            // No need to look further here ("next hop" subnet, single pivot or anomaly-less pivot)
            if(curTrail.isVoid() || curTrail.getNbAnomalies() == 0 || !postProcessed)
                return (*i);
        
            /*
             * If we have a post-processed subnet, we need to ensure the returned pivot has the 
             * best possible trail. Selected pivots deemed as compatible during post-processing 
             * can have differing amount of anomalies; it's therefore important to select the 
             * pivot which the trail as the smallest amount of anomalies, hence the following 
             * code (and also the condition above, which ensures that such a search isn't needed).
             */
            
            if(selected != NULL)
            {
                Trail &assocTrail = selected->getTrail();
                if(curTrail.getNbAnomalies() < assocTrail.getNbAnomalies())
                    selected = curIP;
            }
            else
                selected = curIP;
        }
    }
    
    // Fetches the selected interface as a SubnetInterface
    if(selected != NULL)
        for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
            if(i->ip == selected)
                return (*i);
    return VOID_INTERFACE;
}

IPTableEntry* Subnet::getSelectedPivot()
{
    SubnetInterface &interface = this->getSelectedSubnetInterface();
    if(!interface.isVoid())
        return interface.ip;
    return NULL; // To please the compiler. In practice, this should never occur.
}

IPTableEntry* Subnet::getInterface(InetAddress interface)
{
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if((InetAddress) *(i->ip) == interface)
            return i->ip;
    return NULL;
}

bool Subnet::hasInterfaces(unsigned short type)
{
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if(i->status == type)
            return true;
    return false;
}

unsigned short Subnet::countInterfaces(unsigned short type)
{
    unsigned short count = 0;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if(i->status == type)
            count++;
    return count;
}

list<IPTableEntry*> Subnet::listInterfaces(unsigned short type)
{
    list<IPTableEntry*> res;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if(i->status == type)
            res.push_back(i->ip);
    return res;
}

/*
 * N.B.: "ALTERNATIVE_CONTRAPIVOT" is irrelevant for the next two methods (for now) since this 
 * notion only appears at the end of post-processing and since these methods are relevant during 
 * post-processing.
 */

bool Subnet::hasOnlyPivots()
{
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        if(i->status == SubnetInterface::OUTLIER || i->status == SubnetInterface::CONTRAPIVOT)
            return false;
    return true;
}

vector<unsigned short> Subnet::countAllInterfaces()
{
    vector<unsigned short> amounts;
    for(unsigned short i = 0; i < 3; i++)
        amounts.push_back(0);
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if(i->status == SubnetInterface::OUTLIER)
            amounts[2]++;
        else if(i->status == SubnetInterface::CONTRAPIVOT)
            amounts[1]++;
        else
            amounts[0]++;
    }
    return amounts;
}

list<IPTableEntry*> Subnet::listBetween(InetAddress low, InetAddress up)
{
    list<IPTableEntry*> res;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        InetAddress ip = (InetAddress) *(i->ip);
        if(ip > low && ip < up)
            res.push_back(i->ip);
    }
    return res;
}

void Subnet::clearInterfaces()
{
    interfaces.clear();
}

InetAddress Subnet::getLowerBorder(bool withLimits)
{
    if(prefixLength == 32)
        return initialPivot;

    if(!withLimits)
        return prefix + 1;
    return prefix;
}

InetAddress Subnet::getUpperBorder(bool withLimits)
{
    if(prefixLength == 32)
        return initialPivot;

    // N.B.: method to produce the upper border is identical to common/init/NetworkAddress
    unsigned long mask = ~0;
	mask = mask >> prefixLength;
	InetAddress upperBorderIP = InetAddress(prefix.getULongAddress() ^ mask);
	if(!withLimits)
	    return upperBorderIP - 1;
	return upperBorderIP;
}

bool Subnet::contains(InetAddress interface)
{
    InetAddress lowerBound = prefix;
    InetAddress upperBound = this->getUpperBorder();
    if(interface >= lowerBound && interface <= upperBound)
        return true;
    return false;
}

bool Subnet::overlaps(Subnet *other)
{
    InetAddress lowerBound = prefix;
    InetAddress upperBound = this->getUpperBorder();
    
    InetAddress oLowerBound = other->getLowerBorder();
    InetAddress oUpperBound = other->getUpperBorder();
    
    if(lowerBound <= oUpperBound && oLowerBound <= upperBound)
        return true;
    return false;
}

void Subnet::resetPrefix()
{
    unsigned long mask = ~0;
	mask = mask >> (32 - prefixLength);
	mask = mask << (32 - prefixLength);
    prefix = InetAddress(initialPivot.getULongAddress() & mask);
}

void Subnet::expand()
{
    prefixLength--;
    this->resetPrefix();
}

void Subnet::shrink(list<IPTableEntry*> *out)
{
    prefixLength++;
    this->resetPrefix();
    
    InetAddress newLowBorder = this->getLowerBorder(), newUpBorder = this->getUpperBorder();
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        InetAddress correspondingIP = (InetAddress) *(cur.ip);
        if(correspondingIP < newLowBorder || correspondingIP > newUpBorder)
        {
            if(out != NULL)
                out->push_back(cur.ip);
            interfaces.erase(i--);
        }
    }
}

unsigned char Subnet::getPivotTTL(unsigned short otherType)
{
    unsigned char TTL = 0;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        if(otherType == 1 && cur.status != SubnetInterface::CONTRAPIVOT)
            continue;
        if(otherType != 1 && cur.status == SubnetInterface::CONTRAPIVOT)
            continue;
        if(otherType == 2 && cur.status != SubnetInterface::OUTLIER)
            continue;
        if(otherType != 2 && cur.status == SubnetInterface::OUTLIER)
            continue;
        
        if(TTL != 0 && cur.ip->getTTL() != TTL)
            return 255;
        else
            TTL = cur.ip->getTTL();
    }
    return TTL;
}

unsigned char Subnet::getSmallestTTL()
{
    unsigned char TTL = 255;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        unsigned char curTTL = i->ip->getTTL();
        if(curTTL < TTL)
            TTL = curTTL;
    }
    return TTL;
}

void Subnet::findAdjustedPrefix()
{
    if(prefixLength == 32)
        return;
    
    // Buffers non-adjusted data to restore latter (if adjusted prefix differs)
    InetAddress prevPrefix = prefix;
    unsigned short prevPrefixLength = prefixLength;
    
    // Shrinks up to the point where any listed interface is no longer encompassed
    bool coversEverything = true;
    while(coversEverything && prefixLength < 32)
    {
        prefixLength++;
        this->resetPrefix();
        
        InetAddress newLowBorder = this->getLowerBorder(), newUpBorder = this->getUpperBorder();
        for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
        {
            InetAddress curIP = (InetAddress) *(i->ip);
            if(curIP < newLowBorder || curIP > newUpBorder)
            {
                coversEverything = false;
                break;
            }
        }
    }
    
    // Expands once to undo the last shrinkage
    prefixLength--;
    this->resetPrefix();
    
    // Adjusted prefix differs: sets data accordingly
    if(prefix != prevPrefix || prefixLength != prevPrefixLength)
    {
        adjustedPrefix = prefix;
        adjustedPrefixLength = prefixLength;
        
        prefix = prevPrefix;
        prefixLength = prevPrefixLength;
    }
}

void Subnet::findAlternativeContrapivot(unsigned short maxNb)
{
    if(this->hasContrapivots())
        return;
    
    // Do TTLs of pivots vary and are there any outliers out there ?
    unsigned short nbOutliers = 0;
    list<unsigned short> TTLs;
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        if(cur.status == SubnetInterface::OUTLIER)
        {
            nbOutliers++;
            continue;
        }
        
        unsigned short TTL = cur.ip->getTTL();
        if(TTLs.size() != 0)
        {
            // Not very elegant, but since there are not dozens of TTL values, should be fine
            bool found = false;
            for(list<unsigned short>::iterator j = TTLs.begin(); j != TTLs.end(); ++j)
            {
                if((*j) == TTL)
                {
                    found = true;
                    break;
                }
            }
            
            if(!found)
                TTLs.push_back(TTL);
        }
        else
            TTLs.push_back(TTL);
    }
    
    // Varying TTLs for pivots ?
    if(TTLs.size() < 2)
        return;
    
    // Not too many outliers (less than 5 and less than 1/3 of all interfaces) ?
    if(nbOutliers > maxNb || nbOutliers > ((interfaces.size() / 3) * 2))
        return;
    
    // If we reach this point, we can re-label outliers as contra-pivots (alternative definition)
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        if(cur.status == SubnetInterface::OUTLIER)
            cur.status = SubnetInterface::ALTERNATIVE_CONTRAPIVOT;
    }
}

Trail& Subnet::getTrail()
{
    SubnetInterface &refPivot = this->getSelectedSubnetInterface();
    if(refPivot.isVoid())
        return IPTableEntry::VOID_TRAIL;
    return refPivot.ip->getTrail();
}

list<InetAddress> Subnet::getDirectTrailIPs()
{
    list<InetAddress> res;

    SubnetInterface &refPivot = this->getSelectedSubnetInterface();
    if(refPivot.isVoid())
        return res;
    Trail &refTrail = refPivot.ip->getTrail();
    if(refTrail.isVoid() || refTrail.getNbAnomalies() > 0)
        return res; // No direct trails
    
    res.push_back(refTrail.getLastValidIP());
    
    // Looking for other direct trails
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        unsigned short type = cur.status;
        if(type == SubnetInterface::RULE_4_FLICKERING || type == SubnetInterface::RULE_5_ALIAS)
        {
            Trail &assocTrail = cur.ip->getTrail();
            if(assocTrail.isVoid() || assocTrail.getNbAnomalies() > 0)
                continue;
        
            /*
             * The code now checks we don't have already listed this IP. This isn't optimal 
             * algorithmically, but the amount of listed IPs is normally very small (4 or 5 would 
             * already be a lot).
             */
            
            InetAddress trailIP = assocTrail.getLastValidIP();
            bool duplicate = false;
            for(list<InetAddress>::iterator j = res.begin(); j != res.end(); ++j)
            {
                if(trailIP == (*j))
                {
                    duplicate = true;
                    break;
                }
            }
            
            if(!duplicate)
                res.push_back(trailIP);
        }
    }
    
    return res;
}

list<SubnetInterface*> Subnet::getPeerDiscoveryPivots(unsigned short maxNb)
{
    list<SubnetInterface*> VPs;
    SubnetInterface &refPivot = this->getSelectedSubnetInterface();
    if(refPivot.isVoid())
        return VPs;
    VPs.push_back(&refPivot);
    if(maxNb <= 1)
        return VPs;
    
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        bool potentialVP = false;
        if(cur.status == SubnetInterface::RULE_1_TRAIL)
            potentialVP = true;
        else if(cur.status == SubnetInterface::RULE_3_ECHOES)
            potentialVP = true;
        else if(cur.status == SubnetInterface::RULE_4_FLICKERING)
            potentialVP = true;
        else if(cur.status == SubnetInterface::RULE_5_ALIAS)
            potentialVP = true;
        
        if(potentialVP)
        {
            VPs.push_back(&cur);
            if(VPs.size() >= maxNb)
                break;
        }
    }
    return VPs;
}

void Subnet::findPreTrailIPs()
{
    if(preTrailIPs.size() > 0)
        return; // No pointer because it won't be edited
    
    // 1) Lists subnet interfaces with a (partial) route
    list<SubnetInterface> withRoute;
    for(list<SubnetInterface>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
    {
        SubnetInterface &cur = (*j);
        if(cur.partialRoute.size() > 0)
            withRoute.push_back(cur);
    }
    
    if(withRoute.size() == 0)
        return;
    
    // 2) Looks for non-anonymous IPs appearing before the trail
    unsigned short offset = 0;
    while(preTrailIPs.size() == 0 && withRoute.size() > 0)
    {
        for(list<SubnetInterface>::iterator i = withRoute.begin(); i != withRoute.end(); ++i)
        {
            vector<RouteHop> route = i->partialRoute;
            unsigned short routeLen = route.size();
            if(offset >= routeLen)
                withRoute.erase(i--);
            
            short currentOffset = routeLen - 1 - offset;
            if(currentOffset >= 0 && route[currentOffset].isValidHop())
                preTrailIPs.push_back(route[currentOffset].ip);
        }
        offset++;
    }
    
    // 3) List is cleaned (removal of duplicata) and offset is set
    if(preTrailIPs.size() > 0)
    {
        preTrailOffset = offset - 1; // "- 1" because of the final incrementation
        preTrailIPs.sort(InetAddress::smaller);
        InetAddress prev(0);
        for(list<InetAddress>::iterator i = preTrailIPs.begin(); i != preTrailIPs.end(); ++i)
        {
            InetAddress cur = (*i);
            if(cur == prev)
            {
                preTrailIPs.erase(i--);
                continue;
            }
            prev = cur;
        }
    }
}

string Subnet::getCIDR(bool verbose)
{
    stringstream ss;
    if(verbose && adjustedPrefixLength > 0 && adjustedPrefix != InetAddress(0))
    {
        if(adjustedPrefix != prefix || adjustedPrefixLength != prefixLength)
        {
            ss << adjustedPrefix << "/" << adjustedPrefixLength;
            ss << " (adjusted from " << prefix << "/" << prefixLength << ")";
        }
        else
        {
            ss << prefix << "/" << prefixLength;
        }
    }
    else
    {
        ss << prefix << "/" << prefixLength;
    }
    return ss.str();
}

string Subnet::getAdjustedCIDR()
{
    stringstream ss;
    if(adjustedPrefixLength > 0 && adjustedPrefix != InetAddress(0))
    {
        if(adjustedPrefix != prefix || adjustedPrefixLength != prefixLength)
            ss << adjustedPrefix << "/" << adjustedPrefixLength;
        else
            ss << prefix << "/" << prefixLength;
    }
    else
        ss << prefix << "/" << prefixLength;
    return ss.str();
}

unsigned short Subnet::getAdjustedPrefixLength()
{
    if(adjustedPrefixLength > 0 && adjustedPrefixLength != prefixLength)
        return adjustedPrefixLength;
    return prefixLength;
}

unsigned int Subnet::getTotalCoveredIPs()
{
    unsigned short prefixLen = this->getAdjustedPrefixLength();
    unsigned int size = 1;
    for(unsigned short i = 0; i < 32 - prefixLen; i++)
        size *= 2;
    return size;
}

string Subnet::toString()
{
    stringstream ss;
    
    ss << this->getCIDR(true) << "\n";
    for(list<SubnetInterface>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        SubnetInterface &cur = (*i);
        IPTableEntry *curIP = cur.ip;
        unsigned short dispTTL = (unsigned short) curIP->getTTL();
        ss << dispTTL << " - " << (InetAddress) (*curIP);
        if(dispTTL > 1 && !curIP->getTrail().isVoid())
            ss << " - " << curIP->getTrail();
        else if(dispTTL == 1)
            ss << " - [This computer]";
        ss << " - ";
        switch(cur.status)
        {
            case SubnetInterface::SELECTED_PIVOT:
                ss << "Selected pivot";
                break;
            case SubnetInterface::CONTRAPIVOT:
                ss << "Contra-pivot";
                break;
            case SubnetInterface::ALTERNATIVE_CONTRAPIVOT:
                ss << "Contra-pivot (alternative definition)";
                break;
            case SubnetInterface::RULE_1_TRAIL:
                ss << "Rule 1";
                break;
            case SubnetInterface::RULE_2_TTL_TIMEOUT:
                ss << "Rule 2";
                break;
            case SubnetInterface::RULE_3_ECHOES:
                ss << "Rule 3";
                break;
            case SubnetInterface::RULE_4_FLICKERING:
                ss << "Rule 4";
                break;
            case SubnetInterface::RULE_5_ALIAS:
                ss << "Rule 5";
                break;
            default:
                ss << "Outlier";
                break;
        }
        ss << "\n";
    }
    if(!stopDescription.empty())
        ss << "Stop: " << stopDescription << "\n";
    
    return ss.str();
}
