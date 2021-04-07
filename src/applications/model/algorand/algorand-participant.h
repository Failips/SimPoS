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
#include <vector>

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

    /**
     * setting vrf seed from genesis block -> using on global level from AlgorandParticipantHelper
     * @param vrfSeed Algorand genesis block vrf seed value
     */
    void SetGenesisVrfSeed(unsigned char *vrfSeed);

    void SetAllPrint (bool allPrint);

    void SetVrfThresholdBP (unsigned char *threshold);
    void SetVrfThresholdSV (unsigned char *threshold);
    void SetVrfThresholdCV (unsigned char *threshold);

    void SetIntervalBP (double interval);
    void SetIntervalSV (double interval);
    void SetIntervalCV (double interval);

protected:
    // inherited from Application base class.
    virtual void StartApplication (void);    // Called at time specified by Start
    virtual void StopApplication (void);     // Called at time specified by Stop

    virtual void DoDispose (void);

    /**
     * \brief Sends a message to a peer
     * \param receivedMessage the type of the received message
     * \param responseMessage the type of the response message
     * \param d the stringified rapidjson document containing the info of the outgoing message
     * \param outgoingSocket the socket of the peer
     */
    void SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string d, Ptr<Socket> outgoingSocket);

    /**
     * \brief Handle a document received by the application with unknown type number
     * \param document the received document
     */
    virtual void HandleCustomRead (rapidjson::Document *document, double receivedTime, Address receivedFrom);

    /**
     * generates seed for evaluating VRF in next round
     * @return generated seed different from zero (seed != 0)
     */
    unsigned int GenerateVrfSeed();

    /**
     * \brief Block proposal phase -> choosing proposing participants using VRF and sending newly created blocks
     */
    void BlockProposalPhase (void);

    /**
     * sends message of messageType to participants peers
     * @param messageType type of message
     * @param d rapidjson document containing message
     */
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
     * in received proposals for certain iteration finds block proposal with lowest VRF
     * @param iteration soft vote iteration number (also iteration from which block proposal was received)
     * @return pointer on block proposal with lowest VRF
     */
    Block* FindLowestProposal(int iteration);

    /**
     * processing of received message with soft vote -> verifying voter and check if quorum is reached for certain block
     * @param message pointer to rapidjson document containing voted block
      * @param receivedFrom address of block sender
     */
    void ProcessReceivedSoftVote(rapidjson::Document *message, Address receivedFrom);

    /**
     * Certify vote phase -> choosing members of committee, checking validity of block and sending to other participants if valid
     */
    void CertifyVotePhase (void);

    /**
     * Checks the block proposal that was voted on in the soft vote phase for overspending, double-spending, or any other problems
     * @param block pointer on voted block proposal
     * @return true if block is valid, false otherwise
     */
    bool IsVotedBlockValid (Block *block);

    /**
     * processing of received message with certify vote -> verifying voter and check if quorum is reached for certain block,
     * if is quorum reached then we insert block to blockchain (if it is not already inside)
     * @param message pointer to rapidjson document containing voted block
      * @param receivedFrom address of block sender
     */
    void ProcessReceivedCertifyVote(rapidjson::Document *message, Address receivedFrom);

    /**
     * saves block into vector (blockProposals) if vector does not contains it yet
     * if needed, it makes resize of vector to proper size (by iteration number)
     * @param blockVector vector where pointer should be inserted
     * @param iteration phase iteration number
     * @param block block which we want to save
     * @return true if block has been inserted into vector, false otherwise (block already inside)
     */
    bool SaveBlockToVector(std::vector<std::vector<Block>> *blockVector, int iteration, Block block);

    /**
     * saves block into vector (softVoteTally, certifyVoteTally, ...) if vector does not contains it yet
     * if needed, it makes resize of vector to proper size (by iteration number)
     * @param blockVector vector where pointer should be inserted
     * @param iteration phase iteration number
     * @param block pointer on block which we want to save
     * @return true if block has been inserted into vector, false otherwise (block already inside)
     */
    bool SaveBlockToVector(std::vector<std::vector<Block*>> *blockVector, int iteration, Block *block);

    /**
     * saves pointer on block into vector of maps (softVotePreTally) and increases count of votes for this block
     * if needed, it makes resize of vector to proper size (by iteration number)
     * @param blockVector vector where pointer should be inserted
     * @param iteration phase iteration number
     * @param block pointer on block which we want to save
     * @return count of total votes for this block (for checking if quorum has been reached)
     */
    int SaveBlockToVector(std::vector<std::vector<std::pair<Block*, int>>> *blockVector, int iteration, Block *block);

    /**
     * Finds block in vector (blockProposals) and returns pointer to it
     * @param blockVector vector where pointer should be found
     * @param iteration phase iteration number
     * @param blockHash hash of block which we are looking for
     * @return returns pointer on block if block was found, nullptr otherwise
     */
    Block* FindBlockInVector(std::vector<std::vector<Block>> *blockVector, int iteration, std::string blockHash);

    /**
     * increases total count of votes received in the iteration
     * @param counterVector pointer to vector with counters of votes
     * @param iteration phase iteration number
     * @return total count of received votes
     */
    int IncreaseCntOfReceivedVotes(std::vector<int> *counterVector, int iteration);

    /**
     * from preTally vector and votes counter vector returns a pointer on block which reaches quorum in confirmed votes
     * @param phase Algorand phase (soft vote, certify vote) - for decision from which vector we should take values
     * @param iteration phase iteration number
     * @return pointer on confirmed blocks, if none of blocks reaches quorum we return nullptr
     */
    Block* GetConfirmedBlock(AlgorandPhase phase, int iteration);

    /**
     * prints actual state on stderr
     * @param iteration
     */
    void InformAboutState(int iteration);

    void GenNextBlockSize();

    AlgorandParticipantHelper *m_helper;

    unsigned char m_sk[64];         // secret participation key
    unsigned char m_pk[32];         // public participation key
    unsigned char m_vrfOut[64];     // vrf output value (Y)
    unsigned char m_vrfProof[80];   // vrf proof value (p)
    unsigned char m_actualVrfSeed[32]; // VRF seed for current Algorand round (X)

    unsigned char m_vrfThresholdBP[64];         // threshold for Y value (VRF output value) in block proposal phase
    unsigned char m_vrfThresholdSV[64];         // threshold for Y value (VRF output value) in soft vote phase
    unsigned char m_vrfThresholdCV[64];         // threshold for Y value (VRF output value) in certify vote phase

