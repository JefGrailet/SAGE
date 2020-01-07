/*
 * ProbeRecord.cpp
 *
 *  Created on: ?
 *      Author: root
 *
 * Implements what is described in ProbeRecord.h. See that file for more details on the purpose 
 * of this class.
 */

#include "ProbeRecord.h"

ProbeRecord::ProbeRecord(InetAddress dstAddr, 
                         InetAddress rpAddr, 
                         TimeVal rqTime, 
                         TimeVal rpTime, 
                         unsigned char rqTTL, 
                         unsigned char rpTTL, 
                         unsigned char rpICMPtype, 
                         unsigned char rpICMPcode, 
                         unsigned short srIPidentifier, 
                         unsigned short rpIPidentifier, 
                         unsigned char payTTL, 
                         unsigned short payLen, 
                         unsigned long oTs, 
                         unsigned long rTs, 
                         unsigned long tTs, 
                         int prbCost, 
                         bool ffID):
dstAddress(dstAddr), 
rplyAddress(rpAddr), 
reqTime(rqTime), 
rplyTime(rpTime), 
reqTTL(rqTTL), 
rplyTTL(rpTTL), 
rplyICMPtype(rpICMPtype), 
rplyICMPcode(rpICMPcode), 
srcIPidentifier(srIPidentifier), 
rplyIPidentifier(rpIPidentifier), 
payloadTTL(payTTL), 
payloadLength(payLen), 
originateTs(oTs), 
receiveTs(rTs), 
transmitTs(tTs), 
probingCost(prbCost), 
usingFixedFlowID(ffID)
{

}

ProbeRecord::ProbeRecord(const ProbeRecord &toClone)
{
    *this = toClone;
}

ProbeRecord::~ProbeRecord()
{

}

const ProbeRecord &ProbeRecord::operator=(const ProbeRecord &right)
{
    if(this != &right)
    {
        this->dstAddress = right.dstAddress;
        this->rplyAddress = right.rplyAddress;
        this->reqTime = right.reqTime;
        this->rplyTime = right.rplyTime;
        this->reqTTL = right.reqTTL;
        this->rplyTTL = right.rplyTTL;
        this->rplyICMPtype = right.rplyICMPtype;
        this->rplyICMPcode = right.rplyICMPcode;
        this->srcIPidentifier = right.srcIPidentifier;
        this->rplyIPidentifier = right.rplyIPidentifier;
        this->payloadTTL = right.payloadTTL;
        this->payloadLength = right.payloadLength;
        this->originateTs = right.originateTs;
        this->receiveTs = right.receiveTs;
        this->transmitTs = right.transmitTs;
        this->probingCost = right.probingCost;
        this->usingFixedFlowID = right.usingFixedFlowID;
    }
    return *this;
}

