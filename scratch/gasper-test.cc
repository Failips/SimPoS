/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

void createVRFThreshold(unsigned char*threshold, int leadingZerosCount);
double get_wall_time();
int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr);
void PrintStatsForEachNode (nodeStatistics *stats, int totalNodes);
void PrintTotalStats (nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalMinutes, bool relayNetwork);
void PrintBitcoinRegionStats (uint32_t *bitcoinNodesRegions, uint32_t totalNodes);

NS_LOG_COMPONENT_DEFINE ("MyMpiTest");

int
main (int argc, char *argv[])
{
#ifdef NS3_MPI
  bool nullmsg = false;
  bool testScalability = false;
  bool unsolicited = false;
  bool relayNetwork = false;
  bool unsolicitedRelayNetwork = true;
  enum Cryptocurrency  cryptocurrency = GASPER;
  double tStart = get_wall_time(), tStartSimulation, tFinish;
  const int secsPerMin = 60;
  const uint16_t bitcoinPort = 8333;
  const double realAverageBlockGenIntervalMinutes = 10; //minutes
  int start = 0;
  double stop = 0.30;

  int totalNoNodes = 16;
  int minConnectionsPerNode = -1;
  int maxConnectionsPerNode = -1;
  double *minersHash;
  enum BitcoinRegion *minersRegions;
  int noMiners = 16;
  bool allPrint = false;

  int epochSize = 64;

  // intervals between phases (in seconds)
  double intervalBP = 4;
  double intervalAttest = 4;

  // Thresholds for VRF output Y in each phase -> this values are just example and will be changed by leadingZeros parameter
  // Block proposal - in test with 100 nodes was 1-5 chosen
  unsigned char vrfThresholdBP[64] = {0x08,0xff,0xff,0x65,0xa9,0xac,0xb1,0x3d,0x05,0x15,0xa4,0x22,0xa9,0xfa,0x35,0x7d,0x1b,0x53,0x33,0x86,0xa5,0xe6,0x74,0x51,0x06,0x6a,0x8e,0xd3,0x11,0x38,0x10,0x18,0x65,0x25,0x76,0xe7,0x95,0xc1,0x32,0x85,0x28,0x0c,0x14,0xf7,0x0e,0x5c,0x4c,0x91,0x37,0xb5,0x68,0x47,0x85,0xef,0x7e,0x21,0x75,0x79,0x5e,0x83,0x3a,0x35,0xc6,0x44};
  // Attest voters
  unsigned char vrfThreshold[64] = {0x33,0xff,0xff,0x65,0xa9,0xac,0xb1,0x3d,0x05,0x15,0xa4,0x22,0xa9,0xfa,0x35,0x7d,0x1b,0x53,0x33,0x86,0xa5,0xe6,0x74,0x51,0x06,0x6a,0x8e,0xd3,0x11,0x38,0x10,0x18,0x65,0x25,0x76,0xe7,0x95,0xc1,0x32,0x85,0x28,0x0c,0x14,0xf7,0x0e,0x5c,0x4c,0x91,0x37,0xb5,0x68,0x47,0x85,0xef,0x7e,0x21,0x75,0x79,0x5e,0x83,0x3a,0x35,0xc6,0x44};

  int leadingZerosVrfBP = 5;
  int leadingZerosVrf   = 4;

#ifdef MPI_TEST

  double bitcoinMinersHash[] = {0.289, 0.196, 0.159, 0.133, 0.066, 0.054,
                                0.029, 0.016, 0.012, 0.012, 0.012, 0.009,
                                0.005, 0.005, 0.002, 0.002};
  enum BitcoinRegion bitcoinMinersRegions[] = {ASIA_PACIFIC, ASIA_PACIFIC, ASIA_PACIFIC, NORTH_AMERICA, ASIA_PACIFIC, NORTH_AMERICA,
                                               EUROPE, EUROPE, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, EUROPE,
                                               NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA};

#else

  double bitcoinMinersHash[] = {0.5, 0.5};
  enum BitcoinRegion bitcoinMinersRegions[] = {ASIA_PACIFIC, ASIA_PACIFIC};

#endif


  Ipv4InterfaceContainer                               ipv4InterfaceContainer;
  std::map<uint32_t, std::vector<Ipv4Address>>         nodesConnections;
  std::map<uint32_t, std::map<Ipv4Address, double>>    peersDownloadSpeeds;
  std::map<uint32_t, std::map<Ipv4Address, double>>    peersUploadSpeeds;
  std::map<uint32_t, nodeInternetSpeeds>               nodesInternetSpeeds;
  std::vector<uint32_t>                                miners;
  int                                                  nodesInSystemId0 = 0;

  Time::SetResolution (Time::NS);

  CommandLine cmd;
  cmd.AddValue ("nullmsg", "Enable the use of null-message synchronization", nullmsg);
  cmd.AddValue ("nodes", "The total number of nodes in the network", totalNoNodes);
//  cmd.AddValue ("miners", "The total number of miners in the network", noMiners);
  cmd.AddValue ("minConnections", "The minConnectionsPerNode of the grid", minConnectionsPerNode);
  cmd.AddValue ("maxConnections", "The maxConnectionsPerNode of the grid", maxConnectionsPerNode);
  cmd.AddValue ("test", "Test the scalability of the simulation", testScalability);
  cmd.AddValue ("unsolicited", "Change the miners block broadcast type to UNSOLICITED", unsolicited);
  cmd.AddValue ("relayNetwork", "Change the miners block broadcast type to RELAY_NETWORK", relayNetwork);
  cmd.AddValue ("unsolicitedRelayNetwork", "Change the miners block broadcast type to UNSOLICITED_RELAY_NETWORK", unsolicitedRelayNetwork);
  cmd.AddValue ("stop", "Stop simulation after X simulation minutes", stop);
  cmd.AddValue ("intervalBP", "Interval between block proposal phase and attest phase", intervalBP);
  cmd.AddValue ("intervalAtt", "Interval between attest phase and block proposal phase", intervalAttest);
  cmd.AddValue ("lzBP", "Leading zeros in VRF threshold used in choosing committee for choosing leader of committee (who is sending block proposal)", leadingZerosVrfBP);
  cmd.AddValue ("lzAtt", "Leading zeros in VRF threshold used in choosing committee members", leadingZerosVrf);
  cmd.AddValue ("allPrint", "On the end of simulation, each participant will print its blockchain stats", allPrint);
  cmd.AddValue ("epochSize", "Number of blocks in one Gasper epoch", epochSize);

  cmd.Parse(argc, argv);

  // all nodes are participants
  noMiners = totalNoNodes;

  // create VRF thresholds for each phase, based on choosed leading zeros by user
  createVRFThreshold(vrfThresholdBP, leadingZerosVrfBP);
  createVRFThreshold(vrfThreshold, leadingZerosVrf);


  // original bitcoin simulator had rule to have count of miners mutliple of 16, we fixed it with helpNum :)
  int helpNum = (noMiners + (16 - (noMiners % 16)));
  minersHash = new double[helpNum];
  minersRegions = new enum BitcoinRegion[helpNum];

  for(int i = 0; i < helpNum/16; i++)
  {
    for (int j = 0; j < 16 ; j++)
    {
      minersHash[i*16 + j] = bitcoinMinersHash[j]*16/helpNum;
      minersRegions[i*16 + j] = bitcoinMinersRegions[j];
    }
  }


  nodeStatistics *stats = new nodeStatistics[totalNoNodes];

  #ifdef MPI_TEST
  // Distributed simulation setup; by default use granted time window algorithm.
  if(nullmsg)
    {
      GlobalValue::Bind ("SimulatorImplementationType",
                         StringValue ("ns3::NullMessageSimulatorImpl"));
    }
  else
    {
      GlobalValue::Bind ("SimulatorImplementationType",
                         StringValue ("ns3::DistributedSimulatorImpl"));
    }

  // Enable parallel simulator with the command line arguments
  MpiInterface::Enable (&argc, &argv);
  uint32_t systemId = MpiInterface::GetSystemId ();
  uint32_t systemCount = MpiInterface::GetSize ();
#else
  uint32_t systemId = 0;
  uint32_t systemCount = 1;
#endif

  if (unsolicited && relayNetwork && unsolicitedRelayNetwork)
  {
    std::cout << "You have set both the unsolicited/relayNetwork/unsolicitedRelayNetwork flag\n";
    return 0;
  }

  BitcoinTopologyHelper bitcoinTopologyHelper (systemCount, totalNoNodes, noMiners, minersRegions,
                                               cryptocurrency, minConnectionsPerNode,
                                               maxConnectionsPerNode, 5, systemId);

  // Install stack on Grid
  InternetStackHelper stack;
  bitcoinTopologyHelper.InstallStack (stack);

  // Assign Addresses to Grid
  bitcoinTopologyHelper.AssignIpv4Addresses (Ipv4AddressHelperCustom ("1.0.0.0", "255.255.255.0", false));
  ipv4InterfaceContainer = bitcoinTopologyHelper.GetIpv4InterfaceContainer();
  nodesConnections = bitcoinTopologyHelper.GetNodesConnectionsIps();
  miners = bitcoinTopologyHelper.GetMiners();
  peersDownloadSpeeds = bitcoinTopologyHelper.GetPeersDownloadSpeeds();
  peersUploadSpeeds = bitcoinTopologyHelper.GetPeersUploadSpeeds();
  nodesInternetSpeeds = bitcoinTopologyHelper.GetNodesInternetSpeeds();
  if (systemId == 0)
    PrintBitcoinRegionStats(bitcoinTopologyHelper.GetBitcoinNodesRegions(), totalNoNodes);

  //Install miners
  GasperParticipantHelper gasperVoterHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                          nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0],
                                          nodesInternetSpeeds[0], stats);
  ApplicationContainer gasperVoters;
  int count = 0;

  for(auto &miner : miners)
  {
	Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (miner);

	if (systemId == targetNode->GetSystemId())
	{
      gasperVoterHelper.SetPeersAddresses (nodesConnections[miner]);
	  gasperVoterHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
	  gasperVoterHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);
	  gasperVoterHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[miner]);
	  gasperVoterHelper.SetNodeStats (&stats[miner]);

	  if(unsolicited)
	    gasperVoterHelper.SetBlockBroadcastType (UNSOLICITED);
	  if(relayNetwork)
	    gasperVoterHelper.SetBlockBroadcastType (RELAY_NETWORK);
	  if(unsolicitedRelayNetwork)
	    gasperVoterHelper.SetBlockBroadcastType (UNSOLICITED_RELAY_NETWORK);

	  gasperVoterHelper.SetVrfThresholdBP(vrfThresholdBP);
	  gasperVoterHelper.SetVrfThreshold(vrfThreshold);

	  gasperVoterHelper.SetIntervalBP(intervalBP);
	  gasperVoterHelper.SetIntervalAttest(intervalAttest);

	  gasperVoterHelper.SetAllPrint(allPrint);

	  gasperVoterHelper.SetEpochSize(epochSize);

	  gasperVoters.Add(gasperVoterHelper.Install (targetNode));

	  if (systemId == 0)
        nodesInSystemId0++;
	}
	count++;
  }
  gasperVoters.Start (Seconds (start));
  gasperVoters.Stop (Minutes (stop));


  //Install simple nodes
  BitcoinNodeHelper bitcoinNodeHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                        nodesConnections[0], peersDownloadSpeeds[0],  peersUploadSpeeds[0], nodesInternetSpeeds[0], stats);
  ApplicationContainer bitcoinNodes;

  for(auto &node : nodesConnections)
  {
    Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (node.first);

	if (systemId == targetNode->GetSystemId())
	{

      if ( std::find(miners.begin(), miners.end(), node.first) == miners.end() )
	  {
	    bitcoinNodeHelper.SetPeersAddresses (node.second);
	    bitcoinNodeHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[node.first]);
	    bitcoinNodeHelper.SetPeersUploadSpeeds (peersUploadSpeeds[node.first]);
	    bitcoinNodeHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[node.first]);
		bitcoinNodeHelper.SetNodeStats (&stats[node.first]);

	    bitcoinNodes.Add(bitcoinNodeHelper.Install (targetNode));

	    if (systemId == 0)
          nodesInSystemId0++;
	  }
	}
  }
  bitcoinNodes.Start (Seconds (start));
  bitcoinNodes.Stop (Minutes (stop));

  if (systemId == 0)
    std::cout << "The applications have been setup.\n";

  // Set up the actual simulation
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  tStartSimulation = get_wall_time();
  if (systemId == 0)
    std::cout << "Setup time = " << tStartSimulation - tStart << "s\n";
  Simulator::Stop (Minutes (stop + 0.1));
  Simulator::Run ();
  Simulator::Destroy ();

  tFinish = get_wall_time();
  if (systemId == 0)
    std::cout << "\nSimulation time = " << tFinish - tStartSimulation << "s\n";



