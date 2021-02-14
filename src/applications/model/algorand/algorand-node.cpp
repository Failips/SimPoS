/**
 * AlgorandNode class implementation. Extends BitcoinNode.
 */

#include "ns3/algorand-node.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AlgorandNode");

NS_OBJECT_ENSURE_REGISTERED (AlgorandNode);

TypeId
AlgorandNode::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::AlgorandNode")
            .SetParent<Application> ()
            .SetGroupName("Applications")
            .AddConstructor<AlgorandNode> ()
            .AddAttribute ("Local",
                           "The Address on which to Bind the rx socket.",
                           AddressValue (),
                           MakeAddressAccessor (&AlgorandNode::m_local),
                           MakeAddressChecker ())
            .AddAttribute ("Protocol",
                           "The type id of the protocol to use for the rx socket.",
                           TypeIdValue (UdpSocketFactory::GetTypeId ()),
                           MakeTypeIdAccessor (&AlgorandNode::m_tid),
                           MakeTypeIdChecker ())
            .AddAttribute ("BlockTorrent",
                           "Enable the BlockTorrent protocol",
                           BooleanValue (false),
                           MakeBooleanAccessor (&AlgorandNode::m_blockTorrent),
                           MakeBooleanChecker ())
            .AddAttribute ("InvTimeoutMinutes",
                           "The timeout of inv messages in minutes",
                           TimeValue (Minutes (20)),
                           MakeTimeAccessor (&AlgorandNode::m_invTimeoutMinutes),
                           MakeTimeChecker())
            .AddTraceSource ("Rx",
                             "A packet has been received",
                             MakeTraceSourceAccessor (&AlgorandNode::m_rxTrace),
                             "ns3::Packet::AddressTracedCallback")
    ;
    return tid;
}

AlgorandNode::AlgorandNode (void) : BitcoinNode()
{
    NS_LOG_FUNCTION (this);
    m_socket = 0;
    m_meanBlockReceiveTime = 0;
    m_previousBlockReceiveTime = 0;
    m_meanBlockPropagationTime = 0;
    m_meanBlockSize = 0;
    m_numberOfPeers = m_peersAddresses.size();
}

AlgorandNode::~AlgorandNode(void)
{
    NS_LOG_FUNCTION (this);
}

void AlgorandNode::StartApplication(){
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": Before btc start");
    BitcoinNode::StartApplication ();
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << ": After btc start");
}

} // namespace ns3