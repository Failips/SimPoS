/**
 * This file contains the definitions of the functions declared in blockchain.h
  */


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/log.h"
#include "blockchain.h"
#include "ns3/simulator.h"
#include <bits/stdc++.h>

namespace ns3 {

/**
 *
 * Class Block functions
 *
 */

Block::Block(int blockHeight, int minerId, int parentBlockMinerId, int blockSizeBytes,
             double timeCreated, double timeReceived, Ipv4Address receivedFromIpv4)
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distribution(0, INT_MAX); // universal distribution for all INT numbers

    m_blockId = distribution(gen);
    m_blockHeight = blockHeight;
    m_minerId = minerId;
    m_parentBlockMinerId = parentBlockMinerId;
    m_blockSizeBytes = blockSizeBytes;
    m_timeCreated = timeCreated;
    m_timeReceived = timeReceived;
    m_receivedFromIpv4 = receivedFromIpv4;

    memset(m_participantPublicKey,0, sizeof m_participantPublicKey);
    memset(m_vrfOutput,0, sizeof m_vrfOutput);
}

Block::Block()
{
    Block(0, 0, 0, 0, 0, 0, Ipv4Address("0.0.0.0"));
}

Block::Block (const Block &blockSource)
{
    m_blockId = blockSource.m_blockId;
    m_blockHeight = blockSource.m_blockHeight;
    m_minerId = blockSource.m_minerId;
    m_parentBlockMinerId = blockSource.m_parentBlockMinerId;
    m_blockSizeBytes = blockSource.m_blockSizeBytes;
    m_timeCreated = blockSource.m_timeCreated;
    m_timeReceived = blockSource.m_timeReceived;
    m_receivedFromIpv4 = blockSource.m_receivedFromIpv4;
    m_blockProposalIteration = blockSource.m_blockProposalIteration;
    m_vrfSeed = blockSource.m_vrfSeed;
    m_casperState = blockSource.m_casperState;
    memcpy(m_participantPublicKey, blockSource.m_participantPublicKey, sizeof m_participantPublicKey);
    memcpy(m_vrfOutput, blockSource.m_vrfOutput, sizeof m_vrfOutput);
}

Block::~Block (void)
{
}

int
Block::GetBlockId() const {
    return m_blockId;
}

void
Block::SetBlockId(int blockId) {
    m_blockId = blockId;
}

std::string
Block::GetBlockHash(void) const {
    std::ostringstream stringStream;
    stringStream << GetBlockHeight () << "/" << GetMinerId ();
    return stringStream.str();
}

int
Block::GetBlockHeight (void) const
{
    return m_blockHeight;
}

void
Block::SetBlockHeight (int blockHeight)
{
    m_blockHeight = blockHeight;
}

int
Block::GetMinerId (void) const
{
    return m_minerId;
}

void
Block::SetMinerId (int minerId)
{
    m_minerId = minerId;
}

int
Block::GetParentBlockMinerId (void) const
{
    return m_parentBlockMinerId;
}

void
Block::SetParentBlockMinerId (int parentBlockMinerId)
{
    m_parentBlockMinerId = parentBlockMinerId;
}

int
Block::GetBlockSizeBytes (void) const
{
    return m_blockSizeBytes;
}

void
Block::SetBlockSizeBytes (int blockSizeBytes)
{
    m_blockSizeBytes = blockSizeBytes;
}

int
Block::GetBlockProposalIteration() const {
    return m_blockProposalIteration;
}

void
Block::SetBlockProposalIteration(int blockProposalIteration) {
    m_blockProposalIteration = blockProposalIteration;
}

unsigned int
Block::GetVrfSeed() const {
    return m_vrfSeed;
}

void
Block::SetVrfSeed(unsigned int vrfSeed) {
    m_vrfSeed = vrfSeed;
}

unsigned char*
Block::GetParticipantPublicKey() const {
    return (unsigned char*) m_participantPublicKey;
}

void
Block::SetParticipantPublicKey(unsigned char *publicKey) {
    memset(m_participantPublicKey, 0, sizeof m_participantPublicKey);
    memcpy(m_participantPublicKey, publicKey, sizeof m_participantPublicKey);
//    std::cout << "SET VRF PK:" << std::endl;
//    for(int i=0; i<(sizeof m_participantPublicKey); ++i)
//        std::cout << std::hex << (int)m_participantPublicKey[i];
//    std::cout << std::endl << std::dec;
}

unsigned char*
Block::GetVrfOutput() const {
    return (unsigned char*) m_vrfOutput;
}

void
Block::SetVrfOutput(unsigned char *vrfOutput) {
    memset(m_vrfOutput, 0, sizeof m_vrfOutput);
    memcpy(m_vrfOutput, vrfOutput, sizeof m_vrfOutput);
//    std::cout << "SET VRF OUTP:" << std::endl;
//    for(int i=0; i<(sizeof m_vrfOutput); ++i)
//        std::cout << std::hex << (int)m_vrfOutput[i];
//    std::cout << std::endl << std::dec;
}

