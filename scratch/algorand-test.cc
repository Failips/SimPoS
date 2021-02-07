/**
 * implementation for testing of Algorand protocol
 */

#include <fstream>
#include <time.h>
#include <sys/time.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/mpi-interface.h"
#define MPI_TEST

#ifdef NS3_MPI
#include <mpi.h>
#endif

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AlgorandBlockchainTest");

int main (int argc, char *argv[])
{
#ifdef NS3_MPI
    // simulation parameters
    const int secsPerMin = 60;
    const uint16_t bitcoinPort = 8333;
    int start = 0;

    enum BitcoinRegion regions[] = {ASIA_PACIFIC, ASIA_PACIFIC, NORTH_AMERICA, ASIA_PACIFIC, NORTH_AMERICA,
                                               EUROPE, EUROPE, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, EUROPE,
                                               NORTH_AMERICA, EUROPE, NORTH_AMERICA, NORTH_AMERICA, ASIA_PACIFIC};

    int totalNoNodes = 5;
    int noMiners = totalNoNodes;
    int attackerId = totalNoNodes - 1;
    int minConnectionsPerNode = 1;
    int maxConnectionsPerNode = 1;
    int  nodesInSystemId0 = 0;
    enum Cryptocurrency  cryptocurrency = BITCOIN; // only used for creating topology

    nodeStatistics *stats = new nodeStatistics[totalNoNodes];

    srand (1000);
    Time::SetResolution (Time::NS);

    // todo zvazit pridanie distribuovanych sprav namiesto rovnomerneho (viz. bitcoin-test)
    uint32_t systemId = 0;
    uint32_t systemCount = 1;

    // enabling logs on output
    LogComponentEnable("AlgorandBlockchainTest", LOG_LEVEL_INFO);
    LogComponentEnable("AlgorandParticipant", LOG_LEVEL_INFO);


    NS_LOG (LOG_ERROR, "Node ISISISISISISI----55959 B/s");
    // todo pridat podporu roznych parametrov spustenia
    CommandLine cmd;
    cmd.Parse(argc, argv);

    double stop = 0.5; // doba trvania v minutach

    // --- ITERATION START ---
    // declaring variables
    Ipv4InterfaceContainer                               ipv4InterfaceContainer;
    std::map<uint32_t, std::vector<Ipv4Address>>         nodesConnections;
    std::map<uint32_t, std::map<Ipv4Address, double>>    peersDownloadSpeeds;
    std::map<uint32_t, std::map<Ipv4Address, double>>    peersUploadSpeeds;
    std::map<uint32_t, nodeInternetSpeeds>               nodesInternetSpeeds;
    std::vector<uint32_t>                                participants;

	// creating topologyHelper
    BitcoinTopologyHelper topologyHelper (systemCount, totalNoNodes, noMiners, regions,
                                                 cryptocurrency, minConnectionsPerNode,
                                                 maxConnectionsPerNode, 2, systemId);

    // Install stack on Grid
    InternetStackHelper stack;
    topologyHelper.InstallStack (stack);

    // Assign Addresses to Grid
    topologyHelper.AssignIpv4Addresses (Ipv4AddressHelperCustom ("1.0.0.0", "255.255.255.0", false));
    ipv4InterfaceContainer = topologyHelper.GetIpv4InterfaceContainer();
    nodesConnections = topologyHelper.GetNodesConnectionsIps();
    participants = topologyHelper.GetMiners();
    peersDownloadSpeeds = topologyHelper.GetPeersDownloadSpeeds();
    peersUploadSpeeds = topologyHelper.GetPeersUploadSpeeds();
    nodesInternetSpeeds = topologyHelper.GetNodesInternetSpeeds();

    //Install miners
    AlgorandParticipantHelper participantHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                            nodesConnections[participants[0]], noMiners, peersDownloadSpeeds[0],
                                            peersUploadSpeeds[0], nodesInternetSpeeds[0], stats);

    ApplicationContainer algorandParticipants;
    int count = 0;

    for(auto &miner : participants)
    {
        Ptr<Node> targetNode = topologyHelper.GetNode (miner);

        if (systemId == targetNode->GetSystemId())
        {
            participantHelper.SetPeersAddresses (nodesConnections[miner]);
            participantHelper.SetNodeStats (&stats[miner]);
            participantHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
            participantHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);

            participantHelper.SetBlockBroadcastType (UNSOLICITED_RELAY_NETWORK);

            ApplicationContainer cont = participantHelper.Install (targetNode);
            algorandParticipants.Add(cont);
            if (systemId == 0)
                nodesInSystemId0++;
        }
        count++;
    }
    algorandParticipants.Start (Seconds (start));
    algorandParticipants.Stop (Minutes (stop));

    // Set up the actual simulation
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Minutes (stop + 0.1));
    Simulator::Run ();
    Simulator::Destroy ();



    // --- ITERATION END ---



//#ifdef MPI_TEST
//
//  // Exit the MPI execution environment
//  MpiInterface::Disable ();
//#endif

  delete[] stats;
  return 0;

#else
    NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
}
