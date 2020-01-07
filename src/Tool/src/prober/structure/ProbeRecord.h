/*
 * Originally undescribed class (though usage is obvious).
 *
 * Modifications brought by J.-F. Grailet:
 * -December 2014: improved coding style.
 * -March 4, 2016: this class was edited to add a new field, "srcIPidentifier". Indeed, some IPs 
 *  echoes the IP identifier that was in the initial packet rather than providing their own, which 
 *  can result in inconsistencies while performing alias resolution.
 * -Augustus 2016: the record route option has been entirely removed from the program (see 
 *  DirectProber.h), and as a consequence, it disappears as well from ProbeRecord.
 * -October 2019: light changes in order to handle ProbeRecord objects more easily.
 */

#ifndef PROBERECORD_H_
#define PROBERECORD_H_

#include <string>
using std::string;
#include <iostream>
using std::ostream;
#include <iomanip>
using std::setw;
using std::setiosflags;
using std::ios;

#include "../../common/inet/InetAddress.h"
#include "../../common/date/TimeVal.h"

class ProbeRecord
{
public:
    
    ProbeRecord(InetAddress dstAddress = InetAddress(0),
                InetAddress rplyAddress = InetAddress(0),
                TimeVal reqTime = TimeVal(0, 0),
                TimeVal rplyTime = TimeVal(0, 0),
                unsigned char reqTTL = 0,
                unsigned char rplyTTL = 0,
                unsigned char rplyICMPtype = 255,
                unsigned char rplyICMPcode = 255, 
                unsigned short srcIPidentifier = 0, 
                unsigned short rplyIPidentifier = 0, 
                unsigned char payloadTTL = 0, 
                unsigned short payloadLength = 0, 
                unsigned long originateTs = 0, 
                unsigned long receiveTs = 0, 
                unsigned long transmitTs = 0, 
                int probingCost = 1, 
                bool usingFixedFlowID = false);
    
    ProbeRecord(const ProbeRecord &toClone);
    virtual ~ProbeRecord();
    
    // Normally automatically added by compiler; written to explicitely tell it's being used
    const ProbeRecord &operator=(const ProbeRecord &right);
    
    // Query methods
    inline bool isVoidRecord() const { return dstAddress.isUnset(); }
    inline bool isAnonymousRecord() const { return rplyAddress.isUnset(); }

    // Setters
    void setDstAddress(const InetAddress &addr) { this->dstAddress.setInetAddress(addr.getULongAddress()); }
    void setRplyAddress(const InetAddress &addr) { this->rplyAddress.setInetAddress(addr.getULongAddress()); }
    void setReqTime(TimeVal reqTime) { this->reqTime = reqTime; }
    void setRplyTime(TimeVal rplyTime) { this->rplyTime = rplyTime; }
    void setReqTTL(unsigned char reqTTL) { this->reqTTL = reqTTL; }
    void setRplyTTL(unsigned char rplyTTL) { this->rplyTTL = rplyTTL; }
    void setRplyICMPtype(unsigned char rplyICMPtype) { this->rplyICMPtype = rplyICMPtype; }
    void setRplyICMPcode(unsigned char rplyICMPcode) { this->rplyICMPcode = rplyICMPcode; }
    void setSrcIPidentifier(unsigned short srcIPidentifier) { this->srcIPidentifier = srcIPidentifier; }
    void setRplyIPidentifier(unsigned short rplyIPidentifier) { this->rplyIPidentifier = rplyIPidentifier; }
    void setPayloadTTL(unsigned char payloadTTL) { this->payloadTTL = payloadTTL; }
    void setPayloadLength(unsigned short payloadLength) { this->payloadLength = payloadLength; }
    void setOriginateTs(unsigned long originateTs) { this->originateTs = originateTs; }
    void setReceiveTs(unsigned long receiveTs) { this->receiveTs = receiveTs; }
    void setTransmitTs(unsigned long transmitTs) { this->transmitTs = transmitTs; }
    void setProbingCost(int probingCost) { this->probingCost = probingCost; }
    void setUsingFixedFlowID(bool usingFixedFlowID) { this->usingFixedFlowID = usingFixedFlowID; }

    // Accessers
    const InetAddress &getDstAddress() const { return dstAddress; }
    const InetAddress &getRplyAddress() const { return rplyAddress; }
    const TimeVal &getReqTime() const { return reqTime; }
    const TimeVal &getRplyTime() const { return rplyTime; }
    unsigned char getReqTTL() const { return reqTTL; }
    unsigned char getRplyTTL() const { return rplyTTL; }
    unsigned char getRplyICMPtype() const { return rplyICMPtype; }
    unsigned char getRplyICMPcode() const { return rplyICMPcode; }
    unsigned short getSrcIPidentifier() const { return srcIPidentifier; }
    unsigned short getRplyIPidentifier() const { return rplyIPidentifier; }
    unsigned char getPayloadTTL() const { return payloadTTL; }
    unsigned short getPayloadLength() const { return payloadLength; }
    unsigned long getOriginateTs() const { return originateTs; }
    unsigned long getReceiveTs() const { return receiveTs; }
    unsigned long getTransmitTs() const { return transmitTs; }
    int getProbingCost() const { return this->probingCost; }
    bool getUsingFixedFlowID() const { return this->usingFixedFlowID; }
    
    // Changes brought by J.-F. Grailet
    friend ostream& operator<<(ostream& os, const ProbeRecord& rec);
    bool isATimeout();

protected:

    InetAddress dstAddress;
    InetAddress rplyAddress;
    TimeVal reqTime;
    TimeVal rplyTime;
    unsigned char reqTTL;
    unsigned char rplyTTL;
    unsigned char rplyICMPtype; // 101 implies TCP RESET obtained
    unsigned char rplyICMPcode; // 101 implies TCP RESET obtained
    unsigned short srcIPidentifier;
    unsigned short rplyIPidentifier;
    unsigned char payloadTTL;
    unsigned short payloadLength;
    unsigned long originateTs; // Only set and used for TS replies
    unsigned long receiveTs; // Only set and used for TS replies
    unsigned long transmitTs; // Only set and used for TS replies
    int probingCost;
    bool usingFixedFlowID;

};

#endif /* PROBERECORD_H_ */
