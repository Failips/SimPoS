
/**
 * This file contains declares the GasperParticipantHelper class.
 * @author failips (xborci01@fit.stud.vutbr.cz / mak100@azet.sk)
 * @date 28. 1. 2021
 */

#ifndef SIMPOS_GASPER_PARTICIPANT_HELPER_H
#define SIMPOS_GASPER_PARTICIPANT_HELPER_H

#include "ns3/bitcoin-node-helper.h"
#include <vector>
#include <random>

namespace ns3 {

/**
 * Based on bitcoin-miner-helper
 */

class GasperParticipantHelper : public BitcoinNodeHelper {
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

    GasperParticipantHelper(std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                              std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                              double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats);

    enum MinerType GetMinerType(void);
    void SetMinerType (enum MinerType m);
    void SetBlockBroadcastType (enum BlockBroadcastType m);
    void SetAllPrint (bool allPrint);
    void SetVrfThresholdBP (unsigned char *threshold);
    void SetVrfThreshold (unsigned char *threshold);

    void SetIntervalBP (double interval);
    void SetIntervalAttest (double interval);


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

    enum MinerType              m_minerType;
    enum BlockBroadcastType     m_blockBroadcastType;
    int                         m_noMiners;
    bool                        m_allPrint;


    unsigned char m_genesisVrfSeed[32];         // genesis block vrf seed
    unsigned char m_vrfThresholdBP[64];         // threshold for Y value (VRF output value) in choosing leader of committee
    unsigned char m_vrfThreshold[64];           // threshold for Y value (VRF output value) in choosing committee members

    double m_intervalBP;            // Interval between block proposal phase and soft vote phase
    double m_intervalAttest;         // Interval between attest phase and block proposal phase

    std::mt19937 m_generator;
};

}// Namespace ns3


#endif //SIMPOS_GASPER_PARTICIPANT_HELPER_H