ostream& operator<<(ostream& os, const ProbeRecord& rec)
{
    os << "\nProbe record:\n";
    os << "Request TTL: " << (int) rec.reqTTL << "\n";
    os << "Request IP identifier: " << rec.srcIPidentifier << "\n";
    os << "Request destination address: " << rec.dstAddress << "\n";
    os << "Reply address: " << rec.rplyAddress << "\n";
    os << "Reply TTL: " << (int) rec.rplyTTL << "\n";
    os << "Reply IP identifier: " << rec.rplyIPidentifier << "\n";
    os << "Reply ICMP type: " << (int) rec.rplyICMPtype << " - ";
    switch((int) rec.rplyICMPtype)
    {
        case 0:
            os << "Echo reply";
            break;
        case 3:
            os << "Destination unreachable\n";
            os << "Reply ICMP code: " << (int) rec.rplyICMPcode << " - ";
            switch((int) rec.rplyICMPcode)
            {
                case 0: os << "Net Unreachable"; break;
                case 1: os << "Host Unreachable"; break;
                case 2: os << "Protocol Unreachable"; break;
                case 3: os << "Port Unreachable"; break;
                case 4: os << "Fragmentation Needed and Don't Fragment was Set"; break;
                case 5: os << "Source Route Failed"; break;
                case 6: os << "Destination Network Unknown"; break;
                case 7: os << "Destination Host Unknown"; break;
                case 8: os << "Source Host Isolated"; break;
                case 9: os << "Communication with Destination Network is Administratively Prohibited"; break;
                case 10: os << "Communication with Destination Host is Administratively Prohibited"; break;
                case 11: os << "Destination Network Unreachable for Type of Service"; break;
                case 12: os << "Destination Host Unreachable for Type of Service"; break;
                case 13: os << "Communication Administratively Prohibited"; break;
                case 14: os << "Host Precedence Violation"; break;
                case 15: os << "Precedence cutoff in effect"; break;
                default: os << "Unknown"; break;
            }
            break;
        case 4:
            os << "Source quench";
            break;
        case 5:
            os << "Redirect\n";
            os << "Reply ICMP code: " << (int) rec.rplyICMPcode << " - ";
            switch((int) rec.rplyICMPcode)
            {
                case 0: os << "Redirect Datagram for the Network (or subnet)"; break;
                case 1: os << "Redirect Datagram for the Host"; break;
                case 2: os << "Redirect Datagram for the Type of Service and Network"; break;
                case 3: os << "Redirect Datagram for the Type of Service and Host"; break;
                default: os << "Unknown"; break;
            }
            break;
        case 6:
            os << "Alternate host address";
            break;
        case 8:
            os << "Echo";
            break;
        case 11:
            os << "Time exceeded\n";
            os << "Reply ICMP code: " << (int) rec.rplyICMPcode << " - ";
            switch((int) rec.rplyICMPcode)
            {
                case 0: os << "Time to Live exceeded in Transit"; break;
                case 1: os << "Fragment Reassembly Time Exceeded"; break;
                default: os << "Unknown"; break;
            }
            break;
        case 12:
            os << "Parameter problem\n";
            os << "Reply ICMP code: " << (int) rec.rplyICMPcode << " - ";
            switch((int) rec.rplyICMPcode)
            {
                case 0: os << "Pointer indicates the error"; break;
                case 1: os << "Missing a Required Option"; break;
                case 2: os << "Bad Length"; break;
                default: os << "Unknown"; break;
            }
            break;
        case 13:
            os << "Timestamp";
            break;
        case 14:
            os << "Timestamp reply";
            break;
        case 101:
            os << "TCP Reset";
            break;
        case 255:
            os << "Unset (dummy record for unsuccessful probe)";
            break;
        default:
            os << "Unexpected type, see documentation and check code\n";
            os << "Reply ICMP code: " << (int) rec.rplyICMPcode;
            break;
        
        /*
         * Note: there are many possible other ICMP types, but they should occur rarely while 
         * using TreeNET (plus, some of the types above are themselves unlikely). Therefore, 
         * they are not documented further in the log. See a reference on ICMP for more 
         * details.
         */
    }
    os << "\n";
    os << "Payload TTL: " << (int) rec.payloadTTL << "\n";
    os << "Payload length: " << rec.payloadLength << "\n";
    os << "Request time: " << rec.reqTime << "\n";
    os << "Reply time: " << rec.rplyTime << "\n";
    
    if(rec.rplyICMPtype == 14) // ICMP timestamp reply
    {
        os << "\nTimestamp fields:\n";
        os << "Originate TS: " << rec.originateTs << "\n";
        os << "Receive TS: " << rec.receiveTs << "\n";
        os << "Transmit TS: " << rec.transmitTs << "\n";
    }
    
    return os;
}

bool ProbeRecord::isATimeout()
{
    if(rplyAddress == InetAddress(0) && (int) rplyICMPtype == 255 && (int) rplyICMPcode == 255 && (int) rplyTTL == 0)
        return true;
    return false;
}
