/*
 * InetAddress.h
 *
 * This file defines a class "InetAddress" used to handle IPs in IPv4 format. The date at which 
 * it was written and the author are unknown (but see source below for more details). This class 
 * was already in ExploreNET and has been improved coding style-wise in September 2015 by J.-F. 
 * Grailet.
 */

#ifndef INETADDRESS_H_
#define INETADDRESS_H_
#include <iostream>
using std::ostream;
#include <netinet/in.h>
#include <ifaddrs.h>
#include <string>
using std::string;
#include <vector>
using std::vector;

#include "InetAddressException.h"
#include "../utils/StringUtils.h"

// VERY NICE SOURCE http://www.lemoda.net/freebsd/net-interfaces/index.html

class InetAddress
{
public:
	static const unsigned long int MAX_IP4;
	static const int ULONG_BIT_LENGTH;
	friend ostream &operator<<(ostream &out, const InetAddress &ip)
	{
		out << ip.getHumanReadableRepresentation(); /* +string("(")+StringUtils::Ulong2string(ip.getULongAddress())+string(")") */
		return out;
	}
	static vector<InetAddress> *getLocalAddressList(); // Formerly auto_ptr<>, removed due to cross-compatibility issues (August 2018)
	static InetAddress getFirstLocalAddress();
	static InetAddress getLocalAddressByInterfaceName(const string &iname);
	static InetAddress getAddressByHostName(const string &hostName);
	static InetAddress getAddressByIPString(const string &stringIP);

	/**
	 * You need to call srand(time(NULL)) before using those two random functions if you want 
	 * different sequences.
	 */
	
	static InetAddress getRandomAddress();
	static InetAddress getUnicastRoutableRandomAddress();
	InetAddress(unsigned long int ip = 0);
	InetAddress(const string &address);
	InetAddress(const InetAddress &address);
	virtual ~InetAddress();
	bool isUnicastRoutableAddress(); // Returns true if the IP address is a unicast routable address
	InetAddress &operator=(const InetAddress &other);
	void operator+=(unsigned int n);
	InetAddress &operator++();
	InetAddress operator++(int);
	InetAddress operator+(unsigned int n);
	
	inline bool operator==(const InetAddress &other) const { return this->ip == other.ip; }
	inline bool operator!=(const InetAddress &other) const { return this->ip != other.ip; }
	inline bool operator<(const InetAddress &other) const { return this->ip < other.ip; }
	inline bool operator<=(const InetAddress &other) const { return this->ip <= other.ip; }
	inline bool operator>(const InetAddress &other) const { return this->ip > other.ip; }
	inline bool operator>=(const InetAddress &other) const { return this->ip >= other.ip; }
	
	void operator-=(unsigned int n);
	InetAddress &operator--();
	InetAddress operator--(int);
	InetAddress operator-(unsigned int n);
	void setBit(int position, int value); // 0 <= position <= 31
	int getBit(int position) const; // 0 <= position <= 31
	void inverseBits(); // Inverses each bit of the IP address e.g.  1011...11 will be 0100...00
	void reverseBits(); // Reverses the bit sequence of the IP address e.g. 1011...11 will be 11...1101
	unsigned long int getULongAddress() const;

    /*
     * (August 2018) N.B.: in former versions, the next 3 methods were using auto_ptr<>, removed 
     * due to cross-compatibility issues (i.e. warnings with more recent environments but none 
     * with Fedora 8, which is the environment of many PlanetLab computers).
     */

    string getHumanReadableRepresentation() const;
	string getBinaryRepresentation() const;
	string getHostName() const;
	
	inline InetAddress get31Mate() const { return InetAddress(1 ^ this->ip); }
	inline bool is31Mate(const InetAddress &addr) const { return ((this->ip ^ addr.ip) == 1); }

	/**
	 * Throws InetAddressException if the IP ends with 00 or 11 binary
	 */
	
	InetAddress get30Mate() const;
	void setInetAddress(unsigned long int address);
	void setInetAddress(const string &address);
	
	inline bool is30Mate(const InetAddress &addr) const { return ((this->ip ^ addr.ip) == 3); }
	inline bool isEnding00() const { return ((this->ip & 3) == 0); }
	inline bool isEnding11() const { return ((this->ip & 3) == 3); }
	inline bool isUnset() const { return (ip == 0); }
	inline void unSet() { this->ip = 0; }
	
	// Comparison method for sorting purposes (added by J.-F. Grailet)
	inline static bool smaller(InetAddress &inet1, InetAddress &inet2) { return inet1 < inet2; }

protected:
	unsigned long int ip;
};

#endif /* INETADDRESS_H_ */
