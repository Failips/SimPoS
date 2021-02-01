/**
 * This file contains the definitions of the functions declared in bitcoin.h
 */


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/log.h"
#include "bitcoin.h"

namespace ns3 {

/**
 *
 * Class BitcoinChunk functions
 *
 */
 
BitcoinChunk::BitcoinChunk(int blockHeight, int minerId, int chunkId, int parentBlockMinerId, int blockSizeBytes, 
             double timeCreated, double timeReceived, Ipv4Address receivedFromIpv4) :  
             Block (blockHeight, minerId, parentBlockMinerId, blockSizeBytes, 
                    timeCreated, timeReceived, receivedFromIpv4)
{  
  m_chunkId = chunkId;
}

BitcoinChunk::BitcoinChunk()
{  
  BitcoinChunk(0, 0, 0, 0, 0, 0, 0, Ipv4Address("0.0.0.0"));
}

BitcoinChunk::BitcoinChunk (const BitcoinChunk &chunkSource)
{  
  m_blockHeight = chunkSource.m_blockHeight;
  m_minerId = chunkSource.m_minerId;
  m_chunkId = chunkSource.m_chunkId;
  m_parentBlockMinerId = chunkSource.m_parentBlockMinerId;
  m_blockSizeBytes = chunkSource.m_blockSizeBytes;
  m_timeCreated = chunkSource.m_timeCreated;
  m_timeReceived = chunkSource.m_timeReceived;
  m_receivedFromIpv4 = chunkSource.m_receivedFromIpv4;

}

BitcoinChunk::~BitcoinChunk (void)
{
}

int 
BitcoinChunk::GetChunkId (void) const
{
  return m_chunkId;
}

void
BitcoinChunk::SetChunkId (int chunkId)
{
  m_chunkId = chunkId;
}

BitcoinChunk& 
BitcoinChunk::operator= (const BitcoinChunk &chunkSource)
{  
  m_blockHeight = chunkSource.m_blockHeight;
  m_minerId = chunkSource.m_minerId;
  m_chunkId = chunkSource.m_chunkId;
  m_parentBlockMinerId = chunkSource.m_parentBlockMinerId;
  m_blockSizeBytes = chunkSource.m_blockSizeBytes;
  m_timeCreated = chunkSource.m_timeCreated;
  m_timeReceived = chunkSource.m_timeReceived;
  m_receivedFromIpv4 = chunkSource.m_receivedFromIpv4;

  return *this;
}


bool operator== (const BitcoinChunk &chunk1, const BitcoinChunk &chunk2)
{
    if (chunk1.GetBlockHeight() == chunk2.GetBlockHeight() && chunk1.GetMinerId() == chunk2.GetMinerId() && chunk1.GetChunkId() == chunk2.GetChunkId())
        return true;
    else
        return false;
}

bool operator< (const BitcoinChunk &chunk1, const BitcoinChunk &chunk2)
{
    if (chunk1.GetBlockHeight() < chunk2.GetBlockHeight())
        return true;
    else if (chunk1.GetBlockHeight() == chunk2.GetBlockHeight() && chunk1.GetMinerId() < chunk2.GetMinerId())
        return true;
    else if (chunk1.GetBlockHeight() == chunk2.GetBlockHeight() && chunk1.GetMinerId() == chunk2.GetMinerId() && chunk1.GetChunkId() < chunk2.GetChunkId())
        return true;
    else
        return false;
}

std::ostream& operator<< (std::ostream &out, const BitcoinChunk &chunk)
{

    out << "(m_blockHeight: " << chunk.GetBlockHeight() << ", " <<
        "m_minerId: " << chunk.GetMinerId() << ", " <<
        "chunkId: " << chunk.GetChunkId() << ", " <<
        "m_parentBlockMinerId: " << chunk.GetParentBlockMinerId() << ", " <<
        "m_blockSizeBytes: " << chunk.GetBlockSizeBytes() << ", " <<
        "m_timeCreated: " << chunk.GetTimeCreated() << ", " <<
        "m_timeReceived: " << chunk.GetTimeReceived() << ", " <<
        "m_receivedFromIpv4: " << chunk.GetReceivedFromIpv4() <<
        ")";
    return out;
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
  }
}

const char* getMinerType(enum MinerType m)
{
  switch (m) 
  {
    case NORMAL_MINER: return "NORMAL_MINER";
    case SIMPLE_ATTACKER: return "SIMPLE_ATTACKER";
    case SELFISH_MINER: return "SELFISH_MINER";
    case SELFISH_MINER_TRIALS: return "SELFISH_MINER_TRIALS";
  }
}

const char* getCryptocurrency(enum Cryptocurrency m)
{
  switch (m) 
  {
    case BITCOIN: return "BITCOIN";
    case LITECOIN: return "LITECOIN";
    case DOGECOIN: return "DOGECOIN";
  }
}
}// Namespace ns3
