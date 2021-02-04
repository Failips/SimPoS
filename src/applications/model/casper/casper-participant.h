/**
 * CasperParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_CASPER_PARTICIPANT_H
#define SIMPOS_CASPER_PARTICIPANT_H

#include "ns3/casper-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class CasperParticipant : public CasperNode {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    CasperParticipant ();

    virtual ~CasperParticipant (void);

    /**
     * set the type of block (vote) broadcast
     */
    void SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType);

protected:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop

    virtual void DoDispose (void);


    //debug
    double       m_timeStart;
    double       m_timeFinish;
    enum BlockBroadcastType   m_blockBroadcastType;      //!< the type of broadcast for votes and blocks
};

} // ns3 namespace

#endif //SIMPOS_CASPER_PARTICIPANT_H
