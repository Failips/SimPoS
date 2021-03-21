/**
* Implementation of GasperParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "gasper-participant.h"
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
#include <iomanip>
#include <sys/time.h>

static double GetWallTime();

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GasperParticipant");

NS_OBJECT_ENSURE_REGISTERED (GasperParticipant);

TypeId
GasperParticipant::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::GasperParticipant")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<GasperParticipant>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&GasperParticipant::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&GasperParticipant::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("BlockTorrent",
                          "Enable the BlockTorrent protocol",
                          BooleanValue(false),
                          MakeBooleanAccessor(&GasperParticipant::m_blockTorrent),
                          MakeBooleanChecker())
            .AddAttribute ("FixedBlockSize",
                           "The fixed size of the block",
                           UintegerValue (0),
                           MakeUintegerAccessor (&GasperParticipant::m_fixedBlockSize),
                           MakeUintegerChecker<uint32_t> ())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&GasperParticipant::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback");
    return tid;
}

GasperParticipant::GasperParticipant() : GasperNode(),
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

    m_iterationBP = 0;
    m_iterationAttest = 0;

    m_currentEpoch = 1;
    m_maxBlocksInEpoch = 64;
    m_lastFinalized = std::make_pair(0, -1);    // last finalized is genesis block
}

GasperParticipant::~GasperParticipant(void) {
    NS_LOG_FUNCTION(this);
}

void
GasperParticipant::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    NS_LOG_FUNCTION (this);
    m_blockBroadcastType = blockBroadcastType;
}

void
GasperParticipant::SetHelper(ns3::GasperParticipantHelper *helper) {
    NS_LOG_FUNCTION (this);
    m_helper = helper;
}

void
GasperParticipant::SetGenesisVrfSeed(unsigned char *vrfSeed) {
    memset(m_actualVrfSeed, 0, sizeof m_actualVrfSeed);
    memcpy(m_actualVrfSeed, vrfSeed, sizeof m_actualVrfSeed);
}

void GasperParticipant::StartApplication() {
    NS_LOG_FUNCTION(this);
    GasperNode::StartApplication ();
    NS_LOG_INFO("Node " << GetNode()->GetId() << ": fully started");

    // scheduling gasper events
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_intervalAttest), &GasperParticipant::BlockProposalPhase, this);
    NS_LOG_INFO("Node " << GetNode()->GetId() << ": scheduled block proposal");
}

void GasperParticipant::StopApplication() {
    NS_LOG_FUNCTION(this);
    GasperNode::StopApplication ();
    Simulator::Cancel(this->m_nextBlockProposalEvent);
    Simulator::Cancel(this->m_nextAttestEvent);

//    NS_LOG_WARN ("\n\nBITCOIN NODE " << GetNode ()->GetId () << ":");
//    NS_LOG_WARN("Total Blocks = " << m_blockchain.GetTotalBlocks());
//    NS_LOG_WARN("longest fork = " << m_blockchain.GetLongestForkSize());
//    NS_LOG_WARN("blocks in forks = " << m_blockchain.GetBlocksInForks());

    if(m_allPrint || GetNode()->GetId() == 0) {
        std::cout << "\n\nBITCOIN NODE " << GetNode()->GetId() << ":" << std::endl;
        std::cout << "Total Blocks = " << m_blockchain.GetTotalBlocks() << std::endl;
        std::cout << "longest fork = " << m_blockchain.GetLongestForkSize() << std::endl;
        std::cout << "blocks in forks = " << m_blockchain.GetBlocksInForks() << std::endl;
    }
    if(GetNode()->GetId() == 0)
        std::cout << m_blockchain << std::endl;
}

void GasperParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    GasperNode::DoDispose ();
}

void GasperParticipant::HandleCustomRead(rapidjson::Document *document, double receivedTime, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    switch ((*document)["message"].GetInt()) {
        case BLOCK_PROPOSAL:
            NS_LOG_INFO (GetNode()->GetId() << " - Block Proposal");
            ProcessReceivedProposedBlock(document, receivedFrom);
            break;
        case ATTEST:
            NS_LOG_INFO (GetNode()->GetId() << " - Gasper Attest");
            ProcessReceivedAttest(document, receivedFrom);
            break;
        default:
            NS_LOG_INFO (GetNode()->GetId() << " - Default");
            break;
    }
}

unsigned int
GasperParticipant::GenerateVrfSeed() {
    unsigned int vrfSeed = 0;
    while(vrfSeed == 0)
        vrfSeed = m_generator();
    return vrfSeed;
}

void
GasperParticipant::SetVrfThresholdBP(unsigned char *threshold) {
    memset(m_vrfThresholdBP, 0, sizeof m_vrfThresholdBP);
    memcpy(m_vrfThresholdBP, threshold, sizeof m_vrfThresholdBP);
}

void
GasperParticipant::SetVrfThreshold(unsigned char *threshold) {
    memset(m_vrfThreshold, 0, sizeof m_vrfThreshold);
    memcpy(m_vrfThreshold, threshold, sizeof m_vrfThreshold);
}

void
GasperParticipant::SetAllPrint(bool allPrint)
{
    m_allPrint = allPrint;
}

void
GasperParticipant::SetIntervalBP(double interval) {
    m_intervalBP = interval;
}

void
GasperParticipant::SetIntervalAttest(double interval) {
    m_intervalAttest = interval;
}


void GasperParticipant::AdvertiseVoteOrProposal(enum Messages messageType, rapidjson::Document &d){
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
GasperParticipant::SaveBlockToVector(std::vector<std::vector<Block>> *blockVector, int iteration, Block block) {
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

Block*
GasperParticipant::FindBlockInVector(std::vector<std::vector<Block>> *blockVector, int iteration, std::string blockHash) {
    for(auto i = blockVector->at(iteration - 1).begin(); i != blockVector->at(iteration - 1).end(); i++){
        if(i->GetBlockHash() == blockHash)
            return &(*i);
    }

    // block was not found
    return nullptr;
}

int
GasperParticipant::IncreaseCntOfReceivedVotes(std::vector <int> *counterVector, int iteration) {
    NS_LOG_FUNCTION (this);
    // check if our vector have place for blocks in received iteration
    if(counterVector->size() < iteration) {
        counterVector->resize(iteration);
        counterVector->at(iteration-1) = 0;
    }

    // increase count of votes and return actual count
    counterVector->at(iteration-1) = counterVector->at(iteration-1) + 1;
    return counterVector->at(iteration-1);
}

void
GasperParticipant::InformAboutState(int iteration) {
    if(GetNode()->GetId() == 0){
        std::cerr << '\r'
            << std::setw(10) << std::setfill('0') << Simulator::Now().GetSeconds() << ':'
            << std::setw(10) << iteration << ':'
            << std::setw(10) << m_blockchain.GetTotalBlocks() << ':'
            << std::setw(10) << m_blockchain.GetLongestForkSize() << ':'
            << std::setw(10) << m_blockchain.GetBlocksInForks() << ':'
            << std::flush;
    }
}

void
GasperParticipant::InsertBlockToBlockchain(Block &newBlock) {
    if(!m_blockchain.HasBlock(newBlock)) {
        Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
        if(!m_blockchain.IsAncestor(&newBlock, &lastFinalizedCheckpoint))
            return;                 // throw away block which does not have the last finalized checkpoint in its chain

        if(newBlock.GetBlockProposalIteration() % m_maxBlocksInEpoch == 0){
            // tally votes and update blockchain
            TallyingAndBlockchainUpdate();
            m_currentEpoch++;

            // process new checkpoint
            newBlock.SetCasperState(CHECKPOINT);
            BitcoinNode::InsertBlockToBlockchain(newBlock);

            NS_LOG_INFO(GetNode()->GetId() << " - Inserted checkpoint: " << newBlock);
        }else{
            // update blockchain using LEBB rule
            m_blockchain.GasperUpdateEpochBoundaryCheckpoints(&lastFinalizedCheckpoint, m_maxBlocksInEpoch);

            // insert to blockchain in standard way
            BitcoinNode::InsertBlockToBlockchain(newBlock);
            NS_LOG_INFO(GetNode()->GetId() << " - Inserted block: " << newBlock);
        }
    }
}

void
GasperParticipant::TallyingAndBlockchainUpdate() {
    // skipping zeroth epoch (there is only genesis)
    if(m_currentEpoch == 0 || m_votes.size() < m_currentEpoch)
        return;

    int totalVotes = m_votes.at(m_currentEpoch - 1).size();
    if(totalVotes == 0)
        return;

    std::cout <<
    // key is (source hash, target hash), value is count of votes
    std::map<std::pair<std::string, std::string>, int> voteCounter;
    for(auto vote : m_votes.at(m_currentEpoch - 1)){
        rapidjson::Document d;
        d.Parse(vote.second.c_str());

        std::pair<std::string, std::string> sourceTarget = std::make_pair(d["s"].GetString(), d["t"].GetString());

        std::map<std::pair<std::string, std::string>, int>::iterator item = voteCounter.find(sourceTarget);
        if (item == voteCounter.end()) {
            voteCounter.insert({ sourceTarget, 1 });
        }else{
            item->second = item->second + 1;
        }
    }

    // find the one with most votes
    std::pair<std::string, std::string> bestVote = voteCounter.begin()->first;
    int maxVotes = voteCounter.begin()->second;
    for(auto vote : voteCounter){
        if(vote.second > maxVotes){
            maxVotes = vote.second;
            bestVote = vote.first;
        }
    }

    // if quorum reached update blockchain (checkpoints, if finality reached even blocks
    if(maxVotes >  (2 * (totalVotes / 3))){
        NS_LOG_INFO(GetNode()->GetId() << " - EPOCH n." << m_currentEpoch << " quorum for s: " << bestVote.first << ", t: " << bestVote.second << " VOTES " << maxVotes << " out of total " << totalVotes);
        Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
        const Block * newlyFinalized = m_blockchain.CasperUpdateBlockchain(bestVote.first, bestVote.second,
                                                                           &lastFinalizedCheckpoint, m_maxBlocksInEpoch);

        // update information about last finalized checkpoint
        if(newlyFinalized != nullptr){
            m_lastFinalized = std::make_pair(newlyFinalized->GetBlockHeight(), newlyFinalized->GetMinerId());
        }
    }
}


/** ----------- Voting Phase  -  Committee for assets ----------- */

