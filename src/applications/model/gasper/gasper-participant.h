/**
 * GasperParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_GASPER_PARTICIPANT_H
#define SIMPOS_GASPER_PARTICIPANT_H

#include "ns3/gasper-node.h"
#include "ns3/gasper-participant-helper.h"
#include <random>
#include <utility>
#include <vector>

namespace ns3 {

class Address;
class Socket;
class Packet;

class GasperParticipant : public GasperNode {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    GasperParticipant ();

    virtual ~GasperParticipant (void);

    /**
     * \brief set the type of block (vote) broadcast
     */
    void SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType);

    /**
     * set helper object used for common functions like evaluating VRF
     * @param helper Gasper participant helper object
     */
    void SetHelper(ns3::GasperParticipantHelper *helper);

    /**
     * setting vrf seed from genesis block -> using on global level from GasperParticipantHelper
     * @param vrfSeed Gasper genesis block vrf seed value
     */
    void SetGenesisVrfSeed(unsigned char *vrfSeed);

    void SetAllPrint (bool allPrint);

    void SetVrfThresholdBP (unsigned char *threshold);
    void SetVrfThreshold (unsigned char *threshold);

    void SetIntervalBP (double interval);
    void SetIntervalAttest (double interval);

    void SetEpochSize (int epochSize);

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
     * Attest HLMD phase -> choosing members of committee and voting for block with best HLMD score
     */
    void AttestHlmdPhase (void);

    /**
     * processing of received message with attest -> verifying voter and save vote to buffer
     * @param message pointer to rapidjson document containing voted block
      * @param receivedFrom address of block sender
     */
    void ProcessReceivedAttest(rapidjson::Document *message, Address receivedFrom);

    /**
     * finds best block using Hybrid LMD Ghost rule
     * @param iteration iteration (also called as slot) in which we are looking for attests
     * @return pointer on block which is best according to the participant (committee voter)
     */
    const Block* EvalHLMDBlock (int iteration);

    /**
     * between children finds the block with most followers
     * @param children pointer to vector containing pointer to direct children of some block
     * @param iteration iteration (also called as slot) in which we are looking for attests
     * @return pointer to the block with most followers
     */
    const Block* FindArgMax (const std::vector<const Block *> *children, int iteration);

    /**
     * finds best link between checkpoints (the one with highest score based on Hybrid LMD rule)
     * @param attestedBlock pointer on block which is best according to the participant (committee voter)
     * @return pair containing pointers to lower and higher checkpoint, which should be connected
     */
    std::pair<const Block*, const Block*> FindBestLink(const Block* attestedBlock);

    /**
     * in received proposals for certain iteration finds block proposal with lowest VRF
     * @param iteration soft vote iteration number (also iteration from which block proposal was received)
     * @return pointer on block proposal with lowest VRF
     */
    Block* FindLowestProposal(int iteration);

    /**
     * Checks the block proposal that was voted on in the soft vote phase for overspending, double-spending, or any other problems
     * @param block pointer on voted block proposal
     * @return true if block is valid, false otherwise
     */
    bool IsVotedBlockValid (Block *block);

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
     * extracts attest from vote and inserts attest to vote buffer containing attests if it is not already inside
     * @param vote rapidjson document containing the vote (attest + ffg vote)
     * @return true if vote was inserted, false if vote is already in buffer
     */
    bool SaveVoteToAttestBuffer(rapidjson::Document *vote);

    /**
     * extracts FFG vote from vote and inserts FFG vote to vote buffer containing FFG votes if it is not already inside
     * @param vote rapidjson document containing the vote (attest + ffg vote)
     * @return true if vote was inserted, false if vote is already in buffer
     */
    bool SaveVoteToFFGBuffer(rapidjson::Document *vote);

    /**
     * handling insertion of block to blockchain to count blocks in epoch and know when to vote
     * @param newBlock new block to insert
     */
    virtual void InsertBlockToBlockchain(Block &newBlock);

    /**
     * sum up votes inf previous epoch and change states of checkpoints (if finality reached, even blocks)
     */
    virtual void TallyingAndBlockchainUpdate();

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
     * prints actual state on stderr
     * @param iteration
     */
    void InformAboutState(int iteration);

    /**
     * updates value of m_nextBlockSize based on value of m_fixedBlockSize
     */
    void GenNextBlockSize();

    /**
     * updates value of m_nextStakeSize based on value of m_fixedStakeSize
     */
    void GenNextStakeSize();

    /**
     * counts average committee size from received attests
     * @return average committee size
     */
    int GetAvgCommitteeSize();

    GasperParticipantHelper *m_helper;

    unsigned char m_sk[64];         // secret participation key
    unsigned char m_pk[32];         // public participation key
    unsigned char m_vrfOut[64];     // vrf output value (Y)
    unsigned char m_vrfProof[80];   // vrf proof value (p)
    unsigned char m_actualVrfSeed[32]; // VRF seed for current Gasper round (X)

    unsigned char m_vrfThresholdBP[64];         // threshold for Y value (VRF output value) in choosing leader of committee
    unsigned char m_vrfThreshold[64];           // threshold for Y value (VRF output value) in choosing committee members

//    unsigned int m_actualVrfSeed;   // VRF seed for current Gasper round (X)

    int               m_noMiners;
    uint32_t          m_fixedBlockSize;
    uint32_t          m_fixedVoteSize;
    std::default_random_engine m_generator;
    bool              m_allPrint;

    uint32_t          m_fixedStakeSize;
    int               m_nextStakeSize;
    double            m_averageStakeSize;
    int               m_chosenToCommitteeTimes;     // count of times when the participant was chosen to committee

    int                                            m_nextBlockSize;
    double                                         m_minerAverageBlockSize;
    std::piecewise_constant_distribution<double>   m_blockSizeDistribution;

    int               m_iterationBP;
    int               m_iterationAttest;

    // intervals between phases - default value 4 is changed from gasper-test class via gasper participant helpers
    double m_intervalBP = 4;            // Interval between block proposal phase and attest phase
    double m_intervalAttest = 4;        // Interval between attest phase and block proposal phase

    std::vector<std::vector<Block>> m_receivedBlockProposals;     // vector of block proposals received in certain iterations

    std::vector<std::map<int, std::string>> m_votes;              // buffer containing FFG votes for each epoch (map key is voter id, value is serialized vote JSON)
    std::vector<std::map<int, std::pair<const Block*, int>>> m_receivedAttests;        // buffer containing attests for each slot/bpIteration (map key is voter id, value is Block pointer from vote and amount of stake)

    EventId m_nextBlockProposalEvent; 				//!< Event to next block proposal
    EventId m_nextAttestEvent; 				        //!< Event to next attest voting

    // Casper variables
    int m_maxBlocksInEpoch;                             // maximum blocks in one epoch (Ethereum is using 50 blocks)
    int m_currentEpoch;                                 // number of actual epoch in which is participant voting
    std::pair<int, int> m_lastFinalized;                // info about last finalized checkpoint (block height and minerId)

    //debug
    double       m_timeStart;
    double       m_timeFinish;
    enum BlockBroadcastType   m_blockBroadcastType;      //!< the type of broadcast for votes and blocks
};

} // ns3 namespace

#endif //SIMPOS_GASPER_PARTICIPANT_H