double
Block::GetTimeCreated (void) const
{
    return m_timeCreated;
}

double
Block::GetTimeReceived (void) const
{
    return m_timeReceived;
}

void
Block::SetTimeReceived (double timeReceived)
{
    m_timeReceived = timeReceived;
}

Ipv4Address
Block::GetReceivedFromIpv4 (void) const
{
    return m_receivedFromIpv4;
}

void
Block::SetReceivedFromIpv4 (Ipv4Address receivedFromIpv4)
{
    m_receivedFromIpv4 = receivedFromIpv4;
}

CasperState
Block::GetCasperState (void) const
{
    return m_casperState;
}

void
Block::SetCasperState(enum CasperState state) {
    m_casperState = state;
}

rapidjson::Document Block::ToJSON() {
    rapidjson::Document block;
    rapidjson::Value value;

    block.SetObject();

    value = BLOCK;
    block.AddMember("message", value, block.GetAllocator());

    value.SetString("compressed-block");
    block.AddMember("type", value, block.GetAllocator());

    value = GetBlockId ();
    block.AddMember("blockId", value, block.GetAllocator ());

    value = GetBlockHeight ();
    block.AddMember("height", value, block.GetAllocator ());

    value = GetMinerId ();
    block.AddMember("minerId", value, block.GetAllocator ());

    value = GetParentBlockMinerId ();
    block.AddMember("parentBlockMinerId", value, block.GetAllocator ());

    value = GetBlockSizeBytes ();
    block.AddMember("size", value, block.GetAllocator ());

    value = GetTimeCreated ();
    block.AddMember("timeCreated", value, block.GetAllocator ());

    value = GetTimeReceived ();
    block.AddMember("timeReceived", value, block.GetAllocator ());

    value = GetTimeReceived ();
    block.AddMember("timeReceived", value, block.GetAllocator ());

    if(GetBlockProposalIteration() != 0) {
        value = GetBlockProposalIteration();
        block.AddMember("blockProposalIteration", value, block.GetAllocator());
    }

    if(GetVrfSeed() != 0) {
        value = GetVrfSeed();
        block.AddMember("vrfSeed", value, block.GetAllocator());
    }

    unsigned char zeroPK[64];
    memset(zeroPK, 0, 64);
    int cmp = memcmp(zeroPK, GetParticipantPublicKey(), 32);
    if(cmp != 0) {
        value.SetString((const char*) m_participantPublicKey, 32, block.GetAllocator());
        block.AddMember("participantPublicKey", value, block.GetAllocator());
    }

    unsigned char zeroVrfOut[64];
    memset(zeroVrfOut, 0, 64);
    cmp = memcmp(zeroVrfOut, GetVrfOutput(), 64);
    if(cmp != 0) {
//        std::string strOutput(m_vrfOutput, m_vrfOutput + sizeof m_vrfOutput / sizeof m_vrfOutput[0] );
//        const char* C1 = strOutput.c_str();
//        char* S1 = reinterpret_cast<char*>(m_vrfOutput);
//        const char* C1 = reinterpret_cast<const char*>(m_vrfOutput);
        value.SetString((const char*) m_vrfOutput, 64, block.GetAllocator());
        block.AddMember("vrfOutput", value, block.GetAllocator());
    }


    return block;
}

Block Block::FromJSON(rapidjson::Document *document, Ipv4Address receivedFrom) {
    Block block (
            (*document)["height"].GetInt(),
            (*document)["minerId"].GetInt(),
            (*document)["parentBlockMinerId"].GetInt(),
            (*document)["size"].GetInt(),
            (*document)["timeCreated"].GetDouble(),
            (*document)["timeReceived"].GetDouble(),
            receivedFrom
    );

    // Algorand values
    if((*document).HasMember("blockProposalIteration"))
        block.SetBlockProposalIteration((*document)["blockProposalIteration"].GetInt());

    if((*document).HasMember("vrfSeed"))
        block.SetVrfSeed((*document)["vrfSeed"].GetUint());

    if((*document).HasMember("blockId"))
        block.SetBlockId((*document)["blockId"].GetInt());

    if((*document).HasMember("participantPublicKey")){
        unsigned char participantPublicKey[32];
        memcpy(participantPublicKey, (*document)["participantPublicKey"].GetString(), sizeof participantPublicKey);
        block.SetParticipantPublicKey(participantPublicKey);
    }

    if((*document).HasMember("vrfOutput")){
        unsigned char vrfOutput[64];
        memset(vrfOutput, 0, sizeof vrfOutput);
        memcpy(vrfOutput, (*document)["vrfOutput"].GetString(), sizeof vrfOutput);
        block.SetVrfOutput(vrfOutput);
    }
    // end of Algorand values


    return block;
}