void
GasperParticipant::AttestHlmdPhase() {
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": BP phase started at " << Simulator::Now().GetSeconds());
    m_iterationAttest++;    // increase number of block proposal iterations

    // evaluate certified block vote from previous iteration and save to blockchain
    Block* proposedBlock = FindLowestProposal(m_iterationAttest);
    if(proposedBlock) {
        InsertBlockToBlockchain(*proposedBlock);
    }

    // ------ start real attest html phase ------
    InformAboutState(m_iterationAttest);
    int participantId = GetNode()->GetId();

    crypto_vrf_prove(m_vrfProof, m_sk, (const unsigned char*) m_actualVrfSeed, sizeof m_actualVrfSeed);
    crypto_vrf_proof_to_hash(m_vrfOut, m_vrfProof);

    int chosenAP = memcmp(m_vrfOut, m_vrfThreshold, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen AP: " << chosenAP << " r: " << ((chosenAP <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenAP <= 0) ? true : false;

    // check output of VRF and if chosen then evaluate Hybrid LMD score and send attest vote
    if(chosen) {
        // evaluating hlmd block for searching "most valid" block
        const Block* attestedBlock = EvalHLMDBlock(m_iterationAttest);

        std::pair<const Block*, const Block*> link = FindBestLink(attestedBlock);

        // Extract values from block to rapidjson document vote and broadcast the vote
        rapidjson::Document document;
        rapidjson::Value value;
        document.SetObject();

        value = ATTEST;
        document.AddMember("message", value, document.GetAllocator());

        // casper justification values
        std::string sourceBlockHash = link.first->GetBlockHash();
        value.SetString(sourceBlockHash.c_str(), sourceBlockHash.size(), document.GetAllocator());
        document.AddMember("s", value, document.GetAllocator());
        std::string targetBlockHash = link.second->GetBlockHash();
        value.SetString(targetBlockHash.c_str(), targetBlockHash.size(), document.GetAllocator());
        document.AddMember("t", value, document.GetAllocator());

        value = link.first->GetBlockHeight();
        document.AddMember("hs", value, document.GetAllocator());
        value = link.second->GetBlockHeight();
        document.AddMember("ht", value, document.GetAllocator());

        value = m_currentEpoch;
        document.AddMember("epoch", value, document.GetAllocator());

        // block attest values
        std::string blockHash = attestedBlock->GetBlockHash();
        value.SetString(blockHash.c_str(), blockHash.size(), document.GetAllocator());
        document.AddMember("blockHash", value, document.GetAllocator());
        value = m_iterationAttest;    // this value is Gasper slot number
        document.AddMember("blockIteration", value, document.GetAllocator());
        value = participantId;
        document.AddMember("voterId", value, document.GetAllocator());

        // common values for checking valid voter
        value.SetString((const char*) m_vrfProof, 80, document.GetAllocator());
        document.AddMember("vrfProof", value, document.GetAllocator());
        value.SetString((const char*) m_actualVrfSeed, 32, document.GetAllocator());
        document.AddMember("currentSeed", value, document.GetAllocator());
        value.SetString((const char*) m_pk, 32, document.GetAllocator());
        document.AddMember("vrfPK", value, document.GetAllocator());

        AdvertiseVoteOrProposal(ATTEST, document);
        NS_LOG_INFO(GetNode()->GetId()
                            << " - Advertised attest: {e: " << m_currentEpoch
                            << ", voter: " << participantId
                            << ", s: " << link.first->GetBlockHash()
                            << ", t: " << link.second->GetBlockHash()
                            << ", h(s): " << link.first->GetBlockHeight()
                            << ", h(t): " << link.second->GetBlockHeight()
                            << ", bIt: " << m_iterationAttest
                            << ", b: " << blockHash
                            << "}");

        SaveVoteToAttestBuffer(&document);
        SaveVoteToFFGBuffer(&document);
    }

    // Create new certify vote event in m_certifyVoteInterval seconds
    m_nextBlockProposalEvent = Simulator::Schedule (Seconds(m_intervalAttest), &GasperParticipant::BlockProposalPhase, this);
}

