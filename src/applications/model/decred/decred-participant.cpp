/**
* Implementation of DecredParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "ns3/decred-participant.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include "ns3/socket.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DecredParticipant");

NS_OBJECT_ENSURE_REGISTERED (DecredParticipant);

TypeId
DecredParticipant::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::DecredParticipant")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<DecredParticipant>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&DecredParticipant::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&DecredParticipant::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("BlockTorrent",
                          "Enable the BlockTorrent protocol",
                          BooleanValue(false),
                          MakeBooleanAccessor(&DecredParticipant::m_blockTorrent),
                          MakeBooleanChecker())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&DecredParticipant::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback");
    return tid;
}

DecredParticipant::DecredParticipant() : DecredNode(),
                               m_timeStart(0), m_timeFinish(0) {
    NS_LOG_FUNCTION(this);
    // TODO implement

}

DecredParticipant::~DecredParticipant(void) {
    NS_LOG_FUNCTION(this);
}

void
DecredParticipant::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    NS_LOG_FUNCTION (this);
    m_blockBroadcastType = blockBroadcastType;
}


void DecredParticipant::StartApplication() {
    DecredNode::StartApplication ();
}

void DecredParticipant::StopApplication() {
    DecredNode::StopApplication ();
}

void DecredParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    DecredNode::DoDispose ();
}

} //  ns3 namespace