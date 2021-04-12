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
#include "ns3/double.h"
#include "ns3/simulator.h"
#include <algorithm>
#include <utility>
#include "../../libsodium/include/sodium.h"
#include <iomanip>
#include <sys/time.h>

static double GetWallTime();

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
                            "ns3::Packet::AddressTracedCallback")
            .AddAttribute ("Cryptocurrency",
                           "BITCOIN, LITECOIN, DOGECOIN, ALGORAND, GASPER, CASPER",
                           UintegerValue (ALGORAND),
                           MakeUintegerAccessor (&AlgorandParticipant::m_cryptocurrency),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("FixedStakeSize",
                           "The fixed size of the stake of committee member",
                           UintegerValue (0),
                           MakeUintegerAccessor (&AlgorandParticipant::m_fixedStakeSize),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("IsAttacker",
                           "Set Voter to be an attacker",
                           BooleanValue (false),
                           MakeBooleanAccessor (&AlgorandParticipant::m_isAttacker),
                           MakeBooleanChecker ())
            .AddAttribute ("IsFailed",
                           "Set participant to be a failed state",
                           BooleanValue (false),
                           MakeBooleanAccessor (&AlgorandParticipant::m_isFailed),
                           MakeBooleanChecker ())
            .AddAttribute ("AttackPower",
                           "Wanted attack power (attacker stake : total stakes) of the attackers vote",
                           DoubleValue (0.3),
                           MakeDoubleAccessor (&AlgorandParticipant::m_attackPower),
                           MakeDoubleChecker<double> ())
                           ;
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

    m_chosenToSVCommitteeTimes = 0;
    m_averageStakeSize = 0;

    if (m_fixedStakeSize > 0)
        m_nextStakeSize = m_fixedStakeSize;
    else
        m_nextStakeSize = 0;

    m_fixedVoteSize = 256; // size of vote in Bytes

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

    m_nodeStats->miner = 1;
    m_nodeStats->isFailed = m_isFailed;
    m_nodeStats->isAttacker = m_isAttacker;
    m_nodeStats->voteSentBytes = 0;
    m_nodeStats->voteReceivedBytes = 0;
    m_nodeStats->meanBPCommitteeSize = 0;
    m_nodeStats->meanSVCommitteeSize = 0;
    m_nodeStats->meanCVCommitteeSize = 0;
    m_nodeStats->meanSVStakeSize = 0;
    m_nodeStats->countSVCommitteeMember = 0;
    m_nodeStats->successfulInsertions = 0;
    m_nodeStats->successfulInsertionBlocks = 0;

    AlgorandNode::StartApplication ();

    if(m_isFailed){
        return;
    }

    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;

    std::array<double, 201> intervals{0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95,
                                          100, 105, 110, 115, 120, 125,
                                          130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200,
                                          205, 210, 215, 220, 225, 230, 235,
                                          240, 245, 250, 255, 260, 265, 270, 275, 280, 285, 290, 295, 300, 305, 310,
                                          315, 320, 325, 330, 335, 340, 345,
                                          350, 355, 360, 365, 370, 375, 380, 385, 390, 395, 400, 405, 410, 415, 420,
                                          425, 430, 435, 440, 445, 450, 455,
                                          460, 465, 470, 475, 480, 485, 490, 495, 500, 505, 510, 515, 520, 525, 530,
                                          535, 540, 545, 550, 555, 560, 565,
                                          570, 575, 580, 585, 590, 595, 600, 605, 610, 615, 620, 625, 630, 635, 640,
                                          645, 650, 655, 660, 665, 670, 675,
                                          680, 685, 690, 695, 700, 705, 710, 715, 720, 725, 730, 735, 740, 745, 750,
                                          755, 760, 765, 770, 775, 780, 785,
                                          790, 795, 800, 805, 810, 815, 820, 825, 830, 835, 840, 845, 850, 855, 860,
                                          865, 870, 875, 880, 885, 890, 895,
                                          900, 905, 910, 915, 920, 925, 930, 935, 940, 945, 950, 955, 960, 965, 970,
                                          975, 980, 985, 990, 995, 1000};
        std::array<double, 200> weights{4.96, 0.21, 0.17, 0.25, 0.27, 0.3, 0.34, 0.26, 0.26, 0.33, 0.35, 0.49, 0.42,
                                        0.42, 0.48, 0.41, 0.46, 0.45,
                                        0.58, 0.58, 0.57, 0.52, 0.54, 0.47, 0.53, 0.56, 0.5, 0.48, 0.53, 0.54, 0.49,
                                        0.51, 0.56, 0.53, 0.56, 0.5,
                                        0.47, 0.45, 0.52, 0.43, 0.46, 0.47, 0.6, 0.53, 0.42, 0.48, 0.55, 0.49, 0.63,
                                        2.38, 0.47, 0.53, 0.43, 0.51,
                                        0.44, 0.46, 0.44, 0.41, 0.47, 0.46, 0.45, 0.37, 0.49, 0.4, 0.41, 0.41, 0.41,
                                        0.37, 0.43, 0.47, 0.48, 0.37,
                                        0.4, 0.46, 0.34, 0.35, 0.37, 0.36, 0.37, 0.31, 0.35, 0.39, 0.34, 0.38, 0.29,
                                        0.41, 0.37, 0.34, 0.36, 0.34,
                                        0.29, 0.3, 0.36, 0.26, 0.29, 0.31, 0.3, 0.29, 0.35, 0.5, 0.28, 0.37, 0.31, 0.33,
                                        0.32, 0.28, 0.34, 0.31,
                                        0.26, 0.24, 0.22, 0.25, 0.24, 0.25, 0.26, 0.25, 0.24, 0.33, 0.24, 0.23, 0.2,
                                        0.24, 0.26, 0.27, 0.27, 0.21,
                                        0.22, 0.3, 0.25, 0.21, 0.26, 0.21, 0.21, 0.21, 0.23, 0.48, 0.2, 0.19, 0.21, 0.2,
                                        0.17, 0.19, 0.21, 0.22,
                                        0.24, 0.25, 0.23, 0.31, 0.46, 8.32, 0.22, 0.11, 0.13, 0.17, 0.12, 0.16, 0.15,
                                        0.16, 0.19, 0.21, 0.18, 0.24,
                                        0.19, 0.2, 0.16, 0.17, 0.19, 0.17, 0.22, 0.33, 0.17, 0.22, 0.25, 0.19, 0.2,
                                        0.17, 0.28, 0.25, 0.24, 0.25, 0.3,
                                        0.34, 0.46, 0.49, 0.67, 3.13, 2.94, 0.14, 0.36, 3.88, 0.07, 0.11, 0.11, 0.11,
                                        0.26, 0.12, 0.13, 0.88, 5.84, 4.11};
        m_blockSizeDistribution = std::piecewise_constant_distribution<double>(intervals.begin(), intervals.end(),
                                                                               weights.begin());


    NS_LOG_INFO("Node " << GetNode()->GetId() << ": fully started");

    // scheduling algorand events
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_intervalCV), &AlgorandParticipant::BlockProposalPhase, this);
    NS_LOG_INFO("Node " << GetNode()->GetId() << ": scheduled block proposal");
}

