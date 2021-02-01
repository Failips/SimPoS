/**
 * This file contains the definitions of the functions declared in algorand-participant-helper.h
 */

#include "algorand-participant-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/bitcoin-miner.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

    AlgorandParticipantHelper::AlgorandParticipantHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                                            double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) :
            BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD),
            m_secureBlocks (6), m_blockGenBinSize (-1), m_blockGenParameter (-1)
    {
        m_factory.SetTypeId ("ns3::AlgorandParticipant");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
    }

    Ptr<Application>
    AlgorandParticipantHelper::InstallPriv (Ptr<Node> node) //FIX ME
    {

        switch (m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<BitcoinMiner> app = m_factory.Create<BitcoinMiner> ();
                app->SetPeersAddresses(m_peersAddresses);
                app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
                app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
                app->SetNodeInternetSpeeds(m_internetSpeeds);
                app->SetNodeStats(m_nodeStats);
                app->SetBlockBroadcastType(m_blockBroadcastType);
                app->SetProtocolType(m_protocolType);

                node->AddApplication (app);
                return app;
            }

        }

    }

    enum MinerType
    AlgorandParticipantHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    AlgorandParticipantHelper::SetMinerType (enum MinerType m)  //FIX ME
    {
        m_minerType = m;

        switch (m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId ("ns3::AlgorandParticipant");
                SetFactoryAttributes();
                break;
            }
        }
    }


    void
    AlgorandParticipantHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
    {
        m_blockBroadcastType = m;
    }


    void
    AlgorandParticipantHelper::SetFactoryAttributes (void)
    {
        m_factory.Set ("Protocol", StringValue (m_protocol));
        m_factory.Set ("Local", AddressValue (m_address));
    }

} // namespace ns3


