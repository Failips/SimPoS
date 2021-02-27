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
#include "../../libsodium/include/sodium.h"


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

    // generation VRF participation secret and public keys for participant
    memset(m_sk, 0, sizeof m_sk);
    memset(m_pk, 0, sizeof m_pk);
    memset(m_vrfProof, 0, sizeof m_vrfProof);
    memset(m_vrfOut, 0, sizeof m_vrfOut);
    crypto_vrf_keypair(m_pk, m_sk);

    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;
    else
        m_nextBlockSize = 0;

    m_blockProposalInterval = 4;
    m_softVoteInterval = 1.5;
    m_certifyVoteInterval = 2.0;

    m_iterationBP = 0;
    m_iterationSV = 0;
    m_iterationCV = 0;
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

void
AlgorandParticipant::SetGenesisVrfSeed(unsigned char *vrfSeed) {
    memset(m_actualVrfSeed, 0, sizeof m_actualVrfSeed);
    memcpy(m_actualVrfSeed, vrfSeed, sizeof m_actualVrfSeed);
}

void AlgorandParticipant::StartApplication() {
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Node START - ID=" << GetNode()->GetId());
    AlgorandNode::StartApplication ();
    NS_LOG_INFO("Node " << GetNode()->GetId() << ": fully started");

    // scheduling algorand events
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
    NS_LOG_INFO("Node " << GetNode()->GetId() << ": scheduled block proposal");
}

void AlgorandParticipant::StopApplication() {
    NS_LOG_FUNCTION(this);
    AlgorandNode::StopApplication ();
    Simulator::Cancel(this->m_nextBlockProposalEvent);
    Simulator::Cancel(this->m_nextSoftVoteEvent);
    Simulator::Cancel(this->m_nextCertificationEvent);

//    NS_LOG_WARN ("\n\nBITCOIN NODE " << GetNode ()->GetId () << ":");
//    NS_LOG_WARN("Total Blocks = " << m_blockchain.GetTotalBlocks());
//    NS_LOG_WARN("longest fork = " << m_blockchain.GetLongestForkSize());
//    NS_LOG_WARN("blocks in forks = " << m_blockchain.GetBlocksInForks());

    std::cout << "\n\nBITCOIN NODE " << GetNode ()->GetId () << ":" << std::endl;
    std::cout << "Total Blocks = " << m_blockchain.GetTotalBlocks() << std::endl;
    std::cout << "longest fork = " << m_blockchain.GetLongestForkSize() << std::endl;
    std::cout << "blocks in forks = " << m_blockchain.GetBlocksInForks() << std::endl;
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
        case CERTIFY_VOTE:
            NS_LOG_INFO ("Certify Vote");
            ProcessReceivedCertifyVote(document, receivedFrom);
            break;
        default:
            NS_LOG_INFO ("Default");
            break;
    }
}

unsigned int
AlgorandParticipant::GenerateVrfSeed() {
    unsigned int vrfSeed = 0;
    while(vrfSeed == 0)
        vrfSeed = m_generator();
    return vrfSeed;
}

void
AlgorandParticipant::SetVrfThresholdBP(unsigned char *threshold) {
    memset(m_vrfThresholdBP, 0, sizeof m_vrfThresholdBP);
    memcpy(m_vrfThresholdBP, threshold, sizeof m_vrfThresholdBP);
}

void
AlgorandParticipant::SetVrfThresholdSV(unsigned char *threshold) {
    memset(m_vrfThresholdSV, 0, sizeof m_vrfThresholdSV);
    memcpy(m_vrfThresholdSV, threshold, sizeof m_vrfThresholdSV);
}

void
AlgorandParticipant::SetVrfThresholdCV(unsigned char *threshold) {
    memset(m_vrfThresholdCV, 0, sizeof m_vrfThresholdCV);
    memcpy(m_vrfThresholdCV, threshold, sizeof m_vrfThresholdCV);
}


void AlgorandParticipant::AdvertiseVoteOrProposal(enum Messages messageType, rapidjson::Document &d){
    NS_LOG_FUNCTION (this);
    int count = 0;
    // sending to each peer in node list
    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i, ++count)
    {
        SendMessage(NO_MESSAGE, messageType, d, m_peersSockets[*i]);
    }
    NS_LOG_INFO(GetNode()->GetId() << " - advertised vote or proposal to " << count << " nodes.");
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
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": BP phase started at " << Simulator::Now().GetSeconds());

    m_iterationBP++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
//    bool chosen = m_helper->IsChosenByVRF(m_iterationBP, participantId, BLOCK_PROPOSAL_PHASE);

    crypto_vrf_prove(m_vrfProof, m_sk, (const unsigned char*) m_actualVrfSeed, sizeof m_actualVrfSeed);
    crypto_vrf_proof_to_hash(m_vrfOut, m_vrfProof);

    // print generated VRF output
