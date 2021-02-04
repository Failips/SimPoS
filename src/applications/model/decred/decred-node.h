/**
 * DecredNode class declares
 */

#ifndef SIMPOS_DECRED_NODE_H
#define SIMPOS_DECRED_NODE_H

#include "ns3/bitcoin-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class DecredNode : public BitcoinNode {
public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    DecredNode (void);

    virtual ~DecredNode (void);

};

} // ns3 namespace

#endif //SIMPOS_DECRED_NODE_H
