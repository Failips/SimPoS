/**
 * This file contains the definitions of the functions declared in decred-participant-helper.h
 */

#include "ns3/decred-participant-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/decred-participant.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

    DecredParticipantHelper::DecredParticipantHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                                            double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) :
                                            BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD)

    {
        m_factory.SetTypeId ("ns3::DecredParticipant");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
    }

    Ptr<Application>
    DecredParticipantHelper::InstallPriv (Ptr<Node> node) //FIX ME
    {

        switch (m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<DecredParticipant> app = m_factory.Create<DecredParticipant> ();
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
    DecredParticipantHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    DecredParticipantHelper::SetMinerType(enum MinerType m)
    {
        m_minerType = m;

        switch (m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId ("ns3::DecredParticipant");
                SetFactoryAttributes();
                break;
            }
        }
    }


    void
    DecredParticipantHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
    {
        m_blockBroadcastType = m;
    }


    void
    DecredParticipantHelper::SetFactoryAttributes (void)
    {
        m_factory.Set ("Protocol", StringValue (m_protocol));
        m_factory.Set ("Local", AddressValue (m_address));
    }

} // namespace ns3


