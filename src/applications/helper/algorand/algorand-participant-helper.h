
/**
 * This file contains declares the AlgorandParticipantHelper class.
 * @author failips (xborci01@fit.stud.vutbr.cz / mak100@azet.sk)
 * @date 28. 1. 2021
 */

#ifndef SIMPOS_ALGORAND_PARTICIPANT_HELPER_H
#define SIMPOS_ALGORAND_PARTICIPANT_HELPER_H

#include "ns3/bitcoin-node-helper.h"
#include <vector>
#include <random>

namespace ns3 {

/**
 * Based on bitcoin-miner-helper
 */

class AlgorandParticipantHelper : public BitcoinNodeHelper {
    public:
    /**
     * Create a PacketSinkHelper to make it easier to work with BitcoinMiner applications
     *
     * \param protocol the name of the protocol to use to receive traffic
     *        This string identifies the socket factory type used to create
     *        sockets for the applications.  A typical value would be
     *        ns3::TcpSocketFactory.
     * \param address the address of the bitcoin node
     * \param peers a reference to a vector containing the Ipv4 addresses of peers of the bitcoin node
     * \param noMiners total number of miners in the simulation
     * \param peersDownloadSpeeds a map containing the download speeds of the peers of the node
     * \param peersUploadSpeeds a map containing the upload speeds of the peers of the node
     * \param internetSpeeds a reference to a struct containing the internet speeds of the node
     * \param stats a pointer to struct holding the node statistics
     */

    AlgorandParticipantHelper(std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                              std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                              double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats);

    enum MinerType GetMinerType(void);
    void SetMinerType (enum MinerType m);
    void SetBlockBroadcastType (enum BlockBroadcastType m);

    /**
     * Pseudo VRF function for validation if participant is allowed for block proposal or soft vote in certain Algorand iteration phase
     * @param iteration iteration number
     * @param participantId id of Algorand participant
     * @return true if participant is chosen by VRF, false otherwise
     */
    bool IsChosenByVRF(int iteration, int participantId, enum AlgorandPhase algorandPhase);

    /**
     * returns committee size in the iteration of algorand phase
     * @param iteration iteration number
     * @param algorandPhase phase of Algorand process (blockProposal, soft vote, certify vote)
     * @return size of algorand phase committee size
     */
    int GetCommitteeSize(int iteration, enum AlgorandPhase algorandPhase);

protected:
    /**
     * Install an ns3::PacketSink on the node configured with all the
     * attributes set with SetAttribute.
     *
     * \param node The node on which an PacketSink will be installed.
     * \returns Ptr to the application installed.
     */
    virtual Ptr<Application> InstallPriv (Ptr<Node> node);

    /**
     * Customizes the factory object according to the arguments of the constructor
     */
    void SetFactoryAttributes (void);

    void CreateCommittee(int iteration, enum AlgorandPhase algorandPhase);

    enum MinerType              m_minerType;
    enum BlockBroadcastType     m_blockBroadcastType;
    int                         m_noMiners;

    std::mt19937 m_generator;
    std::poisson_distribution<int> m_committeeSizeDistribution;
    std::uniform_int_distribution<int> m_memberDistribution;
    std::vector<std::vector<int>> m_committeeBP;
    std::vector<std::vector<int>> m_committeeSV;
};

}// Namespace ns3


#endif //SIMPOS_ALGORAND_PARTICIPANT_HELPER_H