void
GasperParticipant::ProcessReceivedAttest(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION (this);

    // Checking valid VRF
    std::string blockHash = (*message)["blockHash"].GetString();
    int blockIteration = (*message)["blockIteration"].GetInt();
    int participantId = (*message)["voterId"].GetInt();

    Block *attestedBlock = FindBlockInVector(&m_receivedBlockProposals, blockIteration, blockHash);
    if(attestedBlock == nullptr)
        // block proposal was not found so whole attest is invalid
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

    int chosenAtt = memcmp(vrfOut, m_vrfThreshold, sizeof vrfOut);
    bool chosen = (chosenAtt <= 0) ? true : false;

    if(!chosen){
        NS_LOG_INFO ( "INVALID Attest - participantId: " << participantId << " block iteration/slot: " << blockIteration);
        return;
    }

    // saving vote into buffers
    bool insertedA = SaveVoteToAttestBuffer(message);
    bool insertedB = SaveVoteToFFGBuffer(message);

    if(insertedA && insertedB) {
        // received new vote for the epoch
        unsigned char votersPk[32];
        memset(votersPk, 0, sizeof votersPk);
        memcpy(votersPk, (*message)["vrfPK"].GetString(), sizeof votersPk);

        std::string sId = (*message)["s"].GetString();
        std::string tId = (*message)["t"].GetString();
        int sHeight = (*message)["hs"].GetInt();
        int tHeight = (*message)["ht"].GetInt();
        int epoch = (*message)["epoch"].GetInt();

        NS_LOG_INFO(GetNode()->GetId()
        << " - Received attest: {e: " << epoch
        << ", voter: " << participantId
        << ", s: " << sId
        << ", t: " << tId
        << ", h(s): " << sHeight
        << ", h(t): " << tHeight
        << ", bIt: " << blockIteration
        << ", b: " << blockHash
        << "}");
        AdvertiseVoteOrProposal(ATTEST, *message);
    }
}