//    for(int i=0; i<(sizeof m_vrfOut); ++i)
//        std::cout << std::hex << (int)m_vrfOut[i];
//    std::cout << std::endl;

    int chosenBP = memcmp(m_vrfOut, m_vrfThresholdBP, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen BP: " << chosenBP << " r: " << ((chosenBP <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenBP <= 0) ? true : false;

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
        newBlock.SetVrfSeed(GenerateVrfSeed());
        newBlock.SetParticipantPublicKey(m_pk);
        newBlock.SetVrfOutput(m_vrfOut);

        // Convert block to rapidjson document and broadcast the block
        rapidjson::Document document = newBlock.ToJSON();
        rapidjson::Value value;
        document["type"] = BLOCK;

        value.SetString((const char*) m_vrfProof, 80, document.GetAllocator());
        document.AddMember("vrfProof", value, document.GetAllocator());

        value.SetString((const char*) m_actualVrfSeed, 32, document.GetAllocator());
        document.AddMember("currentSeed", value, document.GetAllocator());

        AdvertiseVoteOrProposal(BLOCK_PROPOSAL, document);
        // inserting to block proposals vector -> in case that we will get this block again we do not advertise it again
        SaveBlockToVector(&m_receivedBlockProposals, m_iterationBP, newBlock);
        NS_LOG_INFO ("Advertised block proposal:  " << newBlock);
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_blockProposalInterval), &AlgorandParticipant::BlockProposalPhase, this);
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_softVoteInterval), &AlgorandParticipant::SoftVotePhase, this);
}

void AlgorandParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    // Converting from rapidjson document message to block object

    // Upper line is commented for correct checking of same blocks
    // (A->B, B->C, C->B => B receives the block from A and B, so block would not be compared as equal even when they are the same block created by A)
    //Block proposedBlock = Block::FromJSON(message, InetSocketAddress::ConvertFrom(receivedFrom).GetIpv4 ());
    Block proposedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));

    // Checking valid VRF output with proof
    int participantId = proposedBlock.GetMinerId();
    int blockIteration = proposedBlock.GetBlockProposalIteration();
    unsigned char pk[32];
    memset(pk, 0, sizeof pk);
    memcpy(pk, proposedBlock.GetParticipantPublicKey(), sizeof pk);
    unsigned char vrfOut[64];
    unsigned char vrfProof[80];
    memset(vrfProof, 0, sizeof vrfProof);
    memcpy(vrfProof, (*message)["vrfProof"].GetString(), sizeof vrfProof);
    unsigned char actualVrfSeed[32];
    memset(actualVrfSeed, 0, sizeof actualVrfSeed);
    memcpy(actualVrfSeed, (*message)["currentSeed"].GetString(), sizeof actualVrfSeed);

    // generate vfrOut for proof and check if it is same as VRF output inside of the received block
    crypto_vrf_verify(vrfOut, pk, vrfProof, (const unsigned char*) actualVrfSeed, sizeof actualVrfSeed);

    int chosenBP = memcmp(vrfOut, m_vrfThresholdBP, sizeof vrfOut);
    bool chosen = (chosenBP <= 0) ? true : false;

    if(!chosen) {
        NS_LOG_INFO(
                "INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIteration
                                                           << "Reason: sender was not chosen");
        return;
    }

    // checking if block contains correct VRF output (this output will be compared to others in Soft vote phase)
    if(memcmp(vrfOut, proposedBlock.GetVrfOutput(), sizeof vrfOut) != 0){
        NS_LOG_INFO(
                "INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIteration
                                                           << "Reason: invalid vrfout");
        return;
    }

    // Inserting received block proposal into vector
    bool inserted = SaveBlockToVector(&m_receivedBlockProposals, blockIteration, proposedBlock);

    // sending only if this is new proposal
    if(inserted){
        NS_LOG_INFO ( "Received new block proposal: " << proposedBlock );
        // Sending to accounts from node list of peers
        AdvertiseVoteOrProposal(BLOCK_PROPOSAL, *message);
    }
}

/** ----------- end of: BLOCK PROPOSAL PHASE ----------- */

/** ----------- SOFT VOTE PHASE ----------- */

