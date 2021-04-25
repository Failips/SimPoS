/**
 * CasperParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_CASPER_PARTICIPANT_H
#define SIMPOS_CASPER_PARTICIPANT_H

#include "ns3/casper-node.h"
#include "ns3/bitcoin-miner.h"
#include <utility>
#include <vector>

namespace ns3 {

class Address;
class Socket;
class Packet;

class CasperParticipant : public BitcoinMiner {
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

    void SetEpochSize(int epochSize);

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
     * \brief replacement of parent mining event with empty method
     */
    virtual void MineBlock (void);

    /**
     * \brief Called for blocks with better score(height). Replacement of parent event with empty method
     * \param newBlock the new block which was received
     */
    virtual void ReceivedHigherBlock(const Block &newBlock);

    /**
     * \brief Handle a document received by the application with unknown type number
     * \param document the received document
     */
    virtual void HandleCustomRead (rapidjson::Document *document, double receivedTime, Address receivedFrom);

    /**
     * advertising vote to node peers
     * @param messageType type of message
     * @param d rapidjson document containing message
     * @param doNotSendTo pointer to address of peer to which we should not send the message, default nullptr (send to all)
     */
    void AdvertiseVote(enum Messages messageType, rapidjson::Document &d, Address *doNotSendTo = nullptr);

    /**
     * handling insertion of block to blockchain to count blocks in epoch and know when to vote
     * @param newBlock new block to insert
     */
    virtual void InsertBlockToBlockchain(Block &newBlock);

    /**
     * finds best link between checkpoints (the one with highest score)
     * @return pair containing pointers to lower and higher checkpoint, which should be connected
     */
    std::pair<const Block*, const Block*> FindBestLink();


    /**
     * sum up votes inf previous epoch and change states of checkpoints (if finality reached, even blocks)
     */
    virtual void TallyingAndBlockchainUpdate();

    /**
     * finds best option for vote and sends vote to its peers
     */
    void Vote();

    /**
    * processing of received message with casper participant vote
    * @param message pointer to rapidjson document containing vote
     * @param receivedFrom address of vote sender
     */
    void ProcessReceivedCasperVote(rapidjson::Document *message, Address receivedFrom);

    /**
     * inserts vote to vote buffer if it is not already inside
     * @param vote rapidjson document containing the vote
     * @return true if vote was inserted, false if vote is already in buffer
     */
    bool SaveVoteToBuffer(rapidjson::Document *vote);

    /**
     * prints actual state on stderr
     */
    void InformAboutState();

    /**
     * send request for missing block to peers
     * @param missingBlockHash hash of requested missing block
     * @param doNotSendTo pointer to address of peer to which we should not send the message, default nullptr (send to all)
     */
    void SendRequestForMissingBlock(std::string missingBlockHash, Address *doNotSendTo = nullptr);

    /**
    * processing of received message with request for missing block
    * @param message pointer to rapidjson document containing vote
     * @param receivedFrom address of vote sender
     */
    void ProcessReceivedRequestForMissingBlock(rapidjson::Document *message, Address receivedFrom);

    /**
    * processing of received message with missing block
    * @param message pointer to rapidjson document containing vote
     * @param receivedFrom address of vote sender
     */
    void ProcessReceivedMissingBlock(rapidjson::Document *message, Address receivedFrom);

    /**
     * updates blockchain when quorum for supermajority link was reached
     * @param source source checkpoint of link
     * @param target target checkpoint of link
     */
    void UpdateBlockchain(std::string source, std::string target);

    void GenNextBlockSize();

    unsigned char m_sk[64];         // secret participation key
    unsigned char m_pk[32];         // public participation key

    uint32_t          m_fixedVoteSize;

    int m_maxBlocksInEpoch;                             // maximum blocks in one epoch (Ethereum is using 50 blocks)
    int m_currentEpoch;                                 // number of actual epoch in which is participant voting
    std::pair<int, int> m_lastFinalized;                // info about last finalized checkpoint (block height and minerId)
    std::vector<std::map<int, std::string>> m_votes;    // buffer containing votes for each epoch (map key is voter id, value is serialized vote JSON)

    std::vector<std::pair<std::string, Address>> m_requestsForBlocks;   // buffer containing requests for missing block (missing block has, peer who sent request)
    std::vector<std::pair<std::string, std::string>> m_unprocessedSupermajorityLinks;   // buffer containing links which reach quorum, but one of blocks were missing (source hash, target hash)

    //debug
    double       m_timeStart;
    double       m_timeFinish;
    enum BlockBroadcastType   m_blockBroadcastType;      //!< the type of broadcast for votes and blocks
};

} // ns3 namespace

#endif //SIMPOS_CASPER_PARTICIPANT_H
