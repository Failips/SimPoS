/**
 * AlgorandNode class declares
 */

#ifndef SIMPOS_ALGORAND_NODE_H
#define SIMPOS_ALGORAND_NODE_H

#include "ns3/bitcoin-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class AlgorandNode : public BitcoinNode {
public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    AlgorandNode (void);

    virtual ~AlgorandNode (void);

protected:
    // inherited from Application base class via BitcoinNode class.
    virtual void StartApplication (void);    // Called at time specified by Start

    Ptr<Socket> m_broadcastSocket;

};

} // ns3 namespace

#endif //SIMPOS_ALGORAND_NODE_H
