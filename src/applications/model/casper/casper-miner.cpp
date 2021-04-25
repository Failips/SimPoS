/**
* Implementation of CasperMiner class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "casper-miner.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("CasperMiner");

    NS_OBJECT_ENSURE_REGISTERED (CasperMiner);

    TypeId
    CasperMiner::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::CasperMiner")
                .SetParent<Application> ()
                .SetGroupName("Applications")
                .AddConstructor<CasperMiner> ()
                .AddAttribute ("Local",
                               "The Address on which to Bind the rx socket.",
                               AddressValue (),
                               MakeAddressAccessor (&CasperMiner::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("Protocol",
                               "The type id of the protocol to use for the rx socket.",
                               TypeIdValue (UdpSocketFactory::GetTypeId ()),
                               MakeTypeIdAccessor (&CasperMiner::m_tid),
                               MakeTypeIdChecker ())
                .AddAttribute ("BlockTorrent",
                               "Enable the BlockTorrent protocol",
                               BooleanValue (false),
                               MakeBooleanAccessor (&CasperMiner::m_blockTorrent),
                               MakeBooleanChecker ())
                .AddAttribute ("SPV",
                               "Enable SPV Mechanism",
                               BooleanValue (false),
                               MakeBooleanAccessor (&CasperMiner::m_spv),
                               MakeBooleanChecker ())
                .AddAttribute ("NumberOfMiners",
                               "The number of miners",
                               UintegerValue (16),
                               MakeUintegerAccessor (&CasperMiner::m_noMiners),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("FixedBlockSize",
                               "The fixed size of the block",
                               UintegerValue (0),
                               MakeUintegerAccessor (&CasperMiner::m_fixedBlockSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("FixedBlockIntervalGeneration",
                               "The fixed time to wait between two consecutive block generations",
                               DoubleValue (0),
                               MakeDoubleAccessor (&CasperMiner::m_fixedBlockTimeGeneration),
                               MakeDoubleChecker<double> ())
                .AddAttribute ("InvTimeoutMinutes",
                               "The timeout of inv messages in minutes",
                               TimeValue (Minutes (20)),
                               MakeTimeAccessor (&CasperMiner::m_invTimeoutMinutes),
                               MakeTimeChecker())
                .AddAttribute ("HashRate",
                               "The hash rate of the miner",
                               DoubleValue (0.2),
                               MakeDoubleAccessor (&CasperMiner::m_hashRate),
                               MakeDoubleChecker<double> ())
                .AddAttribute ("BlockGenBinSize",
                               "The block generation bin size",
                               DoubleValue (-1),
                               MakeDoubleAccessor (&CasperMiner::m_blockGenBinSize),
                               MakeDoubleChecker<double> ())
                .AddAttribute ("BlockGenParameter",
                               "The block generation distribution parameter",
                               DoubleValue (-1),
                               MakeDoubleAccessor (&CasperMiner::m_blockGenParameter),
                               MakeDoubleChecker<double> ())
                .AddAttribute ("AverageBlockGenIntervalSeconds",
                               "The average block generation interval we aim at (in seconds)",
                               DoubleValue (10*60),
                               MakeDoubleAccessor (&CasperMiner::m_averageBlockGenIntervalSeconds),
                               MakeDoubleChecker<double> ())
                .AddAttribute ("Cryptocurrency",
                               "BITCOIN, LITECOIN, DOGECOIN",
                               UintegerValue (0),
                               MakeUintegerAccessor (&CasperMiner::m_cryptocurrency),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("ChunkSize",
                               "The fixed size of the block chunk",
                               UintegerValue (100000),
                               MakeUintegerAccessor (&CasperMiner::m_chunkSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddTraceSource ("Rx",
                                 "A packet has been received",
                                 MakeTraceSourceAccessor (&CasperMiner::m_rxTrace),
                                 "ns3::Packet::AddressTracedCallback")
                .AddAttribute ("IsFailed",
                               "Set participant to be a failed state",
                               BooleanValue (false),
                               MakeBooleanAccessor (&CasperMiner::m_isFailed),
                               MakeBooleanChecker ())
         ;
        return tid;
    }

    CasperMiner::CasperMiner() : CasperParticipant(),
                                             m_timeStart(0), m_timeFinish(0) {
        NS_LOG_FUNCTION(this);
    }

    CasperMiner::~CasperMiner(void) {
        NS_LOG_FUNCTION(this);
    }

    void CasperMiner::StartApplication() {
        BitcoinMiner::StartApplication ();
        m_nodeStats->miner = 1;
        m_nodeStats->voteSentBytes = 0;
        m_nodeStats->voteReceivedBytes = 0;
        m_nodeStats->totalCheckpoints = 0;
        m_nodeStats->totalFinalizedCheckpoints = 0;
        m_nodeStats->totalJustifiedCheckpoints = 0;
        m_nodeStats->totalNonJustifiedCheckpoints = 0;
        m_nodeStats->totalFinalizedBlocks = 0;
        m_nodeStats->isFailed = m_isFailed;
    }

    void CasperMiner::StopApplication() {
        BitcoinMiner::StopApplication ();
        std::cout << "\n\nBITCOIN MINER " << GetNode()->GetId() << ":" << std::endl;
        std::cout << "Total Blocks = " << m_blockchain.GetTotalBlocks() << std::endl;
        std::cout << "Total Checkpoints = " << m_blockchain.GetTotalCheckpoints() << std::endl;
        std::cout << "Total Finalized Checkpoints = " << m_blockchain.GetTotalFinalizedCheckpoints() << std::endl;
        std::cout << "Total Justified Checkpoints = " << m_blockchain.GetTotalJustifiedCheckpoints() << std::endl;
        std::cout << "Total Finalized Blocks = " << m_blockchain.GetTotalFinalizedBlocks() << std::endl;
        std::cout << "longest fork = " << m_blockchain.GetLongestForkSize() << std::endl;
        std::cout << "blocks in forks = " << m_blockchain.GetBlocksInForks() << std::endl;
        std::cout << "failed = " << (m_isFailed ? "true" : "false") << std::endl;

        m_nodeStats->totalCheckpoints = m_blockchain.GetTotalCheckpoints();
        m_nodeStats->totalFinalizedCheckpoints = m_blockchain.GetTotalFinalizedCheckpoints();
        m_nodeStats->totalJustifiedCheckpoints = m_blockchain.GetTotalJustifiedCheckpoints();
        m_nodeStats->totalNonJustifiedCheckpoints = m_blockchain.GetTotalNonJustifiedCheckpoints();
        m_nodeStats->totalFinalizedBlocks = m_blockchain.GetTotalFinalizedBlocks();
        m_nodeStats->isFailed = m_isFailed;
    }

    void CasperMiner::DoDispose(void) {
        NS_LOG_FUNCTION (this);
        BitcoinMiner::DoDispose ();
    }

    void
    CasperMiner::MineBlock() { BitcoinMiner::MineBlock(); }

    void
    CasperMiner::ReceivedHigherBlock(const Block &newBlock) { BitcoinMiner::ReceivedHigherBlock(newBlock); }

    void CasperMiner::HandleCustomRead(rapidjson::Document *document, double receivedTime, Address receivedFrom) {
        NS_LOG_FUNCTION(this);
        CasperParticipant::HandleCustomRead (document, receivedTime, receivedFrom);
    }

    void
    CasperMiner::InsertBlockToBlockchain(Block &newBlock) {
        if(!m_blockchain.HasBlock(newBlock)) {
            Block lastFinalizedCheckpoint(m_lastFinalized.first, m_lastFinalized.second, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
            if(!m_blockchain.IsAncestor(&newBlock, &lastFinalizedCheckpoint))
                return;                 // throw away block which does not have the last finalized checkpoint in its chain

            if(newBlock.GetBlockHeight() % m_maxBlocksInEpoch == 0){
                // tally votes
                CasperParticipant::TallyingAndBlockchainUpdate();
                m_currentEpoch++;

                // process new checkpoint
                newBlock.SetCasperState(CHECKPOINT);
                BitcoinNode::InsertBlockToBlockchain(newBlock);

                NS_LOG_INFO(GetNode()->GetId() << " - Inserted checkpoint: " << newBlock);
            }else{
                // insert to blockchain in standard way
                BitcoinNode::InsertBlockToBlockchain(newBlock);
            }
        }
    }

} //  ns3 namespace