void AlgorandParticipant::GenNextBlockSize()
{
    if (m_fixedBlockSize > 0)
        m_nextBlockSize = m_fixedBlockSize;
    else
    {
        m_nextBlockSize = m_blockSizeDistribution(m_generator) * 1000;	// *1000 because the m_blockSizeDistribution returns KBytes
    }

    if (m_nextBlockSize < m_averageTransactionSize)
        m_nextBlockSize = m_averageTransactionSize + m_headersSizeBytes;
}

void AlgorandParticipant::GenNextStakeSize(AlgorandPhase phase) {
    if (m_fixedStakeSize > 0)
        m_nextStakeSize = m_fixedStakeSize;
    else
    {
        m_nextStakeSize = m_blockSizeDistribution(m_generator);	    // the m_blockSizeDistribution returns KBytes so we can use it also here
        m_nextStakeSize = m_nextStakeSize % 10000;  // this is for fixing generated values like 1068247285 instead of something in interval <0;10000>
    }

    if(m_isAttacker
    && phase == SOFT_VOTE_PHASE
    && m_chosenToSVCommitteeTimes != 0
    && m_attackPower != 0.0){
        int lastStake = m_mySoftVoteStakes.at(m_mySoftVoteStakes.size()-1);
        int lastTotal = m_softVotes.at(m_softVotes.size() -1);

        if(lastStake == lastTotal)
            m_nextStakeSize = lastStake;
        else
            m_nextStakeSize = (int) ceil(((lastTotal - lastStake) / (1-m_attackPower)) * m_attackPower);

        NS_LOG_INFO(GetNode()->GetId() << " - last SV stake: " << lastStake
            << " (" << std::setprecision (2) << (((double )lastStake/(double )lastTotal) * 100)
            <<" % of total: "<< lastTotal<< ") | next SV stake: " << m_nextStakeSize);
    }

}