#ifdef MPI_TEST

  // Exit the MPI execution environment
  MpiInterface::Disable ();
#endif

  delete[] stats;
  return 0;

#else
  NS_FATAL_ERROR ("Can't use distributed simulator without MPI compiled in");
#endif
}

void createVRFThreshold(unsigned char*threshold, int leadingZerosCount) {
    int fullZeroBytes = leadingZerosCount / 8;
    int mod = leadingZerosCount % 8;
    for (int i = 0; i < fullZeroBytes; ++i) {
        threshold[i] = 0x00;
    }

    unsigned char byte = 0xFF;
    byte = byte >> mod;
    threshold[fullZeroBytes] = byte;


    for (int i = fullZeroBytes+1; i < 64; ++i) {
        threshold[i] = 0xFF;
    }
}

double get_wall_time()
{
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    //  Handle error
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr)
{
  for (auto it = container.Begin(); it != container.End(); it++)
  {
    int32_t interface = it->first->GetInterfaceForAddress (addr);
    if ( interface != -1)
      return it->first->GetNetDevice (interface)-> GetNode()->GetId();
  }
  return -1; //if not found
}

void PrintStatsForEachNode (nodeStatistics *stats, int totalNodes)
{
  int secPerMin = 60;

  for (int it = 0; it < totalNodes; it++ )
  {
    std::cout << "\nNode " << stats[it].nodeId << " statistics:\n";
    std::cout << "Connections = " << stats[it].connections << "\n";
    std::cout << "Mean Block Receive Time = " << stats[it].meanBlockReceiveTime << " or "
              << static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin << "min and "
              << stats[it].meanBlockReceiveTime - static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
    std::cout << "Mean Block Propagation Time = " << stats[it].meanBlockPropagationTime << "s\n";
    std::cout << "Mean Block Size = " << stats[it].meanBlockSize << " Bytes\n";
    std::cout << "Total Blocks = " << stats[it].totalBlocks << "\n";
    std::cout << "Stale Blocks = " << stats[it].staleBlocks << " ("
              << 100. * stats[it].staleBlocks / stats[it].totalBlocks << "%)\n";
    std::cout << "The size of the longest fork was " << stats[it].longestFork << " blocks\n";
    std::cout << "There were in total " << stats[it].blocksInForks << " blocks in forks\n";
    std::cout << "The total received INV messages were " << stats[it].invReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_HEADERS messages were " << stats[it].getHeadersReceivedBytes << " Bytes\n";
    std::cout << "The total received HEADERS messages were " << stats[it].headersReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_DATA messages were " << stats[it].getDataReceivedBytes << " Bytes\n";
    std::cout << "The total received BLOCK messages were " << stats[it].blockReceivedBytes << " Bytes\n";
    std::cout << "The total sent INV messages were " << stats[it].invSentBytes << " Bytes\n";
    std::cout << "The total sent GET_HEADERS messages were " << stats[it].getHeadersSentBytes << " Bytes\n";
    std::cout << "The total sent HEADERS messages were " << stats[it].headersSentBytes << " Bytes\n";
    std::cout << "The total sent GET_DATA messages were " << stats[it].getDataSentBytes << " Bytes\n";
    std::cout << "The total sent BLOCK messages were " << stats[it].blockSentBytes << " Bytes\n";
    std::cout << "The total received EXT_INV messages were " << stats[it].extInvReceivedBytes << " Bytes\n";
    std::cout << "The total received EXT_GET_HEADERS messages were " << stats[it].extGetHeadersReceivedBytes << " Bytes\n";
    std::cout << "The total received EXT_HEADERS messages were " << stats[it].extHeadersReceivedBytes << " Bytes\n";
    std::cout << "The total received EXT_GET_DATA messages were " << stats[it].extGetDataReceivedBytes << " Bytes\n";
    std::cout << "The total received CHUNK messages were " << stats[it].chunkReceivedBytes << " Bytes\n";
    std::cout << "The total sent EXT_INV messages were " << stats[it].extInvSentBytes << " Bytes\n";
    std::cout << "The total sent EXT_GET_HEADERS messages were " << stats[it].extGetHeadersSentBytes << " Bytes\n";
    std::cout << "The total sent EXT_HEADERS messages were " << stats[it].extHeadersSentBytes << " Bytes\n";
    std::cout << "The total sent EXT_GET_DATA messages were " << stats[it].extGetDataSentBytes << " Bytes\n";
    std::cout << "The total sent CHUNK messages were " << stats[it].chunkSentBytes << " Bytes\n";

    if ( stats[it].miner == 1)
    {
      std::cout << "The miner " << stats[it].nodeId << " with hash rate = " << stats[it].hashRate*100 << "% generated " << stats[it].minerGeneratedBlocks
                << " blocks "<< "(" << 100. * stats[it].minerGeneratedBlocks / (stats[it].totalBlocks - 1)
                << "%) with average block generation time = " << stats[it].minerAverageBlockGenInterval
                << "s or " << static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin << "min and "
                << stats[it].minerAverageBlockGenInterval - static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin * secPerMin << "s"
                << " and average size " << stats[it].minerAverageBlockSize << " Bytes\n";
    }
  }
}


