/**
* Implementation of AlgorandParticipant class
* @author Filip Borcik (xborci01@stud.fit.vutbr.cz / mak100@azet.sk)
*/

#include "algorand-participant.h"



namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("AlgorandParticipant");

    NS_OBJECT_ENSURE_REGISTERED (AlgorandParticipant);

    TypeId
    AlgorandParticipant::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::AlgorandParticipant")
                .SetParent<Application>()
                .SetGroupName("Applications")
                .AddConstructor<AlgorandParticipant>()
                .AddAttribute("Local",
                              "The Address on which to Bind the rx socket.",
                              AddressValue(),
                              MakeAddressAccessor(&AlgorandParticipant::m_local),
                              MakeAddressChecker())
                .AddAttribute("Protocol",
                              "The type id of the protocol to use for the rx socket.",
                              TypeIdValue(UdpSocketFactory::GetTypeId()),
                              MakeTypeIdAccessor(&AlgorandParticipant::m_tid),
                              MakeTypeIdChecker())
                .AddAttribute("BlockTorrent",
                              "Enable the BlockTorrent protocol",
                              BooleanValue(false),
                              MakeBooleanAccessor(&AlgorandParticipant::m_blockTorrent),
                              MakeBooleanChecker())
                .AddTraceSource("Rx",
                                "A packet has been received",
                                MakeTraceSourceAccessor(&AlgorandParticipant::m_rxTrace),
                                "ns3::Packet::AddressTracedCallback");
        return tid;
    }

    AlgorandParticipant::AlgorandParticipant() : AlgorandNode(),
                                   m_timeStart(0), m_timeFinish(0) {
        NS_LOG_FUNCTION(this);
        // TODO implement
    }


    BitcoinMiner::~BitcoinMiner(void) {
        NS_LOG_FUNCTION(this);
    }

} //  ns3 namespace