const Block *
GasperParticipant::EvalHLMDBlock(int iteration) {
    // get new checkpoints (only in chain of last finalized checkpoint)
    Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    std::vector<const Block*> newCheckpoints = m_blockchain.GetNotFinalizedCheckpoints(lastFinalizedCheckpoint);

    const Block* highestJustifiedCheckpoint = newCheckpoints.at(0); // genesis block or last finalized block
    int maxHeight = highestJustifiedCheckpoint->GetBlockHeight();

    for(auto checkpoint : newCheckpoints){
        // use only justified checkpoints (note: finalized checkpoint is justified too)
        if(checkpoint->GetCasperState() == FINALIZED_CHKP
            || checkpoint->GetCasperState() == JUSTIFIED_CHKP)
        {
            // search for highest justified checkpoint
            if(checkpoint->GetBlockHeight() > maxHeight)
                highestJustifiedCheckpoint = checkpoint;
        }
    }

    const Block* B = highestJustifiedCheckpoint;

    for(;;){
        const std::vector<const Block*> children = m_blockchain.GetChildrenPointers(*B);
        if(children.size() == 0)
            break;
        B = FindArgMax(&children, iteration);
    }

    return B;
}

const Block*
GasperParticipant::FindArgMax(const std::vector<const Block *> *children, int iteration) {
    // return only child
    if(children->size() == 1)
        return children->at(0);

    int maxFollowers = 0;
    const Block* B1 = children->at(0);
    // iterate through children and find the one with most followers
    for (auto child : (*children)){
        int childFollowers = 0;
        if(m_receivedAttests.size() < iteration)
            continue;

        // iterate through votes and check their ancestors
        for (auto vote : m_receivedAttests.at(iteration-1)){
            if((*(vote.second)) == (*child)
                || m_blockchain.IsAncestor(vote.second, child))
            {
                childFollowers++;   // increase count of child followers
            }
        }

        // compare count of followers
        if(childFollowers > maxFollowers){
            B1 = child;
        }
    }

    return B1;
}