//    unsigned int m_actualVrfSeed;   // VRF seed for current Algorand round (X)

    int               m_noMiners;
    uint32_t          m_fixedBlockSize;
    uint32_t          m_fixedVoteSize;
    std::default_random_engine m_generator;
    bool              m_allPrint;

    int                                            m_nextBlockSize;
    double                                         m_minerAverageBlockSize;
    std::piecewise_constant_distribution<double>   m_blockSizeDistribution;

    int               m_iterationBP;
    int               m_iterationSV;
    int               m_iterationCV;

    // intervals between phases - default value 4 is changed from algorand-test class via algorand participant helpers
    double m_intervalBP = 4;
    double m_intervalSV = 4;
    double m_intervalCV = 4;

    std::vector<std::vector<Block>> m_receivedBlockProposals;     // vector of block proposals received in certain iterations

    std::vector<std::vector<Block*>> m_receivedSoftVotes;          // vector of soft votes received from certain voters (iteration is replaced with voterId) - in real Algorand voterId would be VRF proof
    std::vector<std::vector<std::pair<Block*, int>>> m_receivedSoftVotePreTally; // vector of soft vote in certain iterations, with count of votes
    std::vector<std::vector<Block*>> m_receivedSoftVoteTally;      // vector of soft vote in certain iterations with reached quorum
    std::vector<int> m_softVotes;                                 // vector containing count of votes for each iteration

    std::vector<std::vector<Block*>> m_receivedCertifyVotes;       // vector of certify votes received from certain voters similarly as in soft vote phase
    std::vector<std::vector<std::pair<Block*, int>>> m_receivedCertifyVotePreTally; // vector of certify vote in certain iterations, with count of votes
    std::vector<int> m_certifyVotes;                              // vector containing count of votes for each iteration

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
