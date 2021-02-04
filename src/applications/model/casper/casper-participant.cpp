/**
* Implementation of CasperParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "ns3/casper-participant.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"
#include "ns3/socket.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CasperParticipant");

NS_OBJECT_ENSURE_REGISTERED (CasperParticipant);

TypeId
CasperParticipant::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::CasperParticipant")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<CasperParticipant>()
            .AddAttribute("Local",
                          "The Address on which to Bind the rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&CasperParticipant::m_local),
                          MakeAddressChecker())
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&CasperParticipant::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("BlockTorrent",
                          "Enable the BlockTorrent protocol",
                          BooleanValue(false),
                          MakeBooleanAccessor(&CasperParticipant::m_blockTorrent),
                          MakeBooleanChecker())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&CasperParticipant::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback");
    return tid;
}

CasperParticipant::CasperParticipant() : CasperNode(),
                               m_timeStart(0), m_timeFinish(0) {
    NS_LOG_FUNCTION(this);
    // TODO implement

}

CasperParticipant::~CasperParticipant(void) {
    NS_LOG_FUNCTION(this);
}

void
CasperParticipant::SetBlockBroadcastType (enum BlockBroadcastType blockBroadcastType)
{
    NS_LOG_FUNCTION (this);
    m_blockBroadcastType = blockBroadcastType;
}


void CasperParticipant::StartApplication() {
    CasperNode::StartApplication ();
}

void CasperParticipant::StopApplication() {
    CasperNode::StopApplication ();
}

void CasperParticipant::DoDispose(void) {
    NS_LOG_FUNCTION (this);
    CasperNode::DoDispose ();
}

} //  ns3 namespace