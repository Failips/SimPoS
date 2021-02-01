/**
 * AlgorandParticipant class declarations
 * @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
 */

#ifndef SIMPOS_ALGORANDPARTICIPANT_H
#define SIMPOS_ALGORANDPARTICIPANT_H

#include "algorand-node.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class AlgorandParticipant : public AlgorandNode {
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId (void);
    AlgorandParticipant ();

    virtual ~AlgorandParticipant (void);

protected:


    //debug
    double       m_timeStart;
    double       m_timeFinish;
};

} // ns3 namespace

#endif //SIMPOS_ALGORANDPARTICIPANT_H
