/**
 * AlgorandParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_ALGORAND_PARTICIPANT_H
#define SIMPOS_ALGORAND_PARTICIPANT_H

#include "ns3/algorand-node.h"
#include <random>

namespace ns3 {

class Address;
class Socket;
class Packet;

class AlgorandParticipant : public AlgorandNode {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    AlgorandParticipant ();

    virtual ~AlgorandParticipant (void);

    /**
     * \brief set the type of block (vote) broadcast
     */
    void SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType);

protected:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop

    virtual void DoDispose (void);

    /**
     * \brief Handle a document received by the application with unknown type number
     * \param document the received document
     */
    virtual void HandleCustomRead (rapidjson::Document *document, double receivedTime, Address receivedFrom);

    /**
     * \brief Block proposal phase -> choosing proposing participants using VRF and sending newly created blocks
     */
    void BlockProposalPhase (void);

    void AdvertiseBlockProposal(enum Messages messageType, rapidjson::Document &d);

     /**
     * processing of received message with block proposal
     * @param message pointer to rapidjson document containing proposed block
      * @param receivedTime double value of received time in seconds (Simulator)
      * @param receivedFrom address of block sender
      */
    void ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom);


    int               m_noMiners;
    uint32_t          m_fixedBlockSize;
    std::default_random_engine m_generator;
    int               m_nextBlockSize;

    double m_blockProposalInterval;
    double m_softVoteInterval;
    double m_certifyVoteInterval;
    std::vector<Block *> receivedBlockProposals;

    EventId m_nextBlockProposalEvent; 				//!< Event to next block proposal
    EventId m_nextSoftVoteEvent; 				    //!< Event to next soft vote
    EventId m_nextCertificationEvent; 				//!< Event to next certify vote

    //debug
    double       m_timeStart;
    double       m_timeFinish;
    enum BlockBroadcastType   m_blockBroadcastType;      //!< the type of broadcast for votes and blocks
};

} // ns3 namespace

#endif //SIMPOS_ALGORAND_PARTICIPANT_H
