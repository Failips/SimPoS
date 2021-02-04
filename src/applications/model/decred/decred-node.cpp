/**
 * DecredNode class implementation. Extends BitcoinNode.
 */

#include "ns3/decred-node.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DecredNode");

NS_OBJECT_ENSURE_REGISTERED (DecredNode);

TypeId
DecredNode::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::DecredNode")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<DecredNode> ()
            .AddAttribute ("Local",
                           "The Address on which to Bind the rx socket.",
                           AddressValue (),
                           MakeAddressAccessor (&DecredNode::m_local),
                           MakeAddressChecker ())
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&DecredNode::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                           "Enable the BlockTorrent protocol",
                           BooleanValue (false),
                           MakeBooleanAccessor (&DecredNode::m_blockTorrent),
                           MakeBooleanChecker ())
            .AddAttribute ("InvTimeoutMinutes",
                           "The timeout of inv messages in minutes",
                           TimeValue (Minutes (20)),
                           MakeTimeAccessor (&DecredNode::m_invTimeoutMinutes),
                           MakeTimeChecker())
            .AddTraceSource ("Rx",
                             "A packet has been received",
                             MakeTraceSourceAccessor (&DecredNode::m_rxTrace),
                             "ns3::Packet::AddressTracedCallback")
    ;
    return tid;
}

DecredNode::DecredNode (void)
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_meanBlockReceiveTime = 0;
    m_previousBlockReceiveTime = 0;
    m_meanBlockPropagationTime = 0;
    m_meanBlockSize = 0;
    m_numberOfPeers = m_peersAddresses.size();
}

DecredNode::~DecredNode(void)
{
    NS_LOG_FUNCTION (this);
}

} // namespace ns3