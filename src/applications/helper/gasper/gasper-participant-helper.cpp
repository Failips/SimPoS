/**
 * This file contains the definitions of the functions declared in gasper-participant-helper.h
 */

#include "gasper-participant-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/gasper-participant.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <algorithm>
#include "../../libsodium/include/sodium.h"


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("GasperParticipantHelper");

    GasperParticipantHelper::GasperParticipantHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                                            double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) :
                                            BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD)

    {
        m_factory.SetTypeId ("ns3::GasperParticipant");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
        m_noMiners = noMiners;

        std::random_device rd; // obtain a random number from hardware
        m_generator.seed(rd()); // seed the generator

        randombytes_buf(m_genesisVrfSeed, sizeof m_genesisVrfSeed);
    }

    Ptr<Application>
    GasperParticipantHelper::InstallPriv (Ptr<Node> node) //FIX ME
    {
        switch (m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<GasperParticipant> app = m_factory.Create<GasperParticipant> ();
                app->SetPeersAddresses(m_peersAddresses);
                app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
                app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
                app->SetNodeInternetSpeeds(m_internetSpeeds);
                app->SetNodeStats(m_nodeStats);
                app->SetBlockBroadcastType(m_blockBroadcastType);
                app->SetProtocolType(m_protocolType);

                app->SetHelper(this);

                app->SetAllPrint(m_allPrint);

                app->SetGenesisVrfSeed(m_genesisVrfSeed);
                app->SetVrfThresholdBP(m_vrfThresholdBP);
                app->SetVrfThreshold(m_vrfThreshold);

                app->SetIntervalBP(m_intervalBP);
                app->SetIntervalAttest(m_intervalAttest);

                node->AddApplication (app);
                return app;
            }
        }
    }

    enum MinerType
    GasperParticipantHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    GasperParticipantHelper::SetMinerType(enum MinerType m)
    {
        m_minerType = m;

        switch (m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId ("ns3::GasperParticipant");
                SetFactoryAttributes();
                break;
            }
        }
    }


    void
    GasperParticipantHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
    {
        m_blockBroadcastType = m;
    }

    void
    GasperParticipantHelper::SetAllPrint(bool allPrint)
    {
        m_allPrint = allPrint;
    }

    void
    GasperParticipantHelper::SetVrfThresholdBP(unsigned char *threshold) {
        memcpy(m_vrfThresholdBP, threshold, sizeof m_vrfThresholdBP);
    }

    void
    GasperParticipantHelper::SetVrfThreshold(unsigned char *threshold) {
        memcpy(m_vrfThreshold, threshold, sizeof m_vrfThreshold);
    }

    void
    GasperParticipantHelper::SetIntervalBP(double interval) {
        m_intervalBP = interval;
    }

    void
    GasperParticipantHelper::SetIntervalAttest(double interval) {
        m_intervalAttest = interval;
    }

    void
    GasperParticipantHelper::SetFactoryAttributes (void)
    {
        m_factory.Set ("Protocol", StringValue (m_protocol));
        m_factory.Set ("Local", AddressValue (m_address));
    }


} // namespace ns3


