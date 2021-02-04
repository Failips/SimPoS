/**
 * CasperNode class implementation. Extends BitcoinNode.
 */

#include "ns3/casper-node.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CasperNode");

NS_OBJECT_ENSURE_REGISTERED (CasperNode);

TypeId
CasperNode::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::CasperNode")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<CasperNode> ()
            .AddAttribute ("Local",
                           "The Address on which to Bind the rx socket.",
                           AddressValue (),
                           MakeAddressAccessor (&CasperNode::m_local),
                           MakeAddressChecker ())
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&CasperNode::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                           "Enable the BlockTorrent protocol",
                           BooleanValue (false),
                           MakeBooleanAccessor (&CasperNode::m_blockTorrent),
                           MakeBooleanChecker ())
            .AddAttribute ("InvTimeoutMinutes",
                           "The timeout of inv messages in minutes",
                           TimeValue (Minutes (20)),
                           MakeTimeAccessor (&CasperNode::m_invTimeoutMinutes),
                           MakeTimeChecker())
            .AddTraceSource ("Rx",
                             "A packet has been received",
                             MakeTraceSourceAccessor (&CasperNode::m_rxTrace),
                             "ns3::Packet::AddressTracedCallback")
    ;
    return tid;
}

CasperNode::CasperNode (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_meanBlockReceiveTime = 0;
    m_previousBlockReceiveTime = 0;
    m_meanBlockPropagationTime = 0;
    m_meanBlockSize = 0;
    m_numberOfPeers = m_peersAddresses.size();
}

CasperNode::~CasperNode(void)
{
    NS_LOG_FUNCTION (this);
}

} // namespace ns3