bool
Block::IsParent(const Block &block, enum Cryptocurrency currency) const
{
    if (GetBlockHeight() == block.GetBlockHeight() - 1 && GetMinerId() == block.GetParentBlockMinerId())
        return true;
    else if (currency != BITCOIN && currency != LITECOIN && currency != DOGECOIN
            && GetBlockHeight() == block.GetBlockHeight() - 1)
        return true;
    else
        return false;
}

bool
Block::IsChild(const Block &block, enum Cryptocurrency currency) const
{
    if (GetBlockHeight() == block.GetBlockHeight() + 1 && GetParentBlockMinerId() == block.GetMinerId())
        return true;
    else if (currency != BITCOIN && currency != LITECOIN && currency != DOGECOIN
             && GetBlockHeight() == block.GetBlockHeight() + 1)
        return true;
    else
        return false;
}


Block&
Block::operator= (const Block &blockSource)
{
    m_blockHeight = blockSource.m_blockHeight;
    m_minerId = blockSource.m_minerId;
    m_parentBlockMinerId = blockSource.m_parentBlockMinerId;
    m_blockSizeBytes = blockSource.m_blockSizeBytes;
    m_timeCreated = blockSource.m_timeCreated;
    m_timeReceived = blockSource.m_timeReceived;
    m_receivedFromIpv4 = blockSource.m_receivedFromIpv4;

    return *this;
}


/**
 *
 * Class Blockchain functions
 *
 */