void PrintTotalStats (nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalMinutes, bool relayNetwork)
{
  const int  secPerMin = 60;
  double     meanBlockReceiveTime = 0;
  double     meanBlockPropagationTime = 0;
  double     meanMinersBlockPropagationTime = 0;
  double     meanBlockSize = 0;
  double     totalBlocks = 0;
  double     staleBlocks = 0;
  double     invReceivedBytes = 0;
  double     invSentBytes = 0;
  double     getHeadersReceivedBytes = 0;
  double     getHeadersSentBytes = 0;
  double     headersReceivedBytes = 0;
  double     headersSentBytes = 0;
  double     getDataReceivedBytes = 0;
  double     getDataSentBytes = 0;
  double     blockReceivedBytes = 0;
  double     blockSentBytes = 0;
  double     extInvReceivedBytes = 0;
  double     extInvSentBytes = 0;
  double     extGetHeadersReceivedBytes = 0;
  double     extGetHeadersSentBytes = 0;
  double     extHeadersReceivedBytes = 0;
  double     extHeadersSentBytes = 0;
  double     extGetDataReceivedBytes = 0;
  double     extGetDataSentBytes = 0;
  double     chunkReceivedBytes = 0;
  double     chunkSentBytes = 0;
  double     longestFork = 0;
  double     blocksInForks = 0;
  double     averageBandwidthPerNode = 0;
  double     connectionsPerNode = 0;
  double     connectionsPerMiner = 0;
  double     download = 0;
  double     upload = 0;

  uint32_t   nodes = 0;
  uint32_t   miners = 0;
  std::vector<double>    propagationTimes;
  std::vector<double>    minersPropagationTimes;
  std::vector<double>    downloadBandwidths;
  std::vector<double>    uploadBandwidths;
  std::vector<double>    totalBandwidths;
  std::vector<long>      blockTimeouts;
  std::vector<long>      chunkTimeouts;

  for (int it = 0; it < totalNodes; it++ )
  {
    meanBlockReceiveTime = meanBlockReceiveTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                           + stats[it].meanBlockReceiveTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    meanBlockPropagationTime = meanBlockPropagationTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                               + stats[it].meanBlockPropagationTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    meanBlockSize = meanBlockSize*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                    + stats[it].meanBlockSize*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
    totalBlocks += stats[it].totalBlocks;
    staleBlocks += stats[it].staleBlocks;
    invReceivedBytes = invReceivedBytes*it/static_cast<double>(it + 1) + stats[it].invReceivedBytes/static_cast<double>(it + 1);
    invSentBytes = invSentBytes*it/static_cast<double>(it + 1) + stats[it].invSentBytes/static_cast<double>(it + 1);
    getHeadersReceivedBytes = getHeadersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersReceivedBytes/static_cast<double>(it + 1);
    getHeadersSentBytes = getHeadersSentBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersSentBytes/static_cast<double>(it + 1);
    headersReceivedBytes = headersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].headersReceivedBytes/static_cast<double>(it + 1);
    headersSentBytes = headersSentBytes*it/static_cast<double>(it + 1) + stats[it].headersSentBytes/static_cast<double>(it + 1);
    getDataReceivedBytes = getDataReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getDataReceivedBytes/static_cast<double>(it + 1);
    getDataSentBytes = getDataSentBytes*it/static_cast<double>(it + 1) + stats[it].getDataSentBytes/static_cast<double>(it + 1);
    blockReceivedBytes = blockReceivedBytes*it/static_cast<double>(it + 1) + stats[it].blockReceivedBytes/static_cast<double>(it + 1);
    blockSentBytes = blockSentBytes*it/static_cast<double>(it + 1) + stats[it].blockSentBytes/static_cast<double>(it + 1);
    extInvReceivedBytes = extInvReceivedBytes*it/static_cast<double>(it + 1) + stats[it].extInvReceivedBytes/static_cast<double>(it + 1);
    extInvSentBytes = extInvSentBytes*it/static_cast<double>(it + 1) + stats[it].extInvSentBytes/static_cast<double>(it + 1);
    extGetHeadersReceivedBytes = extGetHeadersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].extGetHeadersReceivedBytes/static_cast<double>(it + 1);
    extGetHeadersSentBytes = extGetHeadersSentBytes*it/static_cast<double>(it + 1) + stats[it].extGetHeadersSentBytes/static_cast<double>(it + 1);
    extHeadersReceivedBytes = extHeadersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].extHeadersReceivedBytes/static_cast<double>(it + 1);
    extHeadersSentBytes = extHeadersSentBytes*it/static_cast<double>(it + 1) + stats[it].extHeadersSentBytes/static_cast<double>(it + 1);
    extGetDataReceivedBytes = extGetDataReceivedBytes*it/static_cast<double>(it + 1) + stats[it].extGetDataReceivedBytes/static_cast<double>(it + 1);
    extGetDataSentBytes = extGetDataSentBytes*it/static_cast<double>(it + 1) + stats[it].extGetDataSentBytes/static_cast<double>(it + 1);
    chunkReceivedBytes = chunkReceivedBytes*it/static_cast<double>(it + 1) + stats[it].chunkReceivedBytes/static_cast<double>(it + 1);
    chunkSentBytes = chunkSentBytes*it/static_cast<double>(it + 1) + stats[it].chunkSentBytes/static_cast<double>(it + 1);
    longestFork = longestFork*it/static_cast<double>(it + 1) + stats[it].longestFork/static_cast<double>(it + 1);
    blocksInForks = blocksInForks*it/static_cast<double>(it + 1) + stats[it].blocksInForks/static_cast<double>(it + 1);

    propagationTimes.push_back(stats[it].meanBlockPropagationTime);

    download = stats[it].invReceivedBytes + stats[it].getHeadersReceivedBytes + stats[it].headersReceivedBytes
               + stats[it].getDataReceivedBytes + stats[it].blockReceivedBytes
               + stats[it].extInvReceivedBytes + stats[it].extGetHeadersReceivedBytes + stats[it].extHeadersReceivedBytes
               + stats[it].extGetDataReceivedBytes + stats[it].chunkReceivedBytes;
    upload = stats[it].invSentBytes + stats[it].getHeadersSentBytes + stats[it].headersSentBytes
             + stats[it].getDataSentBytes + stats[it].blockSentBytes
             + stats[it].extInvSentBytes + stats[it].extGetHeadersSentBytes + stats[it].extHeadersSentBytes
             + stats[it].extGetDataSentBytes + stats[it].chunkSentBytes;;
    download = download / (1000 *(stats[it].totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8;
    upload = upload / (1000 *(stats[it].totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8;
    downloadBandwidths.push_back(download);
    uploadBandwidths.push_back(upload);
    totalBandwidths.push_back(download + upload);
    blockTimeouts.push_back(stats[it].blockTimeouts);
    chunkTimeouts.push_back(stats[it].chunkTimeouts);

    if(stats[it].miner == 0)
    {
      connectionsPerNode = connectionsPerNode*nodes/static_cast<double>(nodes + 1) + stats[it].connections/static_cast<double>(nodes + 1);
      nodes++;
    }
    else
    {
      connectionsPerMiner = connectionsPerMiner*miners/static_cast<double>(miners + 1) + stats[it].connections/static_cast<double>(miners + 1);
      meanMinersBlockPropagationTime = meanMinersBlockPropagationTime*miners/static_cast<double>(miners + 1) + stats[it].meanBlockPropagationTime/static_cast<double>(miners + 1);
      minersPropagationTimes.push_back(stats[it].meanBlockPropagationTime);
      miners++;
    }
  }

  averageBandwidthPerNode = invReceivedBytes + invSentBytes + getHeadersReceivedBytes + getHeadersSentBytes + headersReceivedBytes
                            + headersSentBytes + getDataReceivedBytes + getDataSentBytes + blockReceivedBytes + blockSentBytes
                            + extInvReceivedBytes + extInvSentBytes + extGetHeadersReceivedBytes + extGetHeadersSentBytes + extHeadersReceivedBytes
                            + extHeadersSentBytes + extGetDataReceivedBytes + extGetDataSentBytes + chunkReceivedBytes + chunkSentBytes ;

  totalBlocks /= totalNodes;
  staleBlocks /= totalNodes;
  sort(propagationTimes.begin(), propagationTimes.end());
  sort(minersPropagationTimes.begin(), minersPropagationTimes.end());
  sort(blockTimeouts.begin(), blockTimeouts.end());
  sort(chunkTimeouts.begin(), chunkTimeouts.end());

  double median = *(propagationTimes.begin()+propagationTimes.size()/2);
  double p_10 = *(propagationTimes.begin()+int(propagationTimes.size()*.1));
  double p_25 = *(propagationTimes.begin()+int(propagationTimes.size()*.25));
  double p_75 = *(propagationTimes.begin()+int(propagationTimes.size()*.75));
  double p_90 = *(propagationTimes.begin()+int(propagationTimes.size()*.90));
  double minersMedian = *(minersPropagationTimes.begin()+int(minersPropagationTimes.size()/2));

  std::cout << "\nTotal Stats:\n";
  std::cout << "Average Connections/node = " << connectionsPerNode << "\n";
  std::cout << "Average Connections/miner = " << connectionsPerMiner << "\n";
  std::cout << "Mean Block Receive Time = " << meanBlockReceiveTime << " or "
            << static_cast<int>(meanBlockReceiveTime) / secPerMin << "min and "
            << meanBlockReceiveTime - static_cast<int>(meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
  std::cout << "Mean Block Propagation Time = " << meanBlockPropagationTime << "s\n";
  std::cout << "Median Block Propagation Time = " << median << "s\n";
  std::cout << "10% percentile of Block Propagation Time = " << p_10 << "s\n";
  std::cout << "25% percentile of Block Propagation Time = " << p_25 << "s\n";
  std::cout << "75% percentile of Block Propagation Time = " << p_75 << "s\n";
  std::cout << "90% percentile of Block Propagation Time = " << p_90 << "s\n";
  std::cout << "Miners Mean Block Propagation Time = " << meanMinersBlockPropagationTime << "s\n";
  std::cout << "Miners Median Block Propagation Time = " << minersMedian << "s\n";
  std::cout << "Mean Block Size = " << meanBlockSize << " Bytes\n";
  std::cout << "Total Blocks = " << totalBlocks << "\n";
  std::cout << "Stale Blocks = " << staleBlocks << " ("
            << 100. * staleBlocks / totalBlocks << "%)\n";
  std::cout << "The size of the longest fork was " << longestFork << " blocks\n";
  std::cout << "There were in total " << blocksInForks << " blocks in forks\n";
  std::cout << "The average received INV messages were " << invReceivedBytes << " Bytes ("
            << 100. * invReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_HEADERS messages were " << getHeadersReceivedBytes << " Bytes ("
            << 100. * getHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received HEADERS messages were " << headersReceivedBytes << " Bytes ("
            << 100. * headersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_DATA messages were " << getDataReceivedBytes << " Bytes ("
            << 100. * getDataReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received BLOCK messages were " << blockReceivedBytes << " Bytes ("
            << 100. * blockReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent INV messages were " << invSentBytes << " Bytes ("
            << 100. * invSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_HEADERS messages were " << getHeadersSentBytes << " Bytes ("
            << 100. * getHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent HEADERS messages were " << headersSentBytes << " Bytes ("
            << 100. * headersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_DATA messages were " << getDataSentBytes << " Bytes ("
            << 100. * getDataSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent BLOCK messages were " << blockSentBytes << " Bytes ("
            << 100. * blockSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_INV messages were " << extInvReceivedBytes << " Bytes ("
            << 100. * extInvReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_GET_HEADERS messages were " << extGetHeadersReceivedBytes << " Bytes ("
            << 100. * extGetHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_HEADERS messages were " << extHeadersReceivedBytes << " Bytes ("
            << 100. * extHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_GET_DATA messages were " << extGetDataReceivedBytes << " Bytes ("
            << 100. * extGetDataReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received CHUNK messages were " << chunkReceivedBytes << " Bytes ("
            << 100. * chunkReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_INV messages were " << extInvSentBytes << " Bytes ("
            << 100. * extInvSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_GET_HEADERS messages were " << extGetHeadersSentBytes << " Bytes ("
            << 100. * extGetHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_HEADERS messages were " << extHeadersSentBytes << " Bytes ("
            << 100. * extHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_GET_DATA messages were " << extGetDataSentBytes << " Bytes ("
            << 100. * extGetDataSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent CHUNK messages were " << chunkSentBytes << " Bytes ("
            << 100. * chunkSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to INV messages = " << invReceivedBytes +  invSentBytes << " Bytes("
            << 100. * (invReceivedBytes +  invSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to GET_HEADERS messages = " << getHeadersReceivedBytes +  getHeadersSentBytes << " Bytes("
            << 100. * (getHeadersReceivedBytes +  getHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to HEADERS messages = " << headersReceivedBytes +  headersSentBytes << " Bytes("
            << 100. * (headersReceivedBytes +  headersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to GET_DATA messages = " << getDataReceivedBytes +  getDataSentBytes << " Bytes("
            << 100. * (getDataReceivedBytes +  getDataSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to BLOCK messages = " << blockReceivedBytes +  blockSentBytes << " Bytes("
            << 100. * (blockReceivedBytes +  blockSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_INV messages = " << extInvReceivedBytes +  extInvSentBytes << " Bytes("
            << 100. * (extInvReceivedBytes +  extInvSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_GET_HEADERS messages = " << extGetHeadersReceivedBytes +  extGetHeadersSentBytes << " Bytes("
            << 100. * (extGetHeadersReceivedBytes +  extGetHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_HEADERS messages = " << extHeadersReceivedBytes +  extHeadersSentBytes << " Bytes("
            << 100. * (extHeadersReceivedBytes +  extHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_GET_DATA messages = " << extGetDataReceivedBytes +  extGetDataSentBytes << " Bytes("
            << 100. * (extGetDataReceivedBytes +  extGetDataSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to CHUNK messages = " << chunkReceivedBytes +  chunkSentBytes << " Bytes("
            << 100. * (chunkReceivedBytes +  chunkSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic/node = " << averageBandwidthPerNode << " Bytes ("
            << averageBandwidthPerNode / (1000 *(totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8
            << " Kbps and " << averageBandwidthPerNode / (1000 * (totalBlocks - 1)) << " KB/block)\n";
  std::cout << (finish - start)/ (totalBlocks - 1)<< "s per generated block\n";


  std::cout << "\nBlock Propagation Times = [";
  for(auto it = propagationTimes.begin(); it != propagationTimes.end(); it++)
  {
    if (it == propagationTimes.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
  }
  std::cout << "]\n" ;

  std::cout << "\nMiners Block Propagation Times = [";
  for(auto it = minersPropagationTimes.begin(); it != minersPropagationTimes.end(); it++)
  {
    if (it == minersPropagationTimes.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
  }
  std::cout << "]\n" ;

  std::cout << "\nDownload Bandwidths = [";
  double average = 0;
  for(auto it = downloadBandwidths.begin(); it != downloadBandwidths.end(); it++)
  {
    if (it == downloadBandwidths.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
    average += *it;
  }
  std::cout << "] average = " << average/totalBandwidths.size() << "\n" ;

  std::cout << "\nUpload Bandwidths = [";
  average = 0;
  for(auto it = uploadBandwidths.begin(); it != uploadBandwidths.end(); it++)
  {
    if (it == uploadBandwidths.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
    average += *it;
  }
  std::cout << "] average = " << average/totalBandwidths.size() << "\n" ;

  std::cout << "\nTotal Bandwidths = [";
  average = 0;
  for(auto it = totalBandwidths.begin(); it != totalBandwidths.end(); it++)
  {
    if (it == totalBandwidths.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
    average += *it;
  }
  std::cout << "] average = " << average/totalBandwidths.size() << "\n" ;

  std::cout << "\nBlock Timeouts = [";
  average = 0;
  for(auto it = blockTimeouts.begin(); it != blockTimeouts.end(); it++)
  {
    if (it == blockTimeouts.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
    average += *it;
  }
  std::cout << "] average = " << average/blockTimeouts.size() << "\n" ;

  std::cout << "\nChunk Timeouts = [";
  average = 0;
  for(auto it = chunkTimeouts.begin(); it != chunkTimeouts.end(); it++)
  {
    if (it == chunkTimeouts.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
    average += *it;
  }
  std::cout << "] average = " << average/chunkTimeouts.size() << "\n" ;

  std::cout << "\n";
}

void PrintBitcoinRegionStats (uint32_t *bitcoinNodesRegions, uint32_t totalNodes)
{
  uint32_t regions[7] = {0, 0, 0, 0, 0, 0, 0};

  for (uint32_t i = 0; i < totalNodes; i++)
    regions[bitcoinNodesRegions[i]]++;

  std::cout << "Nodes distribution: \n";
  for (uint32_t i = 0; i < 7; i++)
  {
    std::cout << getBitcoinRegion(getBitcoinEnum(i)) << ": " << regions[i] * 100.0 / totalNodes << "%\n";
  }
}

