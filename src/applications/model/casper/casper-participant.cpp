/**
* Implementation of CasperParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "casper-participant.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include "ns3/socket.h"
#include "../../libsodium/include/sodium.h"
#include <iomanip>
#include <sys/time.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CasperParticipant");

NS_OBJECT_ENSURE_REGISTERED (CasperParticipant);

TypeId
CasperParticipant::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::CasperParticipant")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<CasperParticipant>()
            .AddAttribute ("Local",
                           "The Address on which to Bind the rx socket.",
                           AddressValue (),
                           MakeAddressAccessor (&CasperParticipant::m_local),
                           MakeAddressChecker ())
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&CasperParticipant::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                           "Enable the BlockTorrent protocol",
                           BooleanValue (false),
                           MakeBooleanAccessor (&CasperParticipant::m_blockTorrent),
                           MakeBooleanChecker ())
            .AddAttribute ("SPV",
                           "Enable SPV Mechanism",
                           BooleanValue (false),
                           MakeBooleanAccessor (&CasperParticipant::m_spv),
                           MakeBooleanChecker ())
            .AddAttribute ("NumberOfMiners",
                           "The number of miners",
                           UintegerValue (16),
                           MakeUintegerAccessor (&CasperParticipant::m_noMiners),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("FixedBlockSize",
                           "The fixed size of the block",
                           UintegerValue (0),
                           MakeUintegerAccessor (&CasperParticipant::m_fixedBlockSize),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("FixedBlockIntervalGeneration",
                           "The fixed time to wait between two consecutive block generations",
                           DoubleValue (0),
                           MakeDoubleAccessor (&CasperParticipant::m_fixedBlockTimeGeneration),
                           MakeDoubleChecker<double> ())
            .AddAttribute ("InvTimeoutMinutes",
                           "The timeout of inv messages in minutes",
                           TimeValue (Minutes (20)),
                           MakeTimeAccessor (&CasperParticipant::m_invTimeoutMinutes),
                           MakeTimeChecker())
            .AddAttribute ("HashRate",
                           "The hash rate of the miner",
                           DoubleValue (0.2),
                           MakeDoubleAccessor (&CasperParticipant::m_hashRate),
                           MakeDoubleChecker<double> ())
            .AddAttribute ("BlockGenBinSize",
                           "The block generation bin size",
                           DoubleValue (-1),
                           MakeDoubleAccessor (&CasperParticipant::m_blockGenBinSize),
                           MakeDoubleChecker<double> ())
            .AddAttribute ("BlockGenParameter",
                           "The block generation distribution parameter",
                           DoubleValue (-1),
                           MakeDoubleAccessor (&CasperParticipant::m_blockGenParameter),
                           MakeDoubleChecker<double> ())
            .AddAttribute ("AverageBlockGenIntervalSeconds",
                           "The average block generation interval we aim at (in seconds)",
                           DoubleValue (10*60),
                           MakeDoubleAccessor (&CasperParticipant::m_averageBlockGenIntervalSeconds),
                           MakeDoubleChecker<double> ())
            .AddAttribute ("Cryptocurrency",
                           "BITCOIN, LITECOIN, DOGECOIN",
                           UintegerValue (0),
                           MakeUintegerAccessor (&CasperParticipant::m_cryptocurrency),
                           MakeUintegerChecker<uint32_t> ())
            .AddAttribute ("ChunkSize",
                           "The fixed size of the block chunk",
                           UintegerValue (100000),
                           MakeUintegerAccessor (&CasperParticipant::m_chunkSize),
                           MakeUintegerChecker<uint32_t> ())
            .AddTraceSource ("Rx",
                             "A packet has been received",
                             MakeTraceSourceAccessor (&CasperParticipant::m_rxTrace),
                             "ns3::Packet::AddressTracedCallback")
            .AddAttribute ("IsFailed",
                           "Set participant to be a failed state",
                           BooleanValue (false),
                           MakeBooleanAccessor (&CasperParticipant::m_isFailed),
                           MakeBooleanChecker ());
    return tid;
}

CasperParticipant::CasperParticipant() : BitcoinMiner(),
                               m_timeStart(0), m_timeFinish(0) {
    NS_LOG_FUNCTION(this);

    m_currentEpoch = 0;
    m_maxBlocksInEpoch = 50;
    m_lastFinalized = std::make_pair(0, -1);    // last finalized is genesis block

    m_fixedVoteSize = 256; // size of vote in Bytes

    // generation of voters secret and public keys
    memset(m_sk, 0, sizeof m_sk);
    memset(m_pk, 0, sizeof m_pk);
    crypto_vrf_keypair(m_pk, m_sk);
}

CasperParticipant::~CasperParticipant(void) {
    NS_LOG_FUNCTION(this);
}

void
CasperParticipant::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    NS_LOG_FUNCTION (this);
    m_blockBroadcastType = blockBroadcastType;
}

void
CasperParticipant::SetEpochSize(int epochSize) {
    m_maxBlocksInEpoch = epochSize;
}


void CasperParticipant::StartApplication() {
    BitcoinNode::StartApplication ();
    m_nodeStats->voteSentBytes = 0;
    m_nodeStats->voteReceivedBytes = 0;
    m_nodeStats->totalCheckpoints = 0;
    m_nodeStats->totalFinalizedCheckpoints = 0;
    m_nodeStats->totalJustifiedCheckpoints = 0;
    m_nodeStats->totalFinalizedBlocks = 0;
    m_nodeStats->isFailed = m_isFailed;
}

void CasperParticipant::StopApplication() {
    BitcoinNode::StopApplication ();
    std::cout << "\n\nCasper Voter " << GetNode()->GetId() << ":" << std::endl;
    std::cout << "Total Blocks = " << m_blockchain.GetTotalBlocks() << std::endl;
    std::cout << "Total Checkpoints = " << m_blockchain.GetTotalCheckpoints() << std::endl;
    std::cout << "Total Finalized Checkpoints = " << m_blockchain.GetTotalFinalizedCheckpoints() << std::endl;
    std::cout << "Total Justified Checkpoints = " << m_blockchain.GetTotalJustifiedCheckpoints() << std::endl;
    std::cout << "Total Finalized Blocks = " << m_blockchain.GetTotalFinalizedBlocks() << std::endl;
    std::cout << "longest fork = " << m_blockchain.GetLongestForkSize() << std::endl;
    std::cout << "blocks in forks = " << m_blockchain.GetBlocksInForks() << std::endl;
    std::cout << "failed = " << (m_isFailed ? "true" : "false") << std::endl;
//    if(GetNode()->GetId() == 20){
//        std::cout << m_blockchain << std::endl;
//    }

    m_nodeStats->totalCheckpoints = m_blockchain.GetTotalCheckpoints();
    m_nodeStats->totalFinalizedCheckpoints = m_blockchain.GetTotalFinalizedCheckpoints();
    m_nodeStats->totalJustifiedCheckpoints = m_blockchain.GetTotalJustifiedCheckpoints();
    m_nodeStats->totalFinalizedBlocks = m_blockchain.GetTotalFinalizedBlocks();
    m_nodeStats->isFailed = m_isFailed;
}

void CasperParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    BitcoinNode::DoDispose ();
}

void CasperParticipant::GenNextBlockSize()
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

void
CasperParticipant::MineBlock() {}

void
CasperParticipant::ReceivedHigherBlock(const Block &newBlock) {}

void CasperParticipant::HandleCustomRead(rapidjson::Document *document, double receivedTime, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    switch ((*document)["message"].GetInt()) {
        case CASPER_VOTE:
            ProcessReceivedCasperVote(document, receivedFrom);
            break;
        default:
            NS_LOG_INFO ("Default");
            break;
    }
}


void CasperParticipant::AdvertiseVote(enum Messages messageType, rapidjson::Document &d, Address *doNotSendTo){
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

        sendTime = m_fixedVoteSize / m_uploadSpeed;
        m_nodeStats->voteSentBytes += m_fixedVoteSize;
        count++;
        Simulator::Schedule (Seconds(sendTime), &CasperParticipant::SendMessage, this, NO_MESSAGE, messageType, msg, m_peersSockets[*i]);
    }

    NS_LOG_INFO(GetNode()->GetId() << " - advertised vote to " << count << " nodes.");
}

void
CasperParticipant::SendMessage(enum Messages receivedMessage,  enum Messages responseMessage, std::string d, Ptr<Socket> outgoingSocket){
    rapidjson::Document msg;
    msg.Parse(d.c_str());
    BitcoinNode::SendMessage(receivedMessage, responseMessage, msg, outgoingSocket);
}

void
CasperParticipant::InformAboutState() {
    if(GetNode()->GetId() == 1){
        std::cerr << '\r'
                  << std::setw(10) << std::setfill('0') << Simulator::Now().GetSeconds() << ':'
                  << std::setw(10) << m_currentEpoch << ':'
                  << std::setw(10) << m_blockchain.GetTotalBlocks() << ':'
                  << std::setw(10) << m_blockchain.GetLongestForkSize() << ':'
                  << std::setw(10) << m_blockchain.GetBlocksInForks() << ':'
                  << std::flush;
    }
}


void
CasperParticipant::InsertBlockToBlockchain(Block &newBlock) {
    if(!m_blockchain.HasBlock(newBlock)) {
//        InformAboutState();   // print state to stderr

        Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
        if(!m_blockchain.IsAncestor(&newBlock, &lastFinalizedCheckpoint))
            return;                 // throw away block which does not have the last finalized checkpoint in its chain

        if(newBlock.GetBlockHeight() % m_maxBlocksInEpoch == 0){
            // tally votes and update blockchain
            TallyingAndBlockchainUpdate();
            m_currentEpoch++;

            // process new checkpoint
            newBlock.SetCasperState(CHECKPOINT);
            BitcoinNode::InsertBlockToBlockchain(newBlock);

            NS_LOG_INFO(GetNode()->GetId() << " - Inserted checkpoint: " << newBlock);

            Vote();
        }else{
            // insert to blockchain in standard way
            BitcoinNode::InsertBlockToBlockchain(newBlock);
        }
    }
}

void
CasperParticipant::TallyingAndBlockchainUpdate() {
    // skipping zeroth epoch (there is only genesis)
    if(m_currentEpoch == 0 || m_votes.size() < m_currentEpoch)
        return;

    int totalVotes = m_votes.at(m_currentEpoch - 1).size();
    if(totalVotes == 0)
        return;

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


std::pair<const Block*, const Block*>
CasperParticipant::FindBestLink() {
    NS_LOG_FUNCTION(this);
    // find new not finalized yet checkpoints
    Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    std::vector<const Block*> newCheckpoints = m_blockchain.GetNotFinalizedCheckpoints(lastFinalizedCheckpoint);

    // evaluate best two checkpoints
    if(newCheckpoints.size() == 2) {
        return std::make_pair(newCheckpoints.at(0), newCheckpoints.at(1));
    }else{
        // NOTE: implementation of this part is not part of Casper and depends on blockchain use
        // we will follow Ethereum rule, which is following the checkpoint with higher epoch
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
        const Block* highest = newCheckpoints.at(0);
        int highestRelativeHeight = 0;
        for(int height = possibleTargets.size() - 1; height >= 0; height--){
            if(possibleTargets.at(height).size() == 1) {
                highest = possibleTargets.at(height).at(0);
                highestRelativeHeight = height;
                break;
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


void
CasperParticipant::Vote() {
    NS_LOG_FUNCTION(this);

    std::pair<const Block*, const Block*> link = FindBestLink();

    rapidjson::Document document;
    rapidjson::Value value;
    document.SetObject();

    value = CASPER_VOTE;
    document.AddMember("message", value, document.GetAllocator());

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

    // THIS IS correct, just Bitcoin Sim is not supporting crypto values (using delimeter "#" when sending data)
//    value.SetString((const char*) m_pk, 32, document.GetAllocator());
//    document.AddMember("pk", value, document.GetAllocator());

    value = GetNode()->GetId();
    document.AddMember("pId", value, document.GetAllocator());

    value = m_currentEpoch;
    document.AddMember("epoch", value, document.GetAllocator());

    SaveVoteToBuffer(&document);

    NS_LOG_INFO(GetNode()->GetId() << " - Voted for: {e: " << m_currentEpoch
    << ", s: " << sourceBlockHash << ", t: " << targetBlockHash
    << ", h(s): " << link.first->GetBlockHeight() << ", h(t): " << link.second->GetBlockHeight()
    << ", voter: " << GetNode()->GetId() << "}");
    AdvertiseVote (CASPER_VOTE, document);
}

bool
CasperParticipant::SaveVoteToBuffer(rapidjson::Document *vote) {
    int epoch = (*vote)["epoch"].GetInt();
    int voterId = (*vote)["pId"].GetInt();

    // resize vector of votes if needed
    if(m_votes.size() < epoch)
        m_votes.resize(epoch);

    // search for vote from the voter in the epoch
    if (m_votes.at(epoch-1).find(voterId) == m_votes.at(epoch-1).end() ) {
        // not found
        // create json string buffer
        rapidjson::StringBuffer jsonBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonBuffer);
        vote->Accept(writer);

        // insert vote (serialized json) to vote buffer
        m_votes.at(epoch-1).insert({ voterId, jsonBuffer.GetString() });
        return true;
    } else {
        // found
        return false;
    }
}


void CasperParticipant::ProcessReceivedCasperVote(rapidjson::Document *message, Address receivedFrom) {
    NS_LOG_FUNCTION(this);

    bool inserted = SaveVoteToBuffer(message);
    m_nodeStats->voteReceivedBytes += m_fixedVoteSize;

    if(inserted){
        // received new vote for the epoch

        // THIS IS correct, just Bitcoin Sim is not supporting crypto values (using delimeter "#" when sending data)
//        unsigned char votersPk[32];
//        memset(votersPk, 0, sizeof votersPk);
//        memcpy(votersPk, (*message)["pk"].GetString(), sizeof votersPk);


        std::string sId = (*message)["s"].GetString();
        std::string tId = (*message)["t"].GetString();
        int sHeight = (*message)["hs"].GetInt();
        int tHeight = (*message)["ht"].GetInt();
        int votersId = (*message)["pId"].GetInt();
        int epoch = (*message)["epoch"].GetInt();

        NS_LOG_INFO(GetNode()->GetId() << " - Received vote: {e: " << epoch << ", s: " << sId << ", t: " << tId << ", h(s): " << sHeight << ", h(t): " << tHeight << ", voter: " << votersId << "}");
        AdvertiseVote (CASPER_VOTE, *message, &receivedFrom);
    }
}

} //  ns3 namespace