#ifndef NETWORKADDRESS_H_
#define NETWORKADDRESS_H_

#include <string>
using std::string;
#include <iostream>
using std::ostream;
#include "InetAddress.h"
#include "InetAddressException.h"

// October 2019: removed NetworkAddressSet stuff, since it was unused.

class NetworkAddress {
	friend ostream & operator<<(ostream &out, const NetworkAddress &subnet)
    {
		out<< subnet.getHumanReadableRepresentation();
		return out;
	}
public:
	static const unsigned char MAX_PREFIX_LENGTH;
	static unsigned char getLocalSubnetPrefixLengthByLocalAddress(const InetAddress & localInetAddress);
	static NetworkAddress * getLocalNetworkAddress(const InetAddress & localInetAddress);

	/**
	 * @param subnet might be a subnet address or a host address belonging to the subnet. The
	 * constructor automatically clears the host-address poriton of the address to zero
	 */
	NetworkAddress(const InetAddress & subnet, unsigned char prefixLength);
	/**
	 * @param subnet is in the a.b.c.d/m format
	 */
	NetworkAddress(const string & subnet);
	/**Copy Constructor**/
	NetworkAddress(const NetworkAddress &subnet);
	virtual ~NetworkAddress();
    
	//set methods
	void setSubnetPrefix(const InetAddress & subnet);
	void setSubnetPrefixLength(unsigned char prefixLength);
    
	//get methods
	const InetAddress &getSubnetPrefix()const{return this->prefix;}
	unsigned char getPrefixLength()const{return this->prefixLength;}
	InetAddress getRandomAddress() const;
	InetAddress getLowerBorderAddress() const;
	InetAddress getUpperBorderAddress() const;

    /*
     * (August 2018) N.B.: in former versions, the next 3 methods were using auto_ptr<>, removed 
     * due to cross-compatibility issues (i.e. warnings with more recent environments but none 
     * with Fedora 8, which is the environment of many PlanetLab computers).
     */

    string getBinaryRepresentation() const;
	string getHumanReadableRepresentation() const;

	/**
	 * returns 0,1,2, if ip is NOT border, lower border, and upper border respectively
	 */
    
	int isBorder(const InetAddress &ip) const;
	bool operator==(const NetworkAddress &subnet)const{return this->prefix==subnet.prefix && this->prefixLength==subnet.prefixLength;}
	bool operator!=(const NetworkAddress &subnet)const{return !((*this)==subnet);}
    
	/**
	 * Returns true if @param ip is within the range of this subnetwork, else false.
	 */
    
	bool subsumes(const InetAddress & ip) const;
    
	/**
	 * Returns true if this subnet contains the @param subnet. In case *this==subnet (equal subnets)
	 * it returns true (i.e. a subnet contains itself)
	 */
    
	bool subsumes(const NetworkAddress &subnet) const;

	/**
	 * If p1=a.b.c.0/24 and p2=a.b.0.0/16 then we cannot claim that they are
	 * adjacent. We only can say that p2 contains p1. To have two adjacent
	 * subnet masks i) their prefixLengths should be equal, ii)their prefixes of
	 * prefixLength-1 must be equal, iii) last prefix bits should differ by 1
	 * -i.e. one's is binary 1 the other's is 0-
	 *
	 * A.isAdjacent(B) is equal to B.isAdjacent(A) (i.e. method is symmetric)
	 *
	 */
    
	bool isAdjacent(const NetworkAddress &subnet) const;
    
	/**
	 * Returns a NetworkAddress that is adjacent of this network address
	 */
    
	NetworkAddress getAdjacent() const;
    
	/**
	 * Merges @param subnet with this subnet if they are adjacent
	 * and returns true otherwise returns false. No need to check
	 * if this and @param are adjacent before calling this method
	 * because this is done automatically. Note that it only merges
	 * the subnets having adjacency relationship not contains relationship!
	 */
    
	bool mergeAdjacent(const NetworkAddress &subnet);

private:
	void resetSubnetPrefix();
	InetAddress prefix;
	unsigned char prefixLength;
};

#endif /* NETWORKADDRESS_H_ */
