#include <cstdlib>
#include <cstring>
#include <iostream>
using std::cout;
using std::endl;

#include "../utils/StringUtils.h"

#include "NetworkAddress.h"

const unsigned char NetworkAddress::MAX_PREFIX_LENGTH=31;

unsigned char NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(const InetAddress & localInetAddress)
{
	struct ifaddrs *interfaceIterator = 0, *interfaceList = 0;
	unsigned char prefixLength = 0;
    if(getifaddrs(&interfaceList) < 0)
    {
        throw InetAddressException("Could NOT get list of interfaces via \"getifaddrs\"");
    }

    for(interfaceIterator = interfaceList; interfaceIterator!=0; interfaceIterator = interfaceIterator->ifa_next)
    {
        if(interfaceIterator->ifa_addr == 0)
        {
            continue;
        }
        if(strcmp(interfaceIterator->ifa_name,"lo") == 0)
        {
            continue;
        }

        if(interfaceIterator->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *la = (struct sockaddr_in *) (interfaceIterator->ifa_addr);
            InetAddress add;
            add.setInetAddress((unsigned long int) ntohl(la->sin_addr.s_addr));
            if(add == localInetAddress)
            {
                struct sockaddr_in *netmask = (struct sockaddr_in *) (interfaceIterator->ifa_netmask);
                unsigned long int u = (unsigned long int) ntohl(netmask->sin_addr.s_addr);
                unsigned long int uCount;
                uCount = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
                prefixLength = (unsigned char) (((uCount + (uCount >> 3)) & 030707070707) % 63);
            }
        }
    }
    freeifaddrs(interfaceList);
    if(prefixLength == 0)
    {
	    throw InetAddressException("Could NOT obtain local subnetwork netmask");
    }
    return prefixLength;
}

NetworkAddress *NetworkAddress::getLocalNetworkAddress(const InetAddress & localInetAddress)
{
	unsigned char localPrefixLength = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localInetAddress);
	NetworkAddress *na = new NetworkAddress(localInetAddress, localPrefixLength);
	return na;
}

NetworkAddress::NetworkAddress(const InetAddress & subnet, unsigned char prefixLen):
prefix(subnet),
prefixLength(prefixLen)
{
	if(prefixLen>NetworkAddress::MAX_PREFIX_LENGTH)
	{
    	throw InetAddressException("Invalid subnet prefix in NetworkAddress(" + subnet.getHumanReadableRepresentation() + ", " + StringUtils::Uchar2string(prefixLen) + ")");
	}
	resetSubnetPrefix();
	srand(time(0));
}

NetworkAddress::NetworkAddress(const string & subnet):prefixLength(31)
{
	if(subnet.length() > 18)//18 comes from aaa.bbb.ccc.ddd/mn
	{
		throw InetAddressException("Invalid subnet prefix in NetworkAddress(const string & subnet)");
	}
	char tmp[19];
	strcpy(tmp, subnet.c_str());
	char *token = strtok(tmp, "/");
	prefix = InetAddress(token);
	token = strtok(0,"/");
	setSubnetPrefixLength((char) atoi(token));
	srand(time(0));
}

NetworkAddress::NetworkAddress(const NetworkAddress &subnet)
{
	this->prefix=subnet.prefix;
	this->prefixLength=subnet.prefixLength;
}

NetworkAddress::~NetworkAddress()
{
}

void NetworkAddress::setSubnetPrefix(const InetAddress &subnet)
{
	this->prefix = subnet;
	resetSubnetPrefix();
}

void NetworkAddress::setSubnetPrefixLength(unsigned char prefixLength)
{
	if(prefixLength>NetworkAddress::MAX_PREFIX_LENGTH)
	{
		throw InetAddressException("Invalid subnet prefix length ");
	}
	
	// 101.78.0.0/8 cannot be downcasted to /16 because we don't know value of byte # 3 (which is zero in the example)
	if(this->prefixLength < prefixLength)
	{
		throw InetAddressException("Downcasting the subnet prefix causes loss in subnet prefix address");
	}

	this->prefixLength = prefixLength;
	resetSubnetPrefix();
}