void AlgorandParticipant::StopApplication() {
    NS_LOG_FUNCTION(this);
    AlgorandNode::StopApplication ();
    Simulator::Cancel(this->m_nextBlockProposalEvent);
    Simulator::Cancel(this->m_nextSoftVoteEvent);
    Simulator::Cancel(this->m_nextCertificationEvent);

    // update statistics
    m_nodeStats->meanBPCommitteeSize = GetAvgCommitteeSize(BLOCK_PROPOSAL_PHASE);
    m_nodeStats->meanSVCommitteeSize = GetAvgCommitteeSize(SOFT_VOTE_PHASE);
    m_nodeStats->meanCVCommitteeSize = GetAvgCommitteeSize(CERTIFY_VOTE_PHASE);
    m_nodeStats->meanSVStakeSize = m_averageStakeSize;
    m_nodeStats->isAttacker = m_isAttacker;
    m_nodeStats->isFailed = m_isFailed;
    m_nodeStats->countSVCommitteeMember = m_chosenToSVCommitteeTimes;
    m_nodeStats->successfulInsertions = m_successfulInsertions;
    m_nodeStats->successfulInsertionBlocks = m_successfulInsertionBlocks;


    if(m_allPrint || GetNode()->GetId() == 0) {
        std::cout << "\n\nALGORAND NODE " << GetNode()->GetId() << ":" << std::endl;
        std::cout << "Total Blocks = " << m_blockchain.GetTotalBlocks() << std::endl;
        std::cout << "longest fork = " << m_blockchain.GetLongestForkSize() << std::endl;
        std::cout << "blocks in forks = " << m_blockchain.GetBlocksInForks() << std::endl;
        std::cout << "avg BP committee size = " << m_nodeStats->meanBPCommitteeSize << std::endl;
        std::cout << "avg SV committee size = " << m_nodeStats->meanSVCommitteeSize << std::endl;
        std::cout << "avg CV committee size = " << m_nodeStats->meanCVCommitteeSize << std::endl;
        std::cout << "chosen to SV committee (times) = " << m_chosenToSVCommitteeTimes << std::endl;
        std::cout << "avg SV stake size = " << m_averageStakeSize << std::endl;
        std::cout << "failed = " << (m_isFailed ? "true" : "false") << std::endl;
        if(m_isAttacker) {
            std::cout << "attacker = " << (m_isAttacker ? "true" : "false") << std::endl;
            std::cout << "successfulAttacks = " << (m_successfulInsertions + m_successfulInsertionBlocks) << std::endl;
            std::cout << "successfulInsertions = " << (m_successfulInsertions) << std::endl;
            std::cout << "successfulInsertionBlocks = " << (m_successfulInsertionBlocks) << std::endl;
        }

        std::cout << "\n\n" << m_blockchain << std::endl;
    }
}

void AlgorandParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    AlgorandNode::DoDispose ();
}

int AlgorandParticipant::GetAvgCommitteeSize(AlgorandPhase phase) {
    long total = 0;
    int count = 0;

    if(phase == BLOCK_PROPOSAL_PHASE){
        for(auto proposals: m_receivedBlockProposals){
            total += proposals.size();
            count++;
        }
    }else if(phase == SOFT_VOTE_PHASE){
        for(auto votes: m_softVoters){
            total += votes;
            count++;
        }
    }else if(phase == CERTIFY_VOTE_PHASE){
        for(auto votes: m_certifyVoters){
            total += votes;
            count++;
        }
    }else{
        return 0;
    }


    if(count == 0 )
        return 0;

    double res = (count - 1) / static_cast<double>(count) * 0
                 + static_cast<double>(total) / (count);

    return res;
}