void
AlgorandParticipant::SoftVotePhase() {
    NS_LOG_FUNCTION (this);
    m_iterationSV++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
//    bool chosen = m_helper->IsChosenByVRF(m_iterationSV, participantId, SOFT_VOTE_PHASE);

    int chosenSV = memcmp(m_vrfOut, m_vrfThresholdSV, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen SV: " << chosenSV << " r: " << ((chosenSV <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenSV <= 0) ? true : false;
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

        value.SetString((const char*) m_vrfProof, 80, document.GetAllocator());
        document.AddMember("vrfProof", value, document.GetAllocator());
        value.SetString((const char*) m_actualVrfSeed, 32, document.GetAllocator());
        document.AddMember("currentSeed", value, document.GetAllocator());
        value.SetString((const char*) m_pk, 32, document.GetAllocator());
        document.AddMember("vrfPK", value, document.GetAllocator());

        AdvertiseVoteOrProposal(SOFT_VOTE, document);
        NS_LOG_INFO ("Advertised soft vote: " << lowestProposal);
    }

    // Create new certify vote event in m_certifyVoteInterval seconds
    m_nextCertificationEvent = Simulator::Schedule (Seconds(m_certifyVoteInterval), &AlgorandParticipant::CertifyVotePhase, this);
}

Block
AlgorandParticipant::FindLowestProposal(int iteration){
    int lowestIndex = 0;
    unsigned char vrfOut1[64];
    unsigned char vrfOut2[64];
    // iterate through all block proposals for the iteration
    for(auto i = m_receivedBlockProposals.at(iteration - 1).begin(); i != m_receivedBlockProposals.at(iteration - 1).end(); i++){
        memset(vrfOut1, 0, sizeof vrfOut1);
        memset(vrfOut2, 0, sizeof vrfOut2);
        // if the item have lower ID then actualize index
        memcpy(vrfOut1, m_receivedBlockProposals.at(iteration - 1).at(lowestIndex).GetVrfOutput(), sizeof vrfOut1);
        memcpy(vrfOut2, (i)->GetVrfOutput(), sizeof vrfOut2);
        if(memcmp(vrfOut1, vrfOut2, sizeof vrfOut1) > 0) {
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

    unsigned char pk[32];
    memset(pk, 0, sizeof pk);
    memcpy(pk, (*message)["vrfPK"].GetString(), sizeof pk);
    unsigned char vrfOut[64];
    unsigned char vrfProof[80];
    memset(vrfProof, 0, sizeof vrfProof);
    memcpy(vrfProof, (*message)["vrfProof"].GetString(), sizeof vrfProof);
    unsigned char actualVrfSeed[32];
    memset(actualVrfSeed, 0, sizeof actualVrfSeed);
    memcpy(actualVrfSeed, (*message)["currentSeed"].GetString(), sizeof actualVrfSeed);

    // generate vfrOut for proof for proof and check if it is lower than threshold
    crypto_vrf_verify(vrfOut, pk, vrfProof, (const unsigned char*) actualVrfSeed, sizeof actualVrfSeed);

    int chosenSV = memcmp(vrfOut, m_vrfThresholdSV, sizeof vrfOut);
    bool chosen = (chosenSV <= 0) ? true : false;

//    if(!m_helper->IsChosenByVRF(blockIteration, participantId, SOFT_VOTE_PHASE)){
    if(!chosen){
        NS_LOG_INFO ( "INVALID Soft vote - participantId: " << participantId << " block iterationSV: " << blockIteration);
        return;
    }

    // Inserting received block proposal into vector - participantId is indexed from 0 so we increase value to prevent mem errors
    bool inserted = SaveBlockToVector(&m_receivedSoftVotes, participantId + 1, votedBlock);
    if(!inserted)
        return;     // block soft vote from this voter was already received
    NS_LOG_INFO ( "Received soft vote: " << votedBlock );

    // if so, save to pre-tally
    // TODO pri pocitani hlasov brat do uvahy vazeny hlas (podla mnozstva Alga ktore ma ucastnik)
    int totalVotes = SaveBlockToVector(&m_receivedSoftVotePreTally, blockIteration, votedBlock);

    // try to check if has been reached quorum
    int committeeSize = m_helper->GetCommitteeSize(blockIteration, SOFT_VOTE_PHASE);
    if (totalVotes > (2 * (committeeSize / 3))) {
        // if so, save to tally
        SaveBlockToVector(&m_receivedSoftVoteTally, blockIteration, votedBlock);
        NS_LOG_INFO("Quorum reached, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
        NS_LOG_INFO ("Quorum soft vote reached for: " << votedBlock);
    }else {
        NS_LOG_INFO("no SV quorum, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
    }

    AdvertiseVoteOrProposal(SOFT_VOTE, *message);
}

/** ----------- end of: SOFT VOTE PHASE ----------- */

/** ----------- CERTIFY VOTE PHASE ----------- */
void
AlgorandParticipant::CertifyVotePhase() {
    NS_LOG_FUNCTION (this);
    m_iterationCV++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
//    bool chosen = m_helper->IsChosenByVRF(m_iterationCV, participantId, CERTIFY_VOTE_PHASE);

    int chosenCV = memcmp(m_vrfOut, m_vrfThresholdCV, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen CV: " << chosenCV << " r: " << ((chosenCV <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenCV <= 0) ? true : false;

    // check output of VRF and if chosen then vote for lowest VRF proposal
    if (chosen
        && m_receivedSoftVoteTally.size() >= m_iterationCV
        && m_receivedSoftVoteTally.at(m_iterationCV - 1).size() != 0)
    {
        // Check Block Validity
        Block *votedBlock = &(m_receivedSoftVoteTally.at(m_iterationCV - 1).at(0));
        if(IsVotedBlockValid(votedBlock)){

            // Convert valid block to rapidjson document and broadcast the block
            rapidjson::Document document = votedBlock->ToJSON();
            rapidjson::Value value;
            document["type"] = BLOCK;
            value = participantId;
            document.AddMember("voterId", value, document.GetAllocator());
            value = 628;                // TODO ziskat aktualne mnozstvo algo ktore ma dany volic
            document.AddMember("algoAmount", value, document.GetAllocator());

            value.SetString((const char*) m_vrfProof, 80, document.GetAllocator());
            document.AddMember("vrfProof", value, document.GetAllocator());
            value.SetString((const char*) m_actualVrfSeed, 32, document.GetAllocator());
            document.AddMember("currentSeed", value, document.GetAllocator());
            value.SetString((const char*) m_pk, 32, document.GetAllocator());
            document.AddMember("vrfPK", value, document.GetAllocator());

            AdvertiseVoteOrProposal(CERTIFY_VOTE, document);
            NS_LOG_INFO ("Advertised certify vote: " << (*votedBlock));
        }
    }
}

bool
AlgorandParticipant::IsVotedBlockValid(Block *block) {
    // TODO check block validity
    return true;
}

void
AlgorandParticipant::ProcessReceivedCertifyVote(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION (this);

    Block votedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));

    // Checking valid VRF
    int participantId = (*message)["voterId"].GetInt();
    int blockIteration = votedBlock.GetBlockProposalIteration();

    unsigned char pk[32];
    memset(pk, 0, sizeof pk);
    memcpy(pk, (*message)["vrfPK"].GetString(), sizeof pk);
    unsigned char vrfOut[64];
    unsigned char vrfProof[80];
    memset(vrfProof, 0, sizeof vrfProof);
    memcpy(vrfProof, (*message)["vrfProof"].GetString(), sizeof vrfProof);
    unsigned char actualVrfSeed[32];
    memset(actualVrfSeed, 0, sizeof actualVrfSeed);
    memcpy(actualVrfSeed, (*message)["currentSeed"].GetString(), sizeof actualVrfSeed);

    // generate vfrOut for proof for proof and check if it is lower than threshold
    crypto_vrf_verify(vrfOut, pk, vrfProof, (const unsigned char*) actualVrfSeed, sizeof actualVrfSeed);

    int chosenCV = memcmp(vrfOut, m_vrfThresholdCV, sizeof vrfOut);
    bool chosen = (chosenCV <= 0) ? true : false;

//    if(!m_helper->IsChosenByVRF(blockIteration, participantId, CERTIFY_VOTE_PHASE)){
    if(!chosen){
        NS_LOG_INFO ( "INVALID Certify vote - participantId: " << participantId << " block iterationCV: " << blockIteration);
    }

    // Inserting received block proposal into vector - participantId is indexed from 0 so we increase value to prevent mem errors
    bool inserted = SaveBlockToVector(&m_receivedCertifyVotes, participantId + 1, votedBlock);
    if(!inserted)
        return;     // block soft vote from this voter was already received
    NS_LOG_INFO ( "Received certify vote: " << votedBlock );

    // if so, save to pre-tally
    // TODO pri pocitani hlasov brat do uvahy vazeny hlas (podla mnozstva Alga ktore ma ucastnik)
    int totalVotes = SaveBlockToVector(&m_receivedCertifyVotePreTally, blockIteration, votedBlock);

    // try to check if has been reached quorum
    int committeeSize = m_helper->GetCommitteeSize(blockIteration, CERTIFY_VOTE_PHASE);
    if (totalVotes > (2 * (committeeSize / 3))) {
        NS_LOG_INFO("Quorum CV reached, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
        // if so, save to blockchain
        InsertBlockToBlockchain(votedBlock);
//        m_actualVrfSeed = votedBlock.GetVrfSeed();
        NS_LOG_INFO ("Blockchain node inserted (at " << Simulator::Now ().GetSeconds () << "): " << votedBlock);
    }else {
        NS_LOG_INFO("no CV quorum, tv: " << totalVotes << ", committeeSize: " << committeeSize << ", minimum for quorum: " << (2 * (committeeSize / 3)) );
    }

    AdvertiseVoteOrProposal(CERTIFY_VOTE, *message);

}

/** ----------- end of: CERTIFY VOTE PHASE ----------- */

} //  ns3 namespace