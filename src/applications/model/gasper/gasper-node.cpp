/**
 * GasperNode class implementation. Extends BitcoinNode.
 */

#include "ns3/gasper-node.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GasperNode");

NS_OBJECT_ENSURE_REGISTERED (GasperNode);

TypeId
GasperNode::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::GasperNode")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<GasperNode> ()
            .AddAttribute ("Local",
                           "The Address on which to Bind the rx socket.",
                           AddressValue (),
                           MakeAddressAccessor (&GasperNode::m_local),
                           MakeAddressChecker ())
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&GasperNode::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                           "Enable the BlockTorrent protocol",
                           BooleanValue (false),
                           MakeBooleanAccessor (&GasperNode::m_blockTorrent),
                           MakeBooleanChecker ())
            .AddAttribute ("InvTimeoutMinutes",
                           "The timeout of inv messages in minutes",
                           TimeValue (Minutes (20)),
                           MakeTimeAccessor (&GasperNode::m_invTimeoutMinutes),
                           MakeTimeChecker())
            .AddTraceSource ("Rx",
                             "A packet has been received",
                             MakeTraceSourceAccessor (&GasperNode::m_rxTrace),
                             "ns3::Packet::AddressTracedCallback")
    ;
    return tid;
}

GasperNode::GasperNode (void) : BitcoinNode()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_meanBlockReceiveTime = 0;
    m_previousBlockReceiveTime = 0;
    m_meanBlockPropagationTime = 0;
    m_meanBlockSize = 0;
    m_numberOfPeers = m_peersAddresses.size();
}

GasperNode::~GasperNode(void)
{
    NS_LOG_FUNCTION (this);
}

void GasperNode::StartApplication(){
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": Before btc start");
    BitcoinNode::StartApplication ();
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": After btc start");
}

} // namespace ns3