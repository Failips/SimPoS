/**
* Implementation of AlgorandParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "algorand-participant.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include <algorithm>
#include <utility>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AlgorandParticipant");

NS_OBJECT_ENSURE_REGISTERED (AlgorandParticipant);

TypeId
AlgorandParticipant::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::AlgorandParticipant")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<AlgorandParticipant>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&AlgorandParticipant::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&AlgorandParticipant::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("BlockTorrent",
                          "Enable the BlockTorrent protocol",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AlgorandParticipant::m_blockTorrent),
                          MakeBooleanChecker())
            .AddAttribute ("FixedBlockSize",
                           "The fixed size of the block",
                           UintegerValue (0),
                           MakeUintegerAccessor (&AlgorandParticipant::m_fixedBlockSize),
                           MakeUintegerChecker<uint32_t> ())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&AlgorandParticipant::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback");
    return tid;
}

AlgorandParticipant::AlgorandParticipant() : AlgorandNode(),
                               m_timeStart(0), m_timeFinish(0) {
    NS_LOG_FUNCTION(this);
    std::random_device rd;
    m_generator.seed(rd());

    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;
    else
        m_nextBlockSize = 0;

    m_blockProposalInterval = 5.0;
    m_softVoteInterval = m_blockProposalInterval + 1.0;
    m_certifyVoteInterval = m_blockProposalInterval + 2.5;

    m_iterationBP = 0;
    m_iterationSV = 0;
}

AlgorandParticipant::~AlgorandParticipant(void) {
    NS_LOG_FUNCTION(this);
}

void
AlgorandParticipant::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    NS_LOG_FUNCTION (this);
    m_blockBroadcastType = blockBroadcastType;
}

void
AlgorandParticipant::SetHelper(ns3::AlgorandParticipantHelper *helper) {
    NS_LOG_FUNCTION (this);
    m_helper = helper;
}


void AlgorandParticipant::StartApplication() {
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Node START - ID=" << GetNode()->GetId());
    AlgorandNode::StartApplication ();

    // scheduling algorand events
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
    m_nextSoftVoteEvent = Simulator::Schedule (Seconds(m_softVoteInterval), &AlgorandParticipant::SoftVotePhase, this);
//    m_nextCertificationEvent = Simulator::Schedule (Seconds(m_receiveBlockWindow), &AlgorandParticipant::CertifyBlockPhase, this);
}

void AlgorandParticipant::StopApplication() {
    NS_LOG_FUNCTION(this);
    AlgorandNode::StopApplication ();
    Simulator::Cancel(this->m_nextBlockProposalEvent);
    Simulator::Cancel(this->m_nextSoftVoteEvent);
//    Simulator::Cancel(this->m_nextCertificationEvent);
}

void AlgorandParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    AlgorandNode::DoDispose ();
}

void AlgorandParticipant::HandleCustomRead(rapidjson::Document *document, double receivedTime, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    switch ((*document)["message"].GetInt()) {
        case BLOCK_PROPOSAL:
            NS_LOG_INFO ("Block Proposal");
            ProcessReceivedProposedBlock(document, receivedFrom);
            break;
        case SOFT_VOTE:
            NS_LOG_INFO ("Soft Vote");
            ProcessReceivedSoftVote(document, receivedFrom);
            break;
//        case CERTIFY_VOTE:
//            NS_LOG_INFO ("Default");
//            this->ReceiveCertifyVote(document);
//            break;
        default:
            NS_LOG_INFO ("Default");
            break;
    }
}

void AlgorandParticipant::AdvertiseVoteOrProposal(enum Messages messageType, rapidjson::Document &d){
    NS_LOG_FUNCTION (this);
    int count = 0;
    // sending to each peer in node list
    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
    {
        SendMessage(NO_MESSAGE, messageType, d, m_peersSockets[*i]);
    }
}

bool
AlgorandParticipant::SaveBlockToVector(std::vector<std::vector<Block>> *blockVector, int iteration, Block block) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(blockVector->size() < iteration)
        blockVector->resize(iteration);

    // Checking already saved block
    for(auto b: blockVector->at(iteration - 1))
        if((block) == (b))
            return false;

    // Inserting block to vector
    blockVector->at(iteration - 1).push_back(block);
    return true;
}

int
AlgorandParticipant::SaveBlockToVector(std::vector<std::vector<std::pair<Block, int>>> *blockVector, int iteration, Block block) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(blockVector->size() < iteration)
        blockVector->resize(iteration);

    // Checking already saved block
    bool found = false;
    std::vector<std::pair<Block, int>>::iterator m;

    for (m = blockVector->at(iteration - 1).begin(); m != blockVector->at(iteration - 1).end(); m++)
    {
        // if block already in vector of votes, than increase number of votes by one
        if ((block) == ((m->first))) {
            m->second = m->second + 1;
            return m->second;
        }
    }

    // Inserting block to vector because it was not found in vector
    blockVector->at(iteration - 1).push_back(std::make_pair(block, 1));
    return 1;   // count of total votes is 1 for newly inserted vote
}

/** ----------- BLOCK PROPOSAL PHASE ----------- */

