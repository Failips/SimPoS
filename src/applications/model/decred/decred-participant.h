/**
 * DecredParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_DECRED_PARTICIPANT_H
#define SIMPOS_DECRED_PARTICIPANT_H

#include "ns3/decred-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class DecredParticipant : public DecredNode {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    DecredParticipant ();

    virtual ~DecredParticipant (void);

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

#endif //SIMPOS_DECRED_PARTICIPANT_H