/**
 * This file contains the definitions of the functions declared in casper-participant-helper.h
 */

#include "ns3/casper-participant-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/casper-participant.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("CasperParticipantHelper");

    CasperParticipantHelper::CasperParticipantHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                                            double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) :
                                            BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD)

    {
        m_factory.SetTypeId ("ns3::CasperParticipant");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
    }

    Ptr<Application>
    CasperParticipantHelper::InstallPriv (Ptr<Node> node) //FIX ME
    {

        switch (m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<CasperParticipant> app = m_factory.Create<CasperParticipant> ();
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
    CasperParticipantHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    CasperParticipantHelper::SetMinerType(enum MinerType m)
    {
        m_minerType = m;

        switch (m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId ("ns3::CasperParticipant");
                SetFactoryAttributes();
                break;
            }
        }
    }


    void
    CasperParticipantHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
    {
        m_blockBroadcastType = m;
    }


    void
    CasperParticipantHelper::SetFactoryAttributes (void)
    {
        m_factory.Set ("Protocol", StringValue (m_protocol));
        m_factory.Set ("Local", AddressValue (m_address));
    }

} // namespace ns3