void AlgorandParticipant::HandleCustomRead(rapidjson::Document *document, double receivedTime, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    switch ((*document)["message"].GetInt()) {
        case BLOCK_PROPOSAL:
            NS_LOG_INFO (GetNode()->GetId() << " - Block Proposal");
            ProcessReceivedProposedBlock(document, receivedFrom);
            break;
        case SOFT_VOTE:
            NS_LOG_INFO (GetNode()->GetId() << " - Soft Vote");
            ProcessReceivedSoftVote(document, receivedFrom);
            break;
        case CERTIFY_VOTE:
            NS_LOG_INFO (GetNode()->GetId() << " - Certify Vote");
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

void
AlgorandParticipant::SetAllPrint(bool allPrint)
{
    m_allPrint = allPrint;
}

void
AlgorandParticipant::SetIsAttacker(bool isAttacker) {
    m_isAttacker = isAttacker;
}

void
AlgorandParticipant::SetIntervalBP(double interval) {
    m_intervalBP = interval;
}

void
AlgorandParticipant::SetIntervalSV(double interval) {
    m_intervalSV = interval;
}

void
AlgorandParticipant::SetIntervalCV(double interval) {
    m_intervalCV = interval;
}


void AlgorandParticipant::AdvertiseVoteOrProposal(enum Messages messageType, rapidjson::Document &d, Address *doNotSendTo){
    NS_LOG_FUNCTION (this);
    int count = 0;
    double sendTime;

    // create json string buffer
    rapidjson::StringBuffer jsonBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonBuffer);
    d.Accept(writer);
    std::string msg = jsonBuffer.GetString();

    // sending to each peer in node list
    for (std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
    {
        if(doNotSendTo && (InetSocketAddress::ConvertFrom(*doNotSendTo).GetIpv4 () == (*i))) {
            NS_LOG_INFO(GetNode()->GetId() << " - Skipping: " << (*i));
            continue;
        }

        switch(messageType){
            case BLOCK_PROPOSAL: {
                long blockSize = (d)["size"].GetInt();
                sendTime = blockSize / m_uploadSpeed;
                m_nodeStats->blockSentBytes += blockSize;
                break;
            }
            case SOFT_VOTE: {
                sendTime = m_fixedVoteSize / m_uploadSpeed;
                m_nodeStats->voteSentBytes += m_fixedVoteSize;
                break;
            }
            case CERTIFY_VOTE: {
                sendTime = m_fixedVoteSize / m_uploadSpeed;
                m_nodeStats->voteSentBytes += m_fixedVoteSize;
                break;
            }
        }

        count++;
        Simulator::Schedule (Seconds(sendTime), &AlgorandParticipant::SendMessage, this, NO_MESSAGE, messageType, msg, m_peersSockets[*i]);
    }

    NS_LOG_INFO(GetNode()->GetId() << " - advertised vote or proposal to " << count << " nodes.");
}

void
AlgorandParticipant::SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string d, Ptr<Socket> outgoingSocket){
    rapidjson::Document msg;
    msg.Parse(d.c_str());

    BitcoinNode::SendMessage(receivedMessage, responseMessage, msg, outgoingSocket, "^#EOM#^");
    NS_LOG_INFO(GetNode()->GetId() << " - Message sent, respMsg: " << getMessageName(responseMessage));
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

bool
AlgorandParticipant::SaveBlockToVector(std::vector<std::vector<Block*>> *blockVector, int iteration, Block *block) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(blockVector->size() < iteration)
        blockVector->resize(iteration);

    // Checking already saved block
    for(auto b: blockVector->at(iteration - 1))
        if((*block) == (*b) && block->GetBlockProposalIteration() == b->GetBlockProposalIteration())
            return false;

    // Inserting block to vector
    blockVector->at(iteration - 1).push_back(block);
    return true;
}

int
AlgorandParticipant::SaveBlockToVector(std::vector<std::vector<std::pair<Block*, int>>> *blockVector, int iteration, Block *block, int algoAmount) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(blockVector->size() < iteration)
        blockVector->resize(iteration);

    // Checking already saved block
    bool found = false;
    std::vector<std::pair<Block*, int>>::iterator m;


    for (m = blockVector->at(iteration - 1).begin(); m != blockVector->at(iteration - 1).end(); m++)
    {
        // if block already in vector of votes, than increase number of votes by one
        if ((*block) == (*(m->first))) {
            m->second = m->second + algoAmount;
            NS_LOG_INFO(GetNode()->GetId() << " - SBV, s: "  << blockVector->at(iteration - 1).size() << ", i: "  << iteration << ", stake: " << algoAmount << ", total for the block: " << m->second);
            return m->second;
        }
    }

    // Inserting block to vector because it was not found in vector
    blockVector->at(iteration - 1).push_back(std::make_pair(block, algoAmount));
    NS_LOG_INFO(GetNode()->GetId() << " - SBV, s: "  << blockVector->at(iteration - 1).size() << ", i: "  << iteration << ", stake: " << algoAmount << ", total for the block: " << algoAmount);
    return algoAmount;   // count of total votes is algoAmount for newly inserted vote
}

Block*
AlgorandParticipant::FindBlockInVector(std::vector<std::vector<Block>> *blockVector, int iteration, std::string blockHash) {
    if(blockVector->size() < iteration)
        return nullptr;

    for(auto i = blockVector->at(iteration - 1).begin(); i != blockVector->at(iteration - 1).end(); i++){
        if(i->GetBlockHash() == blockHash)
            return &(*i);
    }

    // block was not found
    return nullptr;
}

int
AlgorandParticipant::IncreaseCntOfReceivedVotes(std::vector <int> *counterVector, int iteration, int value) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(counterVector->size() < iteration) {
        counterVector->resize(iteration);
        counterVector->at(iteration-1) = 0;
    }

    // increase count of votes and return actual count
    counterVector->at(iteration-1) = counterVector->at(iteration-1) + value;
    return counterVector->at(iteration-1);
}

Block*
AlgorandParticipant::GetConfirmedBlock(AlgorandPhase phase, int iteration) {
    std::vector<std::pair<Block*, int>> *preTally;
    std::vector<int> *voters;
    int totalVotes = 0;

    if(iteration == 0)
        return nullptr;

    if(phase == SOFT_VOTE_PHASE){
        if(m_receivedSoftVotePreTally.size() < iteration || m_softVoters.size() < iteration)
            return nullptr;
        preTally = &(m_receivedSoftVotePreTally.at(iteration-1));
        voters = &m_softVoters;
        totalVotes = m_softVotes.at(iteration-1);
    }else if(phase == CERTIFY_VOTE_PHASE){
        if(m_receivedCertifyVotePreTally.size() < iteration || m_certifyVoters.size() < iteration)
            return nullptr;
        preTally = &(m_receivedCertifyVotePreTally.at(iteration-1));
        voters = &m_certifyVoters;
        totalVotes = m_certifyVotes.at(iteration-1);
    }else{
        return nullptr;
    }

    // we need at least 3 voters for reaching quorum
    if(voters->size() < iteration || voters->at(iteration-1) < 3)
        return nullptr;

    // searching for block which reached quorum
    std::vector<std::pair<Block*, int>>::iterator m;
    for(m = preTally->begin(); m != preTally->end(); m++){
        // try to check if has been reached quorum
        if (m->second > (2 * (totalVotes / 3))) {
            // if so, save to tally
            NS_LOG_INFO(GetNode()->GetId() << " - Quorum " << phase << " in iteration " << iteration << " reached, tv: " << m->second
                << ", totalStakes: " << totalVotes << ", committeeSize: " << voters->at(iteration-1)
                <<", minimum for quorum: " << ((2 * (totalVotes / 3))+1) );
            NS_LOG_INFO (GetNode()->GetId() << " - Quorum " << phase << " in iteration " << iteration << " reached for: " << (*(m->first)));
            return (m->first);
        }
    }
    return nullptr;
}