void NetworkAddress::resetSubnetPrefix()
{
	// Makes the host portion all zeros
	unsigned long int tmpAddr = prefix.getULongAddress();
	tmpAddr = tmpAddr>>(32-prefixLength);
	tmpAddr = tmpAddr<<(32-prefixLength);
	prefix.setInetAddress(tmpAddr);
}

string NetworkAddress::getHumanReadableRepresentation() const
{
	return this->prefix.getHumanReadableRepresentation() + "/" + StringUtils::Uchar2string(prefixLength);
}

string NetworkAddress::getBinaryRepresentation() const
{
	string tmp = this->prefix.getBinaryRepresentation();
	return tmp + " " + this->getHumanReadableRepresentation();
}

bool NetworkAddress::subsumes(const InetAddress &ip) const
{
	unsigned long int tmpThis = this->prefix.getULongAddress() >> (32 - prefixLength);
	unsigned long int tmpIP = ip.getULongAddress() >> (32 - prefixLength);
	return (tmpThis ^ tmpIP) == 0;
}

bool NetworkAddress::subsumes(const NetworkAddress &subnet) const
{
	if(subnet.prefixLength < this->prefixLength)
	{
		return false;
	}
	else if(subnet.prefixLength == this->prefixLength)
	{
		return (this->prefix) == subnet.prefix;
	}
	else
	{
		unsigned long int tmpThis = this->prefix.getULongAddress() >> (32 - (this->prefixLength));
		unsigned long int tmpSubnet = subnet.prefix.getULongAddress() >> (32 - (this->prefixLength));
		return (tmpThis ^ tmpSubnet) ==0;
	}
}

InetAddress NetworkAddress::getRandomAddress() const
{
	unsigned long int randomAddr;
	unsigned char *randomAddrPtr = (unsigned char *) &randomAddr;
	int randomPortionLength = ((31 - prefixLength) / 8) +1;
	for(int i = 0; i < randomPortionLength; i++)
	{
		randomAddrPtr[i] = rand() % 256;
	}
	randomAddr = randomAddr << prefixLength;
	randomAddr = randomAddr >> prefixLength;
	return InetAddress(prefix.getULongAddress() | randomAddr);
}

InetAddress NetworkAddress::getLowerBorderAddress() const
{
	return prefix;
}

InetAddress NetworkAddress::getUpperBorderAddress() const
{
	unsigned long int addr = ~0;
	addr = addr >> prefixLength;
	return InetAddress(prefix.getULongAddress() ^ addr);
}

int NetworkAddress::isBorder(const InetAddress &ip) const
{
	// Checks lower border
	if(ip == prefix)
	{
		return 1;
	}
	// Checks upper border
	unsigned long int tmp = ~0;
	tmp = tmp >> prefixLength;
	if((ip.getULongAddress()) == (prefix.getULongAddress() ^ tmp))
		return 2;
	// If both borders are false, returns false
	return 0;
}

bool NetworkAddress::isAdjacent(const NetworkAddress &subnet) const
{
	if(this->prefixLength != subnet.prefixLength)
	{
		return false;
	}
	
	// First 32 - prefixLength + 1 bits aligned to right
	unsigned long tmpThis = (this->prefix.getULongAddress()) >> (32 - prefixLength);
	unsigned long tmpSubnet = (subnet.prefix.getULongAddress()) >> (32 - prefixLength);
	return (tmpThis ^ tmpSubnet) == 1;
}

NetworkAddress NetworkAddress::getAdjacent() const
{
	unsigned long int mask = 1 << (32 - prefixLength);
	unsigned long int tmpPrefix = prefix.getULongAddress() ^ mask;
	return NetworkAddress(InetAddress(tmpPrefix), prefixLength);
}

bool NetworkAddress::mergeAdjacent(const NetworkAddress &subnet)
{
	if(!(this->isAdjacent(subnet)))
	{
		return false;
	}
	else
	{
		this->prefixLength--;
		resetSubnetPrefix();
		return true;
	}
}
