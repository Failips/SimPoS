/**
 * CasperNode class declares
 */

#ifndef SIMPOS_CASPER_NODE_H
#define SIMPOS_CASPER_NODE_H

#include "ns3/bitcoin-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class CasperNode : public BitcoinNode {
public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    CasperNode (void);

    virtual ~CasperNode (void);

};

} // ns3 namespace

#endif //SIMPOS_CASPER_NODE_H
