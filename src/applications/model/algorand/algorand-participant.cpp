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
            .AddAttribute ("NumberOfMiners",
                           "The number of miners",
                           UintegerValue (16),
                           MakeUintegerAccessor (&AlgorandParticipant::m_noMiners),
                           MakeUintegerChecker<uint32_t> ())
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
//    m_nextSoftVoteEvent = Simulator::Schedule (Seconds(m_receiveBlockWindow), &AlgorandParticipant::SoftVotePhase, this);
//    m_nextCertificationEvent = Simulator::Schedule (Seconds(m_receiveBlockWindow), &AlgorandParticipant::CertifyBlockPhase, this);
}

void AlgorandParticipant::StopApplication() {
    NS_LOG_FUNCTION(this);
    AlgorandNode::StopApplication ();
    Simulator::Cancel(this->m_nextBlockProposalEvent);
//    Simulator::Cancel(this->m_nextSoftVoteEvent);
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
//        case SOFT_VOTE:
//            NS_LOG_INFO ("Default");
//            this->ReceiveSoftVote(document);
//            break;
//        case CERTIFY_VOTE:
//            NS_LOG_INFO ("Default");
//            this->ReceiveCertifyVote(document);
//            break;
        default:
            NS_LOG_INFO ("Default");
            break;
    }
}

/** ----------- BLOCK PROPOSAL PHASE ----------- */

void AlgorandParticipant::BlockProposalPhase() {
    NS_LOG_FUNCTION (this);
    m_iterationBP++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
    bool chosen = m_helper->IsChosenByVRF(m_iterationBP, participantId, BLOCK_PROPOSAL_PHASE);
    NS_LOG_INFO ("Chosen: " << chosen);

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

        AdvertiseBlockProposal(BLOCK_PROPOSAL, document);
        NS_LOG_INFO ("Broadcasted block proposal: " << newBlock);
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
}

void AlgorandParticipant::AdvertiseBlockProposal(enum Messages messageType, rapidjson::Document &d){
    NS_LOG_FUNCTION (this);
    int count = 0;
    // sending to each peer in node list
    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
    {
        SendMessage(NO_MESSAGE, messageType, d, m_peersSockets[*i]);
    }
}

void AlgorandParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    // Converting from rapidjson document message to block object

    // Upper line is commented for correct checking of same blocks
    // (A->B, B->C, C->B => B receives the block from A and B, so block would not be compared as equal even when they are the same block created by A)
    //Block proposedBlock = Block::FromJSON(message, InetSocketAddress::ConvertFrom(receivedFrom).GetIpv4 ());
    Block proposedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));
    NS_LOG_INFO ( "Received block proposal: " << proposedBlock );

    // check if our vector of proposals have place for blocks in received iteration
    int blockIterationBP = proposedBlock.GetBlockProposalIteration();
    if(m_receivedBlockProposals.size() < blockIterationBP)
        m_receivedBlockProposals.resize(blockIterationBP);

    // Checking already received block proposal
    for(auto block: m_receivedBlockProposals.at(blockIterationBP - 1))
        if(proposedBlock == (*block))
            return;

    // Checking valid VRF
    int participantId = proposedBlock.GetMinerId();
    if(m_helper->IsChosenByVRF(blockIterationBP, participantId, BLOCK_PROPOSAL_PHASE)){
        // Saving new block proposal
        m_receivedBlockProposals.at(blockIterationBP - 1).push_back(&proposedBlock);
        // Sending to accounts from node list of peers
        AdvertiseBlockProposal(BLOCK_PROPOSAL, *message);
    }else{
        NS_LOG_INFO ( "INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIterationBP);
    }
}

/** ----------- end of: BLOCK PROPOSAL PHASE ----------- */

/** ----------- SOFT VOTE PHASE ----------- */




/** ----------- end of: SOFT VOTE PHASE ----------- */

} //  ns3 namespace