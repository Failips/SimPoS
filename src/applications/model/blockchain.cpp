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

    value = GetBlockProposalIteration ();
    block.AddMember("blockProposalIteration", value, block.GetAllocator ());

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

    if((*document).HasMember("blockProposalIteration"))
        block.SetBlockProposalIteration((*document)["blockProposalIteration"].GetInt());

    if((*document).HasMember("blockId"))
        block.SetBlockId((*document)["blockId"].GetInt());

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
    Block genesisBlock(0, -1, -2, 0, 0, 0, Ipv4Address("0.0.0.0"));
    genesisBlock.SetBlockId(0);
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

    for (auto const &block: m_blocks[newBlock.GetBlockHeight()])
    {
        if (block == newBlock)
        {
            return &block;
        }
    }
    return nullptr;
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
                    if(block.second > maxSize)
                        maxSize = block.second;
                    forkedBlocksParentId.erase(block.first);
                }
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
            // algrand messages
        case BLOCK_PROPOSAL: return "BLOCK_PROPOSAL";
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