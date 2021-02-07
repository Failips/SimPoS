/**
 * This file contains all the necessary enumerations and structs used throughout the simulation.
 * It also defines 3 very important classed; the Block, Chunk and Blockchain.
 */


#ifndef BITCOIN_H
#define BITCOIN_H

#include <vector>
#include <map>
#include "ns3/address.h"
#include <algorithm>
#include "ns3/blockchain.h"

namespace ns3 {


/**
 * The bitcoin miner types that have been implemented. The first one is the normal miner (default), the last 3 are used to simulate different attacks.
 */
enum MinerType
{
  NORMAL_MINER,                //DEFAULT
  SIMPLE_ATTACKER,
  SELFISH_MINER,
  SELFISH_MINER_TRIALS
};


const char* getMinerType(enum MinerType m);
const char* getCryptocurrency(enum Cryptocurrency m);

class BitcoinChunk : public Block
{
public:
  BitcoinChunk (int blockHeight, int minerId, int chunkId, int parentBlockMinerId = 0, int blockSizeBytes = 0, 
                double timeCreated = 0, double timeReceived = 0, Ipv4Address receivedFromIpv4 = Ipv4Address("0.0.0.0"));
  BitcoinChunk ();
  BitcoinChunk (const BitcoinChunk &chunkSource);  // Copy constructor
  virtual ~BitcoinChunk (void);
 
  int GetChunkId (void) const;
  void SetChunkId (int minerId);
  
  BitcoinChunk& operator= (const BitcoinChunk &chunkSource); //Assignment Constructor
  
  friend bool operator== (const BitcoinChunk &chunk, const BitcoinChunk &chunk2);
  friend bool operator< (const BitcoinChunk &chunk, const BitcoinChunk &chunk2);
  friend std::ostream& operator<< (std::ostream &out, const BitcoinChunk &chunk);
  
protected:	
  int           m_chunkId;

};


}// Namespace ns3

#endif /* BITCOIN_H */