Blockchain::Blockchain(void)
{
    m_noStaleBlocks = 0;
    m_totalBlocks = 0;
    m_totalFinalizedBlocks = 0;
    m_totalCheckpoints = 0;
    m_totalJustifiedCheckpoints = 0;
    m_totalFinalizedCheckpoints = 0;
    Block genesisBlock(0, -1, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    genesisBlock.SetBlockId(0);
    genesisBlock.SetCasperState(FINALIZED_CHKP);
    AddBlock(genesisBlock);
}

Blockchain::~Blockchain (void)
{
}

int
Blockchain::GetNoStaleBlocks (void) const
{
    return m_noStaleBlocks;
}


int
Blockchain::GetNoOrphans (void) const
{
    return m_orphans.size();
}

int
Blockchain::GetTotalBlocks (void) const
{
    return m_totalBlocks;
}

int
Blockchain::GetTotalFinalizedBlocks (void) const
{
    return m_totalFinalizedBlocks;
}

int
Blockchain::GetTotalCheckpoints (void) const
{
    return m_totalCheckpoints;
}

int
Blockchain::GetTotalFinalizedCheckpoints (void) const
{
    return m_totalFinalizedCheckpoints;
}

int
Blockchain::GetTotalJustifiedCheckpoints (void) const
{
    return m_totalJustifiedCheckpoints;
}


int
Blockchain::GetBlockchainHeight (void) const
{
    return GetCurrentTopBlock()->GetBlockHeight();
}

bool
Blockchain::HasBlock (const Block &newBlock) const
{

    if (newBlock.GetBlockHeight() > GetCurrentTopBlock()->GetBlockHeight())
    {
        /* The new block has a new blockHeight, so we haven't received it previously. */

        return false;
    }
    else
    {
        /*  The new block doesn't have a new blockHeight, so we have to check it is new or if we have already received it. */

        for (auto const &block: m_blocks[newBlock.GetBlockHeight()])
        {
            if (block == newBlock)
            {
                return true;
            }
        }
    }
    return false;
}

bool
Blockchain::HasBlock (int height, int minerId) const
{

    if (height > GetCurrentTopBlock()->GetBlockHeight())
    {
        /* The new block has a new blockHeight, so we haven't received it previously. */

        return false;
    }
    else
    {
        /*  The new block doesn't have a new blockHeight, so we have to check it is new or if we have already received it. */

        for (auto const &block: m_blocks[height])
        {
            if (block.GetBlockHeight() == height && block.GetMinerId() == minerId)
            {
                return true;
            }
        }
    }
    return false;
}


Block
Blockchain::ReturnBlock(int height, int minerId)
{
    std::vector<Block>::iterator  block_it;

    if (height <= GetBlockchainHeight() && height >= 0)
    {
        for (block_it = m_blocks[height].begin();  block_it < m_blocks[height].end(); block_it++)
        {
            if (block_it->GetBlockHeight() == height && block_it->GetMinerId() == minerId)
                return *block_it;
        }
    }

    for (block_it = m_orphans.begin();  block_it < m_orphans.end(); block_it++)
    {
        if (block_it->GetBlockHeight() == height && block_it->GetMinerId() == minerId)
            return *block_it;
    }

    return Block(-1, -1, -1, -1, -1, -1, Ipv4Address("0.0.0.0"));
}


bool
Blockchain::IsOrphan (const Block &newBlock) const
{
    for (auto const &block: m_orphans)
    {
        if (block == newBlock)
        {
            return true;
        }
    }
    return false;
}


bool
Blockchain::IsOrphan (int height, int minerId) const
{
    for (auto const &block: m_orphans)
    {
        if (block.GetBlockHeight() == height && block.GetMinerId() == minerId)
        {
            return true;
        }
    }
    return false;
}


const Block*
Blockchain::GetBlockPointer (const Block &newBlock) const
{
    if(m_blocks.size() <= newBlock.GetBlockHeight())
        return nullptr;

    for (auto const &block: m_blocks[newBlock.GetBlockHeight()])
    {
        if (block == newBlock)
        {
            return &block;
        }
    }
    return nullptr;
}


Block*
Blockchain::GetBlockPointerNonConst (const Block &newBlock)
{
    if(m_blocks.size() <= newBlock.GetBlockHeight())
        return nullptr;

    std::vector<Block>::iterator it;
    it = find (m_blocks[newBlock.GetBlockHeight()].begin(), m_blocks[newBlock.GetBlockHeight()].end(), newBlock);

    if(it == m_blocks[newBlock.GetBlockHeight()].end()){
        return nullptr;
    }else{
        return &(*it);
    }
}


const std::vector<const Block *>
Blockchain::GetChildrenPointers (const Block &block)
{
    std::vector<const Block *> children;
    std::vector<Block>::iterator  block_it;
    int childrenHeight = block.GetBlockHeight() + 1;

    if (childrenHeight > GetBlockchainHeight())
        return children;

    for (block_it = m_blocks[childrenHeight].begin();  block_it < m_blocks[childrenHeight].end(); block_it++)
    {
        if (block.IsParent(*block_it))
        {
            children.push_back(&(*block_it));
        }
    }
    return children;
}


const std::vector<const Block *>
Blockchain::GetOrphanChildrenPointers (const Block &newBlock)
{
    std::vector<const Block *> children;
    std::vector<Block>::iterator  block_it;

    for (block_it = m_orphans.begin();  block_it < m_orphans.end(); block_it++)
    {
        if (newBlock.IsParent(*block_it))
        {
            children.push_back(&(*block_it));
        }
    }
    return children;
}


const std::vector<const Block *>
Blockchain::GetAncestorsPointers (const Block &block, int lowestHeight)
{
    std::vector<const Block *> children;
    std::vector<Block>::iterator  block_it;
    const Block *lastBlock = &block;

    for(int childrenHeight = block.GetBlockHeight() - 1; childrenHeight >= lowestHeight; childrenHeight--) {
        for (block_it = m_blocks[childrenHeight].begin(); block_it < m_blocks[childrenHeight].end(); block_it++) {
            if ((*block_it).IsParent(*lastBlock)) {
                children.push_back(&(*block_it));
                lastBlock = &(*block_it);
            }
        }
    }

    return children;
}

std::vector<Block *>
Blockchain::GetAncestorsPointersNonConst(const Block &block, int lowestHeight) {
    std::vector<Block *> children;
    std::vector<Block>::iterator  block_it;
    Block *lastBlock = GetBlockPointerNonConst(block);

    for(int childrenHeight = block.GetBlockHeight() - 1; childrenHeight >= lowestHeight; childrenHeight--) {
        for (block_it = m_blocks[childrenHeight].begin(); block_it < m_blocks[childrenHeight].end(); block_it++) {
            if ((*block_it).IsParent(*lastBlock)) {
                children.push_back(&(*block_it));
                lastBlock = &(*block_it);
            }
        }
    }

    return children;
}

bool
Blockchain::IsAncestor(const Block *block, const Block *possibleAncestor)
{
    std::vector<const Block*> ancestors = GetAncestorsPointers(*block, possibleAncestor->GetBlockHeight());
    for (auto ancestor : ancestors) {
        if (*ancestor == *possibleAncestor) {
            return true;
        }
    }
    return false;
}


const std::vector<const Block *>
Blockchain::GetNotFinalizedCheckpoints(const Block &lastFinalizedCheckpoint)
{
    std::vector<const Block*> newCheckpoints;
    std::vector<Block>::iterator  block_it;

    // at first, insert pointer to the last finalized checkpoint
    const Block *finalized = GetBlockPointer(lastFinalizedCheckpoint);
    newCheckpoints.push_back(finalized);

    for(int childrenHeight = finalized->GetBlockHeight() + 1; childrenHeight <= GetBlockchainHeight(); childrenHeight++) {
        for (block_it = m_blocks[childrenHeight].begin(); block_it < m_blocks[childrenHeight].end(); block_it++) {
            // we are looking only for not justified and justified checkpoints.. last finalized is already in vector
            if(((*block_it).GetCasperState() == CHECKPOINT
               || (*block_it).GetCasperState() == JUSTIFIED_CHKP)
               && IsAncestor(&(*block_it), finalized)){
                newCheckpoints.push_back(&(*block_it));
            }
        }
    }

    // return pointers on checkpoints
    return newCheckpoints;
}


const Block*
Blockchain::GetParent (const Block &block)
{
    std::vector<Block>::iterator  block_it;
    int parentHeight = block.GetBlockHeight() - 1;

    if (parentHeight > GetBlockchainHeight() || parentHeight < 0)
        return nullptr;

    for (block_it = m_blocks[parentHeight].begin();  block_it < m_blocks[parentHeight].end(); block_it++)  {
        if (block.IsChild(*block_it))
        {
            return &(*block_it);
        }
    }

    return nullptr;
}


const Block*
Blockchain::GetCurrentTopBlock (void) const
{
    return &m_blocks[m_blocks.size() - 1][0];
}


void
Blockchain::AddBlock (const Block& newBlock)
{

    if (m_blocks.size() == 0)
    {
        std::vector<Block> newHeight(1, newBlock);
        m_blocks.push_back(newHeight);
    }
    else if (newBlock.GetBlockHeight() > GetCurrentTopBlock()->GetBlockHeight())
    {
        /**
         * The new block has a new blockHeight, so have to create a new vector (row)
         * If we receive an orphan block we have to create the dummy rows for the missing blocks as well
         */
        int dummyRows = newBlock.GetBlockHeight() - GetCurrentTopBlock()->GetBlockHeight() - 1;

        for(int i = 0; i < dummyRows; i++)
        {
            std::vector<Block> newHeight;
            m_blocks.push_back(newHeight);
        }

        std::vector<Block> newHeight(1, newBlock);
        m_blocks.push_back(newHeight);
    }
    else
    {
        /* The new block doesn't have a new blockHeight, so we have to add it in an existing row */

        if (m_blocks[newBlock.GetBlockHeight()].size() > 0)
            m_noStaleBlocks++;

        m_blocks[newBlock.GetBlockHeight()].push_back(newBlock);
    }

    m_totalBlocks++;
    UpdateCountOfBlocks(&newBlock, newBlock.GetCasperState(), true);
}


void
Blockchain::AddOrphan (const Block& newBlock)
{
    m_orphans.push_back(newBlock);
}


void
Blockchain::RemoveOrphan (const Block& newBlock)
{
    std::vector<Block>::iterator  block_it;

    for (block_it = m_orphans.begin();  block_it < m_orphans.end(); block_it++)
    {
        if (newBlock == *block_it)
            break;
    }

    if (block_it == m_orphans.end())
    {
        // name not in vector
        return;
    }
    else
    {
        m_orphans.erase(block_it);
    }
}


void
Blockchain::PrintOrphans (void)
{
    std::vector<Block>::iterator  block_it;

    std::cout << "The orphans are:\n";

    for (block_it = m_orphans.begin();  block_it < m_orphans.end(); block_it++)
    {
        std::cout << *block_it << "\n";
    }

    std::cout << "\n";
}


void Blockchain::PrintCheckpoints(void) {

    std::vector< std::vector<Block>>::iterator blockHeight_it;
    std::vector<Block>::iterator  block_it;

    int i;
    bool printedHeight = false;

    std::cout << "The checkpoints are:\n";

    for (blockHeight_it = m_blocks.begin(), i = 0; blockHeight_it < m_blocks.end(); blockHeight_it++, i++)
    {
        printedHeight = false;
        for (block_it = blockHeight_it->begin();  block_it < blockHeight_it->end(); block_it++)
        {
            if(   block_it->GetCasperState() == CHECKPOINT
               || block_it->GetCasperState() == JUSTIFIED_CHKP
               || block_it->GetCasperState() == FINALIZED_CHKP) {

                if(!printedHeight) {
                    std::cout << "  BLOCK HEIGHT " << i << ":\n";
                    printedHeight = true;
                }

                std::cout << *block_it << "\n";
            }
        }
    }

    std::cout << std::endl;
}


int
Blockchain::GetBlocksInForks (void)
{
    std::vector< std::vector<Block>>::iterator blockHeight_it;
    int count = 0;

    for (blockHeight_it = m_blocks.begin(); blockHeight_it < m_blocks.end(); blockHeight_it++)
    {
        if (blockHeight_it->size() > 1)
            count += blockHeight_it->size();
    }

    return count;
}


int
Blockchain::GetLongestForkSize (void)
{
    std::vector< std::vector<Block>>::iterator   blockHeight_it;
    std::vector<Block>::iterator                 block_it;
    std::map<int, int>                           forkedBlocksParentId;
    std::vector<int>                             newForks;
    std::vector<int>                             toEraseFromForkedBlocksParentId;
    int maxSize = 0;

    for (blockHeight_it = m_blocks.begin(); blockHeight_it < m_blocks.end(); blockHeight_it++)
    {

        if (blockHeight_it->size() > 1 && forkedBlocksParentId.size() == 0)
        {
            for (block_it = blockHeight_it->begin();  block_it < blockHeight_it->end(); block_it++)
            {
                forkedBlocksParentId[block_it->GetMinerId()] = 1;
            }
        }
        else if (blockHeight_it->size() > 1)
        {
            for (block_it = blockHeight_it->begin();  block_it < blockHeight_it->end(); block_it++)
            {
                std::map<int, int>::iterator mapIndex = forkedBlocksParentId.find(block_it->GetParentBlockMinerId());

                if(mapIndex != forkedBlocksParentId.end())
                {
                    forkedBlocksParentId[block_it->GetMinerId()] = mapIndex->second + 1;
                    if(block_it->GetMinerId() != mapIndex->first)
                        forkedBlocksParentId.erase(mapIndex);
                    newForks.push_back(block_it->GetMinerId());
                }
                else
                {
                    forkedBlocksParentId[block_it->GetMinerId()] = 1;
                }
            }

            for (auto &block : forkedBlocksParentId)
            {
                if (std::find(newForks.begin(), newForks.end(), block.first) == newForks.end() )
                {
                    if(block.second > maxSize) {
                        maxSize = block.second;
                    }
                    toEraseFromForkedBlocksParentId.push_back(block.first);
                }
            }

            for(auto block : toEraseFromForkedBlocksParentId){
                forkedBlocksParentId.erase(block);
            }
        }
        else if (blockHeight_it->size() == 1 && forkedBlocksParentId.size() > 0)
        {

            for (auto &block : forkedBlocksParentId)
            {
                if(block.second > maxSize)
                    maxSize = block.second;
            }

            forkedBlocksParentId.clear();
        }
    }

    for (auto &block : forkedBlocksParentId)
    {
        if(block.second > maxSize)
            maxSize = block.second;
    }

    return maxSize;
}

const Block*
Blockchain::CasperUpdateBlockchain(std::string source, std::string target, const Block *lastFinalizedCheckpoint, int maxBlocksInEpoch) {
    // searching checkpoints in blockchain
    std::string   delimiter = "/";
    size_t        sourcePos = source.find(delimiter);
    size_t        targetPos = target.find(delimiter);

    int sourceHeight = atoi(source.substr(0, sourcePos).c_str());
    int sourceMinerId = atoi(source.substr(sourcePos+1, source.size()).c_str());
    int targetHeight = atoi(target.substr(0, targetPos).c_str());
    int targetMinerId = atoi(target.substr(targetPos+1, target.size()).c_str());

    Block sourceTemplate(sourceHeight, sourceMinerId, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    Block targetTemplate(targetHeight, targetMinerId, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));

    Block* sourceBlock = GetBlockPointerNonConst (sourceTemplate);
    Block* targetBlock = GetBlockPointerNonConst (targetTemplate);

    if(sourceBlock == nullptr){
        std::cerr << "SOURCE " << source << " is NULLPTR" << std::endl;
        std::cerr << "Voted (approved, confirmed, ...) block (checkpoint) is missing in voters chain. Reason is most probable that the block was not propagated via network to this voter. "
                     "Solution can be increasing average block creation interval or increasing max connections between nodes." << std::endl;
        PrintCheckpoints();
        Simulator::Stop();
        return nullptr;
    }
    if(targetBlock == nullptr){
        std::cerr << "TARGET "<< target <<" is NULLPTR" << std::endl;
        std::cerr << "Voted (approved, confirmed, ...) block (checkpoint) is missing in voters chain. Reason is most probable that the block was not propagated via network to this voter. "
                     "Solution can be increasing average block creation interval or increasing max connections between nodes." << std::endl;
        PrintCheckpoints();
        Simulator::Stop();
        return nullptr;
    }

    // committee was not able to find new justified checkpoint
    if(targetBlock->GetCasperState() == FINALIZED_CHKP)
        return nullptr;

    UpdateCountOfBlocks(targetBlock, JUSTIFIED_CHKP);
    targetBlock->SetCasperState(JUSTIFIED_CHKP);

    return CasperUpdateFinalized(lastFinalizedCheckpoint, maxBlocksInEpoch);
}

const Block*
Blockchain::CasperUpdateFinalized(const Block *lastFinalizedCheckpoint, int maxBlocksInEpoch) {
    // find new not finalized yet checkpoints
    std::vector<const Block*> newCheckpoints = GetNotFinalizedCheckpoints(*lastFinalizedCheckpoint);

    std::vector<const Block*> shouldBeFinalized;
    for(int i = 0; i < newCheckpoints.size(); i++){
        const Block* checkpointA = newCheckpoints.at(i);
        if(checkpointA->GetCasperState() != JUSTIFIED_CHKP)
            continue;
        for(int j = i+1; j < newCheckpoints.size(); j++){
            const Block* checkpointB = newCheckpoints.at(j);
            if(checkpointB->GetCasperState() != JUSTIFIED_CHKP)
                continue;

            // get order of checkpoints
            const Block* higher = checkpointA;
            const Block* lower = checkpointB;
            if(checkpointA->GetBlockHeight() < checkpointB->GetBlockHeight()) {
                higher = checkpointB;
                lower = checkpointA;
            }

            // check if height difference between checkpoints is equal to m_maxBlocksInEpoch
            if((higher->GetBlockHeight() - lower->GetBlockHeight()) != maxBlocksInEpoch)
                continue;

            // check if lower checkpoint is ancestor of higher checkpoint
            if(IsAncestor(higher, lower))
                shouldBeFinalized.push_back(lower);
        }
    }

    if(shouldBeFinalized.size() == 0)
        return nullptr;

    // get highest from shouldBeFinalized checkpoints
    // (always should be only one, but maybe some node forget to update state)
    const Block* newlyFinalized = shouldBeFinalized.at(0);
    int height = newlyFinalized->GetBlockHeight();
    for(auto checkpoint : shouldBeFinalized){
        if(checkpoint->GetBlockHeight() > height){
            newlyFinalized = checkpoint;
            height = checkpoint->GetBlockHeight();
        }
    }

    // update state of checkpoint and blocks
    Block* newlyFinalizedNonConst = GetBlockPointerNonConst (*newlyFinalized);
    UpdateCountOfBlocks(newlyFinalizedNonConst, FINALIZED_CHKP);
    newlyFinalizedNonConst->SetCasperState(FINALIZED_CHKP);


    std::vector<Block*> ancestors = GetAncestorsPointersNonConst(*newlyFinalized, lastFinalizedCheckpoint->GetBlockHeight());
    for(auto block : ancestors){
        if(block->GetCasperState() == STD_BLOCK) {
            UpdateCountOfBlocks(block, FINALIZED);
            block->SetCasperState(FINALIZED);
        }
        else if(block->GetCasperState() == JUSTIFIED_CHKP || block->GetCasperState() == CHECKPOINT) {
            UpdateCountOfBlocks(block, FINALIZED_CHKP);
            block->SetCasperState(FINALIZED_CHKP);
        }
    }

    return newlyFinalized;
}

void
Blockchain::GasperUpdateEpochBoundaryCheckpoints(const Block *lastFinalizedCheckpoint, int maxBlocksInEpoch)
{
    // at first, insert pointer to the last finalized checkpoint
    const Block *finalized = GetBlockPointer(*lastFinalizedCheckpoint);

    std::vector<Block>::iterator  block_it;
    for(int childrenHeight = finalized->GetBlockHeight() + 1; childrenHeight <= GetBlockchainHeight(); childrenHeight++) {
        for (block_it = m_blocks[childrenHeight].begin(); block_it < m_blocks[childrenHeight].end(); block_it++) {
            // we are not looking for checkpoints
            if(((*block_it).GetCasperState() == CHECKPOINT
                || (*block_it).GetCasperState() == JUSTIFIED_CHKP))
            {
                continue;
            }

            // we are looking for successors of finalized block
           if(IsAncestor(&(*block_it), finalized))
           {
               const std::vector<const Block *> children = GetChildrenPointers(*block_it);
               if(children.size() < 2)
                   continue;

               bool childInOtherEpoch = false;
               int parentEpoch = block_it->GetBlockProposalIteration() / maxBlocksInEpoch;
               for (auto child : children){
                   if((child->GetCasperState() == CHECKPOINT
                       || child->GetCasperState() == JUSTIFIED_CHKP))
                   {
                       continue;    // child is already a checkpoint
                   }

                   // check if child is in other epoch than parent
                   if(parentEpoch != (child->GetBlockProposalIteration() / maxBlocksInEpoch)) {
                       childInOtherEpoch = true;
                       break;
                   }
               }

               // if block have at least one not checkpoint child in other epoch, than set this block state to checkpoint
               if(childInOtherEpoch)
                   UpdateCountOfBlocks(&(*block_it), CHECKPOINT);
                   block_it->SetCasperState(CHECKPOINT);
           }
        }
    }
}

void
Blockchain::UpdateCountOfBlocks(const Block *block, CasperState newState, bool add) {
    if(!add) {
        switch (block->GetCasperState()) {
            case STD_BLOCK:
                break;
            case FINALIZED:
                m_totalFinalizedBlocks--;
                break;
            case CHECKPOINT:
                m_totalCheckpoints--;
                break;
            case JUSTIFIED_CHKP:
                m_totalJustifiedCheckpoints--;
                break;
            case FINALIZED_CHKP:
                m_totalFinalizedBlocks--;
                m_totalFinalizedCheckpoints--;
                break;
        }
    }
    switch (newState) {
        case STD_BLOCK:
            break;
        case FINALIZED:
            m_totalFinalizedBlocks++;
            break;
        case CHECKPOINT:
            m_totalCheckpoints++;
            break;
        case JUSTIFIED_CHKP:
            m_totalJustifiedCheckpoints++;
            break;
        case FINALIZED_CHKP:
            m_totalFinalizedBlocks++;
            m_totalFinalizedCheckpoints++;
            break;
    }
}


bool operator== (const Block &block1, const Block &block2)
{
    if (block1.GetBlockHeight() == block2.GetBlockHeight() && block1.GetMinerId() == block2.GetMinerId())
        return true;
    else
        return false;
}

std::ostream& operator<< (std::ostream &out, const Block &block)
{

    out << "(m_blockId: " << block.GetBlockId() << ", " <<
        "m_blockHeight: " << block.GetBlockHeight() << ", " <<
        "m_blockHash: " << block.GetBlockHash() << ", " <<
        "m_casperState: " << (int)block.GetCasperState() << ", " <<
        "m_minerId: " << block.GetMinerId() << ", " <<
        "m_parentBlockMinerId: " << block.GetParentBlockMinerId() << ", " <<
        "m_blockSizeBytes: " << block.GetBlockSizeBytes() << ", " <<
        "m_timeCreated: " << block.GetTimeCreated() << ", " <<
        "m_timeReceived: " << block.GetTimeReceived() << ", " <<
        "m_blockProposalIteration: " << block.GetBlockProposalIteration() << ", " <<
        "m_receivedFromIpv4: " << block.GetReceivedFromIpv4() <<
        ")";
    return out;
}

std::ostream& operator<< (std::ostream &out, Blockchain &blockchain)
{

    std::vector< std::vector<Block>>::iterator blockHeight_it;
    std::vector<Block>::iterator  block_it;
    int i;

    for (blockHeight_it = blockchain.m_blocks.begin(), i = 0; blockHeight_it < blockchain.m_blocks.end(); blockHeight_it++, i++)
    {
        out << "  BLOCK HEIGHT " << i << ":\n";
        for (block_it = blockHeight_it->begin();  block_it < blockHeight_it->end(); block_it++)
        {
            out << *block_it << "\n";
        }
    }

    return out;
}


const char* getBlockBroadcastType(enum BlockBroadcastType m)
{
    switch (m)
    {
        case STANDARD: return "STANDARD";
        case UNSOLICITED: return "UNSOLICITED";
        case RELAY_NETWORK: return "RELAY_NETWORK";
        case UNSOLICITED_RELAY_NETWORK: return "UNSOLICITED_RELAY_NETWORK";
    }
}

const char* getProtocolType(enum ProtocolType m)
{
    switch (m)
    {
        case STANDARD_PROTOCOL: return "STANDARD_PROTOCOL";
        case SENDHEADERS: return "SENDHEADERS";
    }
}

const char* getBitcoinRegion(enum BitcoinRegion m)
{
    switch (m)
    {
        case ASIA_PACIFIC: return "ASIA_PACIFIC";
        case AUSTRALIA: return "AUSTRALIA";
        case EUROPE: return "EUROPE";
        case JAPAN: return "JAPAN";
        case NORTH_AMERICA: return "NORTH_AMERICA";
        case SOUTH_AMERICA: return "SOUTH_AMERICA";
        case OTHER: return "OTHER";
    }
}

const char* getMessageName(enum Messages m)
{
    switch (m)
    {
        case INV: return "INV";
        case GET_HEADERS: return "GET_HEADERS";
        case HEADERS: return "HEADERS";
        case GET_BLOCKS: return "GET_BLOCKS";
        case BLOCK: return "BLOCK";
        case GET_DATA: return "GET_DATA";
        case NO_MESSAGE: return "NO_MESSAGE";
        case EXT_INV: return "EXT_INV";
        case EXT_GET_HEADERS: return "EXT_GET_HEADERS";
        case EXT_HEADERS: return "EXT_HEADERS";
        case EXT_GET_BLOCKS: return "EXT_GET_BLOCKS";
        case CHUNK: return "CHUNK";
        case EXT_GET_DATA: return "EXT_GET_DATA";
        // algorand messages
        case BLOCK_PROPOSAL: return "BLOCK_PROPOSAL";
        case SOFT_VOTE: return "SOFT_VOTE";
        case CERTIFY_VOTE: return "CERTIFY_VOTE";
        // casper messages
        case CASPER_VOTE: return "CASPER_VOTE";
    }
}


enum BitcoinRegion getBitcoinEnum(uint32_t n)
{
    switch (n)
    {
        case 0: return NORTH_AMERICA;
        case 1: return EUROPE;
        case 2: return SOUTH_AMERICA;
        case 3: return ASIA_PACIFIC;
        case 4: return JAPAN;
        case 5: return AUSTRALIA;
        case 6: return OTHER;
    }
}
}// Namespace ns3