/**
 * GasperNode class declares
 */

#ifndef SIMPOS_GASPER_NODE_H
#define SIMPOS_GASPER_NODE_H

#include "ns3/bitcoin-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class GasperNode : public BitcoinNode {
public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    GasperNode (void);

    virtual ~GasperNode (void);

protected:
    // inherited from Application base class via BitcoinNode class.
    virtual void StartApplication (void);    // Called at time specified by Start

    Ptr<Socket> m_broadcastSocket;

};

} // ns3 namespace

#endif //SIMPOS_GASPER_NODE_H