void
AlgorandParticipant::InformAboutState(int iteration) {
    if(GetNode()->GetId() == 1){
        std::cerr << '\r'
            << std::setw(10) << std::setfill('0') << Simulator::Now().GetSeconds() << ':'
            << std::setw(10) << iteration << ':'
            << std::setw(10) << m_blockchain.GetTotalBlocks() << ':'
            << std::setw(10) << m_blockchain.GetLongestForkSize() << ':'
            << std::setw(10) << m_blockchain.GetBlocksInForks() << ':'
            << std::flush;
    }
}

/** ----------- BLOCK PROPOSAL PHASE ----------- */

void AlgorandParticipant::BlockProposalPhase() {
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": BP phase started at " << Simulator::Now().GetSeconds());

    // evaluate certified block vote from previous iteration and save to blockchain
    Block* votedBlock = GetConfirmedBlock(CERTIFY_VOTE_PHASE, m_iterationBP);
    if(votedBlock) {
        InsertBlockToBlockchain(*votedBlock);
        if(m_isAttacker && m_chosenToSVCommitteeTimes != 0 && *votedBlock == m_maliciousBlock){
            // if voted block is the malicious block
            m_successfulInsertions++;
        }
    }else if(m_isAttacker
            && m_iterationBP != 0
            && m_mySoftVoteStakes.size() >= m_iterationBP
            && m_mySoftVoteStakes.at(m_iterationBP-1) != 0){
        // or if no block was chosen in last iteration and attacker was chosen to the committee
        m_successfulInsertionBlocks++;
    }

    // ------ start real block proposal ------
    m_iterationBP++;    // increase number of block proposal iterations
//    InformAboutState(m_iterationBP);  // print state to stderr
    int participantId = GetNode()->GetId();

    crypto_vrf_prove(m_vrfProof, m_sk, (const unsigned char*) m_actualVrfSeed, sizeof m_actualVrfSeed);
    crypto_vrf_proof_to_hash(m_vrfOut, m_vrfProof);

    // print generated VRF output
//    for(int i=0; i<(sizeof m_vrfOut); ++i)
//        std::cout << std::hex << (int)m_vrfOut[i];
//    std::cout << std::endl;

    int chosenBP = memcmp(m_vrfOut, m_vrfThresholdBP, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen BP ("<<m_iterationBP<<"): " << chosenBP << " r: " << ((chosenBP <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenBP <= 0) ? true : false;

    // check output of VRF and if chosen then create and send
    if(chosen){
        // Extract info from blockchain
        const Block* prevBlock = m_blockchain.GetCurrentTopBlock();
        int height =  prevBlock->GetBlockHeight() + 1;
        int parentBlockParticipantId = prevBlock->GetMinerId();
        double currentTime = Simulator::Now ().GetSeconds ();

        // Create new block from out information
        GenNextBlockSize();
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
        NS_LOG_INFO (GetNode()->GetId() << " - Advertised block proposal:  " << newBlock);
    }

    // Create new block proposal event in m_blockProposalInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_intervalBP), &AlgorandParticipant::SoftVotePhase, this);
}

void AlgorandParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    // Converting from rapidjson document message to block object

    // FIXED - Upper line is commented for correct checking of same blocks
    // (A->B, B->C, C->B => B receives the block from A and B, so block would not be compared as equal even when they are the same block created by A)
    Block proposedBlock = Block::FromJSON(message, InetSocketAddress::ConvertFrom(receivedFrom).GetIpv4 ());
//    Block proposedBlock = Block::FromJSON(message, Ipv4Address("0.0.0.0"));
    double currentTime = Simulator::Now ().GetSeconds ();
    proposedBlock.SetTimeReceived(currentTime);

    // update statistics
    m_nodeStats->blockReceivedBytes += proposedBlock.GetBlockSizeBytes();

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
                GetNode()->GetId() << " - INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIteration
                                                           << "Reason: sender was not chosen");
        return;
    }

    // checking if block contains correct VRF output (this output will be compared to others in Soft vote phase)
    if(memcmp(vrfOut, proposedBlock.GetVrfOutput(), sizeof vrfOut) != 0){
        NS_LOG_INFO(
                GetNode()->GetId() << " - INVALID Block proposal - participantId: " << participantId << " block iterationBP: " << blockIteration
                                                           << "Reason: invalid vrfout");
        return;
    }

    // Inserting received block proposal into vector
    bool inserted = SaveBlockToVector(&m_receivedBlockProposals, blockIteration, proposedBlock);

    // sending only if this is new proposal
    if(inserted){
        NS_LOG_INFO ( GetNode()->GetId() << " - Received new block proposal: " << proposedBlock );
        // Sending to accounts from node list of peers
        AdvertiseVoteOrProposal(BLOCK_PROPOSAL, *message, &receivedFrom);
    }else{
        NS_LOG_INFO (GetNode()->GetId() << " - Already received block proposal - participantId: " << participantId << ", iterationBP: " << blockIteration);
    }
}