void AlgorandParticipant::BlockProposalPhase() {
    NS_LOG_FUNCTION (this);
    m_iterationBP++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
    bool chosen = m_helper->IsChosenByVRF(m_iterationBP, participantId, BLOCK_PROPOSAL_PHASE);
    NS_LOG_INFO ("Chosen BP: " << chosen);

    // check output of VRF and if chosen then create and send
    if(chosen){
        // Extract info from blockchain
        const Block* prevBlock = m_blockchain.GetCurrentTopBlock();
        int height =  prevBlock->GetBlockHeight() + 1;
        int parentBlockParticipantId = prevBlock->GetMinerId();
        double currentTime = Simulator::Now ().GetSeconds ();

        // Create new block from out information
        Block newBlock (height, participantId, parentBlockParticipantId, m_nextBlockSize,
                        currentTime, currentTime, Ipv4Address("127.0.0.1"));
        newBlock.SetBlockProposalIteration(m_iterationBP);

        // Convert block to rapidjson document and broadcast the block
        rapidjson::Document document = newBlock.ToJSON();
        document["type"] = BLOCK;

        AdvertiseVoteOrProposal(BLOCK_PROPOSAL, document);
        NS_LOG_INFO ("Advertised block proposal: " << newBlock);
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
}

void AlgorandParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    // Converting from rapidjson document message to block object

    // Upper line is commented for correct checking of same blocks
    // (A->B, B->C, C->B => B receives the block from A and B, so block would not be compared as equal even when they are the same block created by A)
    //Block proposedBlock = Block::FromJSON(message, InetSocketAddress::ConvertFrom(receivedFrom).GetIpv4 ());
    Block proposedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));

    // Checking valid VRF
    int participantId = proposedBlock.GetMinerId();
    int blockIteration = proposedBlock.GetBlockProposalIteration();
    if(m_helper->IsChosenByVRF(blockIteration, participantId, BLOCK_PROPOSAL_PHASE)){
        // Inserting received block proposal into vector
        bool inserted = SaveBlockToVector(&m_receivedBlockProposals, blockIteration, proposedBlock);

        // sending only if this is new proposal
        if(inserted){
            NS_LOG_INFO ( "Received new block proposal: " << proposedBlock );
            // Sending to accounts from node list of peers
            AdvertiseVoteOrProposal(BLOCK_PROPOSAL, *message);
        }
    }else{
        NS_LOG_INFO ( "INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIteration);
    }
}

/** ----------- end of: BLOCK PROPOSAL PHASE ----------- */

/** ----------- SOFT VOTE PHASE ----------- */

