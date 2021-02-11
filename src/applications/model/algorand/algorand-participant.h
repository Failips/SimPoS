/**
 * AlgorandParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_ALGORAND_PARTICIPANT_H
#define SIMPOS_ALGORAND_PARTICIPANT_H

#include "ns3/algorand-node.h"
#include "ns3/algorand-participant-helper.h"
#include <random>
#include <utility>

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

    /**
     * set helper object used for common functions like evaluating VRF
     * @param helper Algorand participant helper object
     */
    void SetHelper(ns3::AlgorandParticipantHelper *helper);

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

    void AdvertiseVoteOrProposal(enum Messages messageType, rapidjson::Document &d);

     /**
     * processing of received message with block proposal
     * @param message pointer to rapidjson document containing proposed block
      * @param receivedFrom address of block sender
      */
    void ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom);

    /**
     * Soft vote phase -> choosing members of committee and voting for lowest VRF proposal (block with lowest id)
     */
    void SoftVotePhase (void);

    /**
     * in received proposals for certain iteration finds block proposal with lowest blockId
     * @param iteration soft vote iteration number (also iteration from which block proposal was received)
     * @return block proposal with lowest blockId
     */
    Block FindLowestProposal(int iteration);

    /**
     * saves block into vector (blockProposals, softVoteTally, ...) if vector does not contains it yet
     * if needed, it makes resize of vector to proper size (by iteration number)
     * @param blockVector vector where pointer should be inserted
     * @param iteration phase iteration number
     * @param block pointer on block which we want to save
     * @return true if block has been inserted into vector, false otherwise (block already inside)
     */
    bool SaveBlockToVector(std::vector<std::vector<Block>> *blockVector, int iteration, Block block);

    /**
     * saves pointer on block into vector of maps (softVotePreTally) and increases count of votes
     * if needed, it makes resize of vector to proper size (by iteration number)
     * @param blockVector vector where pointer should be inserted
     * @param iteration phase iteration number
     * @param block pointer on block which we want to save
     * @return count of total votes (for checking if quorum has been reached)
     */
    int SaveBlockToVector(std::vector<std::vector<std::pair<Block, int>>> *blockVector, int iteration, Block block);

    /**
     * processing of received message with soft vote -> verifying voter and check if quorum is reached for certain block
     * @param message pointer to rapidjson document containing proposed block
      * @param receivedFrom address of block sender
     */
    void ProcessReceivedSoftVote(rapidjson::Document *message, Address receivedFrom);

    AlgorandParticipantHelper *m_helper;

    int               m_noMiners;
    uint32_t          m_fixedBlockSize;
    std::default_random_engine m_generator;
    int               m_nextBlockSize;
    int               m_iterationBP;
    int               m_iterationSV;

    double m_blockProposalInterval;
    double m_softVoteInterval;
    double m_certifyVoteInterval;
    std::vector<std::vector<Block>> m_receivedBlockProposals;     // vector of block proposals received in certain iterations
    std::vector<std::vector<Block>> m_receivedSoftVotes;          // vector of soft votes received from certain voters (iteration is replaced with voterId) - in real Algorand voterId would be VRF proof
    std::vector<std::vector<std::pair<Block, int>>> m_receivedSoftVotePreTally; // vector of soft vote in certain iterations, with count of votes
    std::vector<std::vector<Block>> m_receivedSoftVoteTally;      // vector of soft vote in certain iterations with reached quorum

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