/** ----------- end of: BLOCK PROPOSAL PHASE ----------- */

/** ----------- SOFT VOTE PHASE ----------- */

void
AlgorandParticipant::SoftVotePhase() {
    NS_LOG_FUNCTION (this);
    m_iterationSV++;    // increase number of block proposal iterations
    int participantId = GetNode()->GetId();
    int stake = 0;
//    bool chosen = m_helper->IsChosenByVRF(m_iterationSV, participantId, SOFT_VOTE_PHASE);

    int chosenSV = memcmp(m_vrfOut, m_vrfThresholdSV, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen SV ("<<m_iterationSV<<"): " << chosenSV << " r: " << ((chosenSV <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenSV <= 0) ? true : false;
    // check output of VRF and if chosen then vote for lowest VRF proposal
    if (chosen
        && m_receivedBlockProposals.size() >= m_iterationSV
        && m_receivedBlockProposals.at(m_iterationSV - 1).size() != 0)
    {
        // Find lowest block
        Block *lowestProposal;
        if(!m_isAttacker) {
            lowestProposal = FindLowestProposal(m_iterationSV);
        }else{
            lowestProposal = GetMaliciousBlock(m_iterationSV);
            if(lowestProposal)
                m_maliciousBlock = *lowestProposal;
        }

        if(lowestProposal) {
            // generate next stake size
            GenNextStakeSize(SOFT_VOTE_PHASE);

            // Extract values from block to rapidjson document vote and broadcast the vote
            rapidjson::Document document;
            rapidjson::Value value;
            document.SetObject();

            value = SOFT_VOTE;
            document.AddMember("message", value, document.GetAllocator());

            std::string blockHash = lowestProposal->GetBlockHash();
            value.SetString(blockHash.c_str(), blockHash.size(), document.GetAllocator());
            document.AddMember("blockHash", value, document.GetAllocator());
            value = lowestProposal->GetBlockProposalIteration();
            document.AddMember("blockIteration", value, document.GetAllocator());
            value = participantId;
            document.AddMember("voterId", value, document.GetAllocator());
            NS_LOG_INFO(GetNode()->GetId() << " - sendAlgo: " << m_nextStakeSize);
            value = m_nextStakeSize;
            document.AddMember("algoAmount", value, document.GetAllocator());

            value.SetString((const char *) m_vrfProof, 80, document.GetAllocator());
            document.AddMember("vrfProof", value, document.GetAllocator());
            value.SetString((const char *) m_actualVrfSeed, 32, document.GetAllocator());
            document.AddMember("currentSeed", value, document.GetAllocator());
            value.SetString((const char *) m_pk, 32, document.GetAllocator());
            document.AddMember("vrfPK", value, document.GetAllocator());

            AdvertiseVoteOrProposal(SOFT_VOTE, document);
            NS_LOG_INFO(GetNode()->GetId() << " - Advertised soft vote("<<m_iterationSV<<"): " << (*lowestProposal));
            SaveBlockToVector(&m_receivedSoftVotes, participantId + 1, lowestProposal);

            // save also to pretally
            SaveBlockToVector(&m_receivedSoftVotePreTally, m_iterationSV, lowestProposal, m_nextStakeSize);
            int tot = IncreaseCntOfReceivedVotes(&m_softVotes, m_iterationSV, m_nextStakeSize);
            NS_LOG_INFO (GetNode()->GetId() << " - Total Votes SV("<<m_iterationSV<<"): " << tot);

            // increase count of voters in the iterations
            IncreaseCntOfReceivedVotes(&m_softVoters, m_iterationSV);

            stake = m_nextStakeSize;
            // statistics update
            m_averageStakeSize = m_chosenToSVCommitteeTimes / static_cast<double>(m_chosenToSVCommitteeTimes + 1) *
                                 m_averageStakeSize
                                 + static_cast<double>(m_nextStakeSize) / (m_chosenToSVCommitteeTimes + 1);
            m_chosenToSVCommitteeTimes++;
        }
    }

    if(m_isAttacker) {
        // save attackers stakes in each iteration
        IncreaseCntOfReceivedVotes(&m_mySoftVoteStakes, m_iterationSV, stake);
    }
    // Create new certify vote event in m_certifyVoteInterval seconds
    m_nextCertificationEvent = Simulator::Schedule (Seconds(m_intervalSV), &AlgorandParticipant::CertifyVotePhase, this);
}

Block*
AlgorandParticipant::FindLowestProposal(int iteration){
    int lowestIndex = 0;
    unsigned char vrfOut1[64];
    unsigned char vrfOut2[64];
    // iterate through all block proposals for the iteration
    for(auto i = m_receivedBlockProposals.at(iteration - 1).begin(); i != m_receivedBlockProposals.at(iteration - 1).end(); i++){
        // checking block validity first
        if(!IsVotedBlockValid(&(*i)))
            continue;

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
    return &(m_receivedBlockProposals.at(iteration - 1).at(lowestIndex));
}

Block*
AlgorandParticipant::GetMaliciousBlock(int iteration) {
    int highestIndex = 0;
    unsigned char vrfOut1[64];
    unsigned char vrfOut2[64];
    if (m_receivedBlockProposals.at(iteration - 1).size() < 2) {
        NS_LOG_INFO(GetNode()->GetId() << " - Too low number of proposals, total: " << m_receivedBlockProposals.at(iteration - 1).size());
        return nullptr;
    }

    // iterate through all block proposals for the iteration
    for(auto i = m_receivedBlockProposals.at(iteration - 1).begin(); i != m_receivedBlockProposals.at(iteration - 1).end(); i++){
        // checking block validity first
        if(!IsVotedBlockValid(&(*i)))
            continue;

        memset(vrfOut1, 0, sizeof vrfOut1);
        memset(vrfOut2, 0, sizeof vrfOut2);
        // if the item have lower ID then actualize index
        memcpy(vrfOut1, m_receivedBlockProposals.at(iteration - 1).at(highestIndex).GetVrfOutput(), sizeof vrfOut1);
        memcpy(vrfOut2, (i)->GetVrfOutput(), sizeof vrfOut2);
        if(memcmp(vrfOut1, vrfOut2, sizeof vrfOut1) < 0) {
            highestIndex = std::distance(m_receivedBlockProposals.at(iteration - 1).begin(), i);;
        }
    }
    // return block with lowest blockId
    return &(m_receivedBlockProposals.at(iteration - 1).at(highestIndex));
}

void
AlgorandParticipant::ProcessReceivedSoftVote(rapidjson::Document *message, ns3::Address receivedFrom) {
    NS_LOG_FUNCTION (this);

    // Checking valid VRF
    std::string blockHash = (*message)["blockHash"].GetString();
    int blockIteration = (*message)["blockIteration"].GetInt();
    int participantId = (*message)["voterId"].GetInt();

    // update statistics
    m_nodeStats->voteReceivedBytes += m_fixedVoteSize;

    Block *votedBlock = FindBlockInVector(&m_receivedBlockProposals, blockIteration, blockHash);
    if(votedBlock == nullptr) {
        // block proposal was not found
        NS_LOG_INFO (GetNode()->GetId() << " - block proposal was not found");
        return;
    }

    // parsing received VRF values
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
        NS_LOG_INFO (GetNode()->GetId() << " - INVALID Soft vote - participantId: " << participantId << ", iterationSV: " << blockIteration);
        return;
    }

    // Inserting received block proposal into vector - participantId is indexed from 0 so we increase value to prevent mem errors
    bool inserted = SaveBlockToVector(&m_receivedSoftVotes, participantId + 1, votedBlock);
    if(!inserted) {
        NS_LOG_INFO (GetNode()->GetId() << " - Already received Soft vote - participantId: " << participantId << ", iterationSV: " << blockIteration);
        return;     // block soft vote from this voter was already received
    }
    NS_LOG_INFO (GetNode()->GetId() << " - Received soft vote (i: "<< blockIteration <<", voter: "<< participantId <<"): " << (*votedBlock) );

    // advertise vote to other next peers
    AdvertiseVoteOrProposal(SOFT_VOTE, *message, &receivedFrom);

    // and save to pre-tally
    int algoAmount = (int) (*message)["algoAmount"].GetUint();
    SaveBlockToVector(&m_receivedSoftVotePreTally, blockIteration, votedBlock, algoAmount);

    // count total vote stakes
    int tot = IncreaseCntOfReceivedVotes(&m_softVotes, blockIteration, algoAmount);
    NS_LOG_INFO (GetNode()->GetId() << " - Total Votes SV("<<m_iterationSV<<"): " << tot);

    // increase count of voters in the iterations
    IncreaseCntOfReceivedVotes(&m_softVoters, blockIteration);
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
    NS_LOG_INFO ( participantId << " - Chosen CV ("<<m_iterationCV<<"): " << chosenCV << " r: " << ((chosenCV <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenCV <= 0) ? true : false;

    // check output of VRF and if chosen then vote for lowest VRF proposal
    if (chosen
        && m_receivedSoftVotePreTally.size() >= m_iterationCV
        && m_receivedSoftVotePreTally.at(m_iterationCV - 1).size() != 0)
    {
        // Get block with most votes
        Block *votedBlock = GetConfirmedBlock(SOFT_VOTE_PHASE, m_iterationCV);
        if(votedBlock)
            NS_LOG_INFO ( participantId << " - CV Voted block: " << (*votedBlock));
        else
            NS_LOG_INFO ( participantId << " - CV Voted block: nullptr");

        // Check Block Validity
        if(votedBlock && IsVotedBlockValid(votedBlock)){
            // generate next stake size
            GenNextStakeSize(CERTIFY_VOTE_PHASE);

            // Convert valid block to rapidjson document and broadcast the block
            rapidjson::Document document;
            rapidjson::Value value;
            document.SetObject();

            value = CERTIFY_VOTE;
            document.AddMember("message", value, document.GetAllocator());

            std::string blockHash = votedBlock->GetBlockHash();
            value.SetString(blockHash.c_str(), blockHash.size(), document.GetAllocator());
            document.AddMember("blockHash", value, document.GetAllocator());
            value = votedBlock->GetBlockProposalIteration();
            document.AddMember("blockIteration", value, document.GetAllocator());
            value = participantId;
            document.AddMember("voterId", value, document.GetAllocator());
            value = m_nextStakeSize;
            document.AddMember("algoAmount", value, document.GetAllocator());

            value.SetString((const char*) m_vrfProof, 80, document.GetAllocator());
            document.AddMember("vrfProof", value, document.GetAllocator());
            value.SetString((const char*) m_actualVrfSeed, 32, document.GetAllocator());
            document.AddMember("currentSeed", value, document.GetAllocator());
            value.SetString((const char*) m_pk, 32, document.GetAllocator());
            document.AddMember("vrfPK", value, document.GetAllocator());

            AdvertiseVoteOrProposal(CERTIFY_VOTE, document);
            NS_LOG_INFO ("Advertised certify vote: " << (*votedBlock));
            SaveBlockToVector(&m_receivedCertifyVotes, participantId + 1, votedBlock);

            // save also to pre-tally
            SaveBlockToVector(&m_receivedCertifyVotePreTally, m_iterationCV, votedBlock, m_nextStakeSize);
            int tot = IncreaseCntOfReceivedVotes(&m_certifyVotes, m_iterationCV, m_nextStakeSize);
            NS_LOG_INFO (GetNode()->GetId() << " - Total Votes CV("<<m_iterationCV<<"): " << tot);

            // increase count of voters in the iterations
            IncreaseCntOfReceivedVotes(&m_certifyVoters, m_iterationCV);
        }
    }
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_intervalCV), &AlgorandParticipant::BlockProposalPhase, this);
}

bool
AlgorandParticipant::IsVotedBlockValid(Block *block) {
    // checking only validity of position in blockchain
    const Block* currentTopBlock = m_blockchain.GetCurrentTopBlock();

    if(block->GetBlockHeight() - 1 == currentTopBlock->GetBlockHeight())
        return true;

    return false;
}

void
AlgorandParticipant::ProcessReceivedCertifyVote(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION (this);

    // Checking valid VRF
    std::string blockHash = (*message)["blockHash"].GetString();
    int blockIteration = (*message)["blockIteration"].GetInt();
    int participantId = (*message)["voterId"].GetInt();

    // update statistics
    m_nodeStats->voteReceivedBytes += m_fixedVoteSize;

    Block *votedBlock = FindBlockInVector(&m_receivedBlockProposals, blockIteration, blockHash);
    if(votedBlock == nullptr)
        // block proposal was not found
        return;

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
        NS_LOG_INFO (GetNode()->GetId() << " - INVALID Certify vote - participantId: " << participantId << " block iterationCV: " << blockIteration);
        return;
    }

    // Inserting received block proposal into vector - participantId is indexed from 0 so we increase value to prevent mem errors
    bool inserted = SaveBlockToVector(&m_receivedCertifyVotes, participantId + 1, votedBlock);
    if(!inserted) {
        NS_LOG_INFO (GetNode()->GetId() << " - Already received certify vote - participantId: " << participantId << ", iterationCV: " << blockIteration);
        return;     // block soft vote from this voter was already received
    }
    NS_LOG_INFO (GetNode()->GetId() << " - Received certify vote (i: "<< blockIteration <<", voter: "<< participantId <<"): " << (*votedBlock) );

    // if so, save to pre-tally
    int algoAmount = (int) (*message)["algoAmount"].GetUint();
    SaveBlockToVector(&m_receivedCertifyVotePreTally, blockIteration, votedBlock, algoAmount);
    int tot = IncreaseCntOfReceivedVotes(&m_certifyVotes, blockIteration, algoAmount);
    NS_LOG_INFO (GetNode()->GetId() << " - Total Votes CV("<<m_iterationCV<<"): " << tot);

    AdvertiseVoteOrProposal(CERTIFY_VOTE, *message, &receivedFrom);

    // increase count of voters in the iterations
    IncreaseCntOfReceivedVotes(&m_certifyVoters, blockIteration);
}

/** ----------- end of: CERTIFY VOTE PHASE ----------- */

} //  ns3 namespace



static double GetWallTime()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}