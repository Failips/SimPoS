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


void AlgorandParticipant::StartApplication() {
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Node START - ID=" << GetNode()->GetId());
    AlgorandNode::StartApplication ();

    // scheduling algorand events
    this->m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
//    this->m_nextSoftVoteEvent = Simulator::Schedule (Seconds(m_receiveBlockWindow), &AlgorandParticipant::SoftVotePhase, this);
//    this->m_nextCertificationEvent = Simulator::Schedule (Seconds(m_receiveBlockWindow), &AlgorandParticipant::CertifyBlockPhase, this);
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

void AlgorandParticipant::BlockProposalPhase() {
    bool chosen = true; // todo output of VRF

    // check output of VRF and if chosen then create and send
    if(chosen){
        // Extract info from blockchain and create new block
        const Block* prevBlock = m_blockchain.GetCurrentTopBlock();
        int height =  prevBlock->GetBlockHeight() + 1;
        int participantId = GetNode()->GetId();
        int parentBlockParticipantId = prevBlock->GetMinerId();
        double currentTime = Simulator::Now ().GetSeconds ();

        Block newBlock (height, participantId, parentBlockParticipantId, m_nextBlockSize,
                        currentTime, currentTime, Ipv4Address("127.0.0.1"));

        // Convert block to rapidjson document and broadcast the block
        rapidjson::Document document = newBlock.ToJSON();
        document["type"] = BLOCK;

        AdvertiseBlockProposal(BLOCK_PROPOSAL, document);
        std::clog << "Broadcasted block proposal: " << newBlock << std::endl;
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    this->m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
}

void AlgorandParticipant::AdvertiseBlockProposal(enum Messages messageType, rapidjson::Document &d){
    int count = 0;
    // sending to each peer in node list
    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
    {
        SendMessage(NO_MESSAGE, messageType, d, m_peersSockets[*i]);
    }
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

void AlgorandParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    // Converting from rapidjson document message to block object

    // Upper line is commented for correct checking of same blocks
    // (A->B, B->C, C->B => B receives the block from A and B, so block would not be compared as equal even when they are the same block created by A)
    //Block proposedBlock = Block::FromJSON(message, InetSocketAddress::ConvertFrom(receivedFrom).GetIpv4 ());
    Block proposedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));
    std::clog << "Received block proposal: " << proposedBlock << std::endl;

    // Checking already received block proposal
    for(auto block: this->receivedBlockProposals)
        if(proposedBlock == (*block))
            return;

    // TODO check VRF ok

    // Saving new block proposal
    this->receivedBlockProposals.push_back(&proposedBlock);
    // Sending to accounts from node list of peers
    AdvertiseBlockProposal(BLOCK_PROPOSAL, *message);
}



} //  ns3 namespace