void
AlgorandParticipant::SoftVotePhase() {
    NS_LOG_FUNCTION (this);
    m_iterationSV++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
    bool chosen = m_helper->IsChosenByVRF(m_iterationSV, participantId, SOFT_VOTE_PHASE);

    // check output of VRF and if chosen then vote for lowest VRF proposal
    if (chosen
        && m_receivedBlockProposals.size() >= m_iterationSV
        && m_receivedBlockProposals.at(m_iterationSV - 1).size() != 0)
    {
        // Find lowest block
        Block lowestProposal = FindLowestProposal(m_iterationSV);

        // Convert block to rapidjson document and broadcast the block
        rapidjson::Document document = lowestProposal.ToJSON();
        rapidjson::Value value;
        document["type"] = BLOCK;
        value = participantId;
        document.AddMember("voterId", value, document.GetAllocator());
        value = 628;                // TODO ziskat aktualne mnozstvo algo ktore ma dany volic
        document.AddMember("algoAmount", value, document.GetAllocator());

        AdvertiseVoteOrProposal(SOFT_VOTE, document);
        NS_LOG_INFO ("Advertised soft vote: " << lowestProposal);
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_softVoteInterval), &AlgorandParticipant::SoftVotePhase, this);
}

Block
AlgorandParticipant::FindLowestProposal(int iteration){
    int lowestIndex = 0;
    // iterate through all block proposals for the iteration
    for(auto i = m_receivedBlockProposals.at(iteration - 1).begin(); i != m_receivedBlockProposals.at(iteration - 1).end(); i++){
        // if the item have lower ID then actualize index
        if(m_receivedBlockProposals.at(iteration - 1).at(lowestIndex).GetBlockId() > (i)->GetBlockId()) {
            lowestIndex = std::distance(m_receivedBlockProposals.at(iteration - 1).begin(), i);;
        }
    }
    // return block with lowest blockId
    return m_receivedBlockProposals.at(iteration - 1).at(lowestIndex);
}

void
AlgorandParticipant::ProcessReceivedSoftVote(rapidjson::Document *message, ns3::Address receivedFrom) {
    NS_LOG_FUNCTION (this);

    Block votedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));

    // Checking valid VRF
    int participantId = (*message)["voterId"].GetInt();
    int blockIteration = votedBlock.GetBlockProposalIteration();

    if(!m_helper->IsChosenByVRF(blockIteration, participantId, SOFT_VOTE_PHASE)){
        NS_LOG_INFO ( "INVALID Soft vote - participantId: " << participantId << " block iterationSV: " << blockIteration);
    }

    // Inserting received block proposal into vector - participantId is indexed from 0 so we increase value to prevent mem errors
    bool inserted = SaveBlockToVector(&m_receivedSoftVotes, participantId + 1, votedBlock);
    if(!inserted)
        return;     // block soft vote from this voter was already received
    NS_LOG_INFO ( "Received soft vote: " << votedBlock );

    // if so, save to pre-tally
    int totalVotes = SaveBlockToVector(&m_receivedSoftVotePreTally, blockIteration, votedBlock);

    // try to check if has been reached quorum
    int committeeSize = m_helper->GetCommitteeSize(blockIteration, SOFT_VOTE_PHASE);
    if (totalVotes > (2 * (committeeSize / 3))) {
        // if so, save to tally
        SaveBlockToVector(&m_receivedSoftVoteTally, blockIteration, votedBlock);
        NS_LOG_INFO("Quorum reached, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
        NS_LOG_INFO ("Quorum soft vote reached for: " << votedBlock);
    }else {
        NS_LOG_INFO("no quorum, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
    }

    AdvertiseVoteOrProposal(SOFT_VOTE, *message);
}

/** ----------- end of: SOFT VOTE PHASE ----------- */

/** ----------- CERTIFY VOTE PHASE ----------- */

/** ----------- end of: CERTIFY VOTE PHASE ----------- */

} //  ns3 namespace