std::pair<const Block*, const Block*>
GasperParticipant::FindBestLink(const Block *attestedBlock) {
    NS_LOG_FUNCTION(this);
    // find new not finalized yet checkpoints
    Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    std::vector<const Block*> newCheckpoints = m_blockchain.GetNotFinalizedCheckpoints(lastFinalizedCheckpoint);

    // evaluate best two checkpoints
    if(newCheckpoints.size() == 1) {
        return std::make_pair(newCheckpoints.at(0), newCheckpoints.at(0));
    }else {
        // target is boundary epoch last checkpoint which is in chain of attestedBlock

        // NOTE: implementation of this part is not part of Casper or Gasper and depends on blockchain use
        // we will follow Gasper rule, which is following the checkpoint which is ancestor of block with best Hybrid LMD score
        // CASPER slashing RULES:
        // (h(t1) != h(t2))   &&   !(h(s1) < h(s2) < h(t2) < h(t1))

        // target should not be justified to meet the first rule
        // vector contains blocks ordered by their epoch height
        std::vector<std::vector<const Block*>> possibleTargets;

        // source can be any justified or finalized checkpoint, if it is in chain of the chosen target
        // vector contains checkpoints ordered by their epoch height (each height have only 1 item, but this is easy way for have them sorted)
        std::vector<std::vector<const Block*>> possibleSources;

        for(auto b : newCheckpoints){
            int relativeEpochHeight = ((b->GetBlockHeight()) - m_lastFinalized.first) / m_maxBlocksInEpoch;
            // process possible targets
            if(b->GetCasperState() == CHECKPOINT) {
                // resize vector for certain epoch height, if needed
                if((relativeEpochHeight + 1) > possibleTargets.size()) {
                    possibleTargets.resize(relativeEpochHeight + 1);
                }

                // insert on certain position
                possibleTargets.at(relativeEpochHeight).push_back(b);
            }else{
                // resize vector for certain epoch height, if needed
                if((relativeEpochHeight + 1) > possibleSources.size()) {
                    possibleSources.resize(relativeEpochHeight + 1);
                }

                // save info about possible source
                possibleSources.at(relativeEpochHeight).push_back(b);
            }
        }

        // remove not possible targets - other justified checkpoint is on same height
        for(auto justified : possibleSources){
            if(justified.size() != 0) {
                int relativeEpochHeight =
                        (justified.at(0)->GetBlockHeight() - m_lastFinalized.first) / m_maxBlocksInEpoch;
                if (possibleTargets.size() > (relativeEpochHeight + 1))
                    possibleTargets.at(relativeEpochHeight).clear();
            }
        }

        // find highest epoch which is not in conflict (highest count of checkpoints in its chain)
        // ATTENTION: this part is different from standard Casper (CasperParticipant)
        const Block* highest = newCheckpoints.at(0);
        int highestRelativeHeight = 0;
        for(int height = possibleTargets.size() - 1; height >= 0; height--){
            for (auto possibleTarget : possibleTargets.at(height)) {
                if (m_blockchain.IsAncestor(attestedBlock, possibleTarget)) {
                    highest = possibleTarget;
                    highestRelativeHeight = height;
                    break;
                }
            }
        }

        // target should be lowest checkpoint in chain of highest epoch
        const Block* target = highest;
        std::vector<const Block*> highestAncestors = m_blockchain.GetAncestorsPointers(*highest, m_lastFinalized.first);
        bool found = false;
        for(int height = 0; height <= highestRelativeHeight; height++){
            if(possibleTargets.size() >= (height + 1)) {
                for (auto possibleTarget : possibleTargets.at(height)) {
                    // check if target is in highest targets chain
                    if(m_blockchain.IsAncestor(highest, possibleTarget)) {
                        target = possibleTarget;
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
            }
        }

        // choosing source - highest justified checkpoint in targets chain
        const Block* source = newCheckpoints.at(0);
        std::vector<const Block*> targetsAncestors = m_blockchain.GetAncestorsPointers(*target, m_lastFinalized.first);
        for(int height = possibleSources.size() - 1; height >= 0; height--){
            if(possibleSources.at(height).size() != 0) {
                const Block* possibleSource = possibleSources.at(height).at(0);
                // check height
                if(possibleSource->GetBlockHeight() >= target->GetBlockHeight())
                    continue;

                // check if source is in targets chain
                if(m_blockchain.IsAncestor(target, possibleSource)) {
                    source = possibleSource;
                    break;
                }
            }
        }

        return std::make_pair(source, target);
    }
}

bool
GasperParticipant::SaveVoteToFFGBuffer(rapidjson::Document *vote) {
    int epoch = (*vote)["epoch"].GetInt();
    int voterId = (*vote)["voterId"].GetInt();

    // resize vector of votes if needed
    if(m_votes.size() < epoch)
        m_votes.resize(epoch);

    // search for vote from the voter in the epoch
    if (m_votes.at(epoch-1).find(voterId) == m_votes.at(epoch-1).end() ) {
        // not found

        // Extract values from block to rapidjson document vote and broadcast the vote
        rapidjson::Document document;
        rapidjson::Value value;
        document.SetObject();

        value = ATTEST;
        document.AddMember("message", value, document.GetAllocator());

        // casper justification values
        std::string sourceBlockHash =  (*vote)["s"].GetString();
        value.SetString(sourceBlockHash.c_str(), sourceBlockHash.size(), document.GetAllocator());
        document.AddMember("s", value, document.GetAllocator());
        std::string targetBlockHash = (*vote)["t"].GetString();
        value.SetString(targetBlockHash.c_str(), targetBlockHash.size(), document.GetAllocator());
        document.AddMember("t", value, document.GetAllocator());

        value = (*vote)["hs"].GetInt();
        document.AddMember("hs", value, document.GetAllocator());
        value = (*vote)["ht"].GetInt();
        document.AddMember("ht", value, document.GetAllocator());

        value =  (*vote)["epoch"].GetInt();
        document.AddMember("epoch", value, document.GetAllocator());
        value =  (*vote)["voterId"].GetInt();
        document.AddMember("voterId", value, document.GetAllocator());


        // create json string buffer
        rapidjson::StringBuffer jsonBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonBuffer);
        document.Accept(writer);

        // insert vote (serialized json) to vote buffer
        m_votes.at(epoch-1).insert({ voterId, jsonBuffer.GetString() });
        return true;
    } else {
        // found
        return false;
    }
}

bool
GasperParticipant::SaveVoteToAttestBuffer(rapidjson::Document *vote) {
    int blockIteration = (*vote)["blockIteration"].GetInt();
    int voterId = (*vote)["voterId"].GetInt();

    // resize vector of votes if needed
    if(m_receivedAttests.size() < blockIteration)
        m_receivedAttests.resize(blockIteration);

    // search for attest from the voter in the slot/block iteration
    if (m_receivedAttests.at(blockIteration-1).find(voterId) == m_receivedAttests.at(blockIteration-1).end() ) {
        // not found
        // find block in blockchain
        std::string   blockHash = (*vote)["blockHash"].GetString();
        std::string   delimiter = "/";
        size_t        pos = blockHash.find(delimiter);
        int height = atoi(blockHash.substr(0, pos).c_str());
        int minerId = atoi(blockHash.substr(pos+1, blockHash.size()).c_str());

        NS_LOG_INFO(GetNode()->GetId() << " - Saving to attest buffer: h = " << height << " , pId: " << minerId);
        Block block(height, minerId, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
        const Block * blockPointer = m_blockchain.GetBlockPointer(block);

        if(blockPointer != nullptr){
            // insert vote (serialized json) to vote buffer
            m_receivedAttests.at(blockIteration-1).insert({ voterId, blockPointer});
            return true;
        }
    }

    // found
    return false;
}



/** ----------- end of: ATTEST PHASE ----------- */

/** ----------- BLOCK PROPOSAL PHASE ----------- */

void GasperParticipant::BlockProposalPhase() {
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": BP phase started at " << Simulator::Now().GetSeconds());

    m_iterationBP++;    // increase number of block proposal iterations
    InformAboutState(m_iterationBP);
    int participantId = GetNode()->GetId();

    crypto_vrf_prove(m_vrfProof, m_sk, (const unsigned char*) m_actualVrfSeed, sizeof m_actualVrfSeed);
    crypto_vrf_proof_to_hash(m_vrfOut, m_vrfProof);

    int chosenBP = memcmp(m_vrfOut, m_vrfThresholdBP, sizeof m_vrfOut);
    NS_LOG_INFO ( participantId << " - Chosen BP: " << chosenBP << " r: " << ((chosenBP <= 0) ? "Chosen" : "Not chosen"));
    bool chosen = (chosenBP <= 0) ? true : false;

    // check output of VRF and if chosen then create and send
    if(chosen){
        // Extract info from blockchain
        const Block* prevBlock = EvalHLMDBlock(m_iterationBP);
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
    m_nextAttestEvent = Simulator::Schedule (Seconds(m_intervalBP), &GasperParticipant::AttestHlmdPhase, this);
}

void GasperParticipant::ProcessReceivedProposedBlock(rapidjson::Document *message, Address receivedFrom) {
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

Block*
GasperParticipant::FindLowestProposal(int iteration){
    if(iteration == 0)
        return nullptr;

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
        // if the item have lower VRF Output then actualize index
        memcpy(vrfOut1, m_receivedBlockProposals.at(iteration - 1).at(lowestIndex).GetVrfOutput(), sizeof vrfOut1);
        memcpy(vrfOut2, (i)->GetVrfOutput(), sizeof vrfOut2);
        if(memcmp(vrfOut1, vrfOut2, sizeof vrfOut1) > 0) {
            lowestIndex = std::distance(m_receivedBlockProposals.at(iteration - 1).begin(), i);;
        }
    }
    // return block with lowest VRF Output
    return &(m_receivedBlockProposals.at(iteration - 1).at(lowestIndex));
}

bool
GasperParticipant::IsVotedBlockValid(Block *block) {
    // checking only validity of position in blockchain
    const Block* currentTopBlock = m_blockchain.GetCurrentTopBlock();

    if(block->GetBlockHeight() - 1 == currentTopBlock->GetBlockHeight())
        return true;

    return false;
}

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