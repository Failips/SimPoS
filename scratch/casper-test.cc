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
#include "ns3/string.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/mpi-interface.h"
#include <vector>
#include <algorithm>
#include <cmath>

#define MPI_TEST

#ifdef NS3_MPI
#include <mpi.h>
#endif

using namespace ns3;

void collectAndPrintStats(nodeStatistics *stats, int totalNoNodes, int noMiners,
                          uint32_t systemId, uint32_t systemCount, int nodesInSystemId0,
                          double tStart, double tStartSimulation, double tFinish, double stop,
                          int minConnectionsPerNode, int maxConnectionsPerNode, int secsPerMin,
                          double averageBlockGenIntervalMinutes, bool relayNetwork,
                          BitcoinTopologyHelper *bitcoinTopologyHelper);
std::string pretty_bytes(long bytes);
double get_wall_time();
std::vector<int> generateFailedNodes(std::vector<uint32_t> nodes, int failed);
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
  bool sendheaders = false;
  bool blockTorrent = false;
  bool spv = false;
  long blockSize = -1;
  int invTimeoutMins = -1;
  int chunkSize = -1;
  enum Cryptocurrency  cryptocurrency = CASPER;
  double tStart = get_wall_time(), tStartSimulation, tFinish;
  const int secsPerMin = 60;
  const uint16_t bitcoinPort = 8333;
  const double realAverageBlockGenIntervalMinutes = 10; //minutes
  int targetNumberOfBlocks = 100;
  double averageBlockGenIntervalSeconds = 10 * secsPerMin; //seconds
  double fixedHashRate = 0.5;
  int start = 0;

  int totalNoNodes = 16;
  int failedVoters = 0;
  int failedMiners = 0;
  int minConnectionsPerNode = -1;
  int maxConnectionsPerNode = -1;
  double *minersHash;
  enum BitcoinRegion *minersRegions;
  int noMiners = 16;
  int noVoters = 100;
  int epochSize = 50;

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

  double averageBlockGenIntervalMinutes = averageBlockGenIntervalSeconds/secsPerMin;
  double stop;

  Ipv4InterfaceContainer                               ipv4InterfaceContainer;
  std::map<uint32_t, std::vector<Ipv4Address>>         nodesConnections;
  std::map<uint32_t, std::map<Ipv4Address, double>>    peersDownloadSpeeds;
  std::map<uint32_t, std::map<Ipv4Address, double>>    peersUploadSpeeds;
  std::map<uint32_t, nodeInternetSpeeds>               nodesInternetSpeeds;
  std::vector<uint32_t>                                miners;
  std::vector<uint32_t>                                voters;
  int                                                  nodesInSystemId0 = 0;

  Time::SetResolution (Time::NS);

  CommandLine cmd;
  cmd.AddValue ("nullmsg", "Enable the use of null-message synchronization", nullmsg);
  cmd.AddValue ("blockSize", "The the fixed block size (Bytes)", blockSize);
  cmd.AddValue ("noBlocks", "The number of generated blocks", targetNumberOfBlocks);
//  cmd.AddValue ("nodes", "The total number of nodes in the network", totalNoNodes);
  cmd.AddValue ("miners", "The total number of miners in the network", noMiners);
  cmd.AddValue ("voters", "The total number of Casper voters in the network", noVoters);
  cmd.AddValue ("failedNodes", "The total number of failed nodes (voters) in the network, does not participate", failedVoters);
  cmd.AddValue ("failedMiners", "The total number of failed miners in the network, does not mine", failedMiners);
  cmd.AddValue ("minConnections", "The minConnectionsPerNode of the grid", minConnectionsPerNode);
  cmd.AddValue ("maxConnections", "The maxConnectionsPerNode of the grid", maxConnectionsPerNode);
  cmd.AddValue ("blockIntervalMinutes", "The average block generation interval in minutes", averageBlockGenIntervalMinutes);
  cmd.AddValue ("invTimeoutMins", "The inv block timeout", invTimeoutMins);
  cmd.AddValue ("chunkSize", "The chunksize of the blockTorrent in Bytes", chunkSize);
  cmd.AddValue ("test", "Test the scalability of the simulation", testScalability);
  cmd.AddValue ("unsolicited", "Change the miners block broadcast type to UNSOLICITED", unsolicited);
  cmd.AddValue ("relayNetwork", "Change the miners block broadcast type to RELAY_NETWORK", relayNetwork);
  cmd.AddValue ("unsolicitedRelayNetwork", "Change the miners block broadcast type to UNSOLICITED_RELAY_NETWORK", unsolicitedRelayNetwork);
  cmd.AddValue ("sendheaders", "Change the protocol to sendheaders", sendheaders);
  cmd.AddValue ("blockTorrent", "Enable the BlockTorrent protocol", blockTorrent);
  cmd.AddValue ("spv", "Enable the spv mechanism", spv);
  cmd.AddValue ("stop", "Stop simulation after X simulation minutes", stop);
  cmd.AddValue ("epochSize", "Number of blocks in one Gasper epoch", epochSize);

  cmd.Parse(argc, argv);

  // total number of nodes is Miners + Voters (standard nodes are out of game for our simulation purposes)
  totalNoNodes = noMiners + noVoters;

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


  averageBlockGenIntervalSeconds = averageBlockGenIntervalMinutes * secsPerMin;
  nodeStatistics *stats = new nodeStatistics[totalNoNodes];
  averageBlockGenIntervalMinutes = averageBlockGenIntervalSeconds/secsPerMin;

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

  // this is used for showing blockchain stats at finish
//  LogComponentEnable("BitcoinNode", LOG_LEVEL_WARN);


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
  bitcoinTopologyHelper.AssignIpv4Addresses (Ipv4AddressHelperCustom ("1.0.0.0", "255.255.0.0", false));
  ipv4InterfaceContainer = bitcoinTopologyHelper.GetIpv4InterfaceContainer();
  nodesConnections = bitcoinTopologyHelper.GetNodesConnectionsIps();
  miners = bitcoinTopologyHelper.GetMiners();
  peersDownloadSpeeds = bitcoinTopologyHelper.GetPeersDownloadSpeeds();
  peersUploadSpeeds = bitcoinTopologyHelper.GetPeersUploadSpeeds();
  nodesInternetSpeeds = bitcoinTopologyHelper.GetNodesInternetSpeeds();
  if (systemId == 0)
    PrintBitcoinRegionStats(bitcoinTopologyHelper.GetBitcoinNodesRegions(), totalNoNodes);


  for(auto &node : nodesConnections)
  {
    Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (node.first);
    if ( std::find(miners.begin(), miners.end(), node.first) == miners.end() )
        voters.push_back(node.first);
  }

  // generate IDs of failed nodes
  failedMiners = failedMiners > noMiners ? noMiners : failedMiners;
  std::vector<int> failedMinerIDs = generateFailedNodes(miners, failedMiners);
  failedVoters = failedVoters > noVoters ? noVoters : failedVoters;
  std::vector<int> failedVoterIDs = generateFailedNodes(voters, failedVoters);

  //Install miners
  BitcoinMinerHelper bitcoinMinerHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                          nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0],
                                          nodesInternetSpeeds[0], stats, minersHash[0], averageBlockGenIntervalSeconds);
  ApplicationContainer bitcoinMiners;
  int count = 0;

  for(auto &miner : miners)
  {
	Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (miner);

	if (systemId == targetNode->GetSystemId())
	{
      if (blockSize != -1)
        bitcoinMinerHelper.SetAttribute("FixedBlockSize", UintegerValue(blockSize));

      bitcoinMinerHelper.SetMinerType (CASPER_MINER);

      bitcoinMinerHelper.SetPeersAddresses (nodesConnections[miner]);
	  bitcoinMinerHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
	  bitcoinMinerHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);
	  bitcoinMinerHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[miner]);
	  bitcoinMinerHelper.SetNodeStats (&stats[miner]);

	  bitcoinMinerHelper.SetEpochSize(epochSize);

	  if(unsolicited)
	    bitcoinMinerHelper.SetBlockBroadcastType (UNSOLICITED);
	  if(relayNetwork)
	    bitcoinMinerHelper.SetBlockBroadcastType (RELAY_NETWORK);
	  if(unsolicitedRelayNetwork)
	    bitcoinMinerHelper.SetBlockBroadcastType (UNSOLICITED_RELAY_NETWORK);

        if(std::find(failedMinerIDs.begin(), failedMinerIDs.end(), miner) != failedMinerIDs.end()){
            bitcoinMinerHelper.SetAttribute("IsFailed", BooleanValue(true));
        }else{
            bitcoinMinerHelper.SetAttribute("IsFailed", BooleanValue(false));
        }

	  bitcoinMiners.Add(bitcoinMinerHelper.Install (targetNode));

	  if (systemId == 0)
        nodesInSystemId0++;
	}
	count++;
  }
  bitcoinMiners.Start (Seconds (start));
  bitcoinMiners.Stop (Minutes (stop));


  //Install participants
  CasperParticipantHelper casperHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                          nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0],
                                          nodesInternetSpeeds[0], stats);
  ApplicationContainer casperVoters;

  for(auto &node : nodesConnections)
  {
    Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (node.first);

	if (systemId == targetNode->GetSystemId())
	{

      if ( std::find(miners.begin(), miners.end(), node.first) == miners.end() )
	  {
	    if(std::find(failedVoterIDs.begin(), failedVoterIDs.end(), node.first) != failedVoterIDs.end()){
            casperHelper.SetAttribute("IsFailed", BooleanValue(true));
	        std::cout << "F:"<<node.first<<std::endl;
        }else{
            casperHelper.SetAttribute("IsFailed", BooleanValue(false));
	        std::cout << "N:"<<node.first<<std::endl;
        }

	    casperHelper.SetPeersAddresses (node.second);
	    casperHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[node.first]);
	    casperHelper.SetPeersUploadSpeeds (peersUploadSpeeds[node.first]);
	    casperHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[node.first]);
		casperHelper.SetNodeStats (&stats[node.first]);

	    casperHelper.SetEpochSize(epochSize);

	    casperVoters.Add(casperHelper.Install (targetNode));

	    if (systemId == 0)
          nodesInSystemId0++;
	  }
	}
  }
  casperVoters.Start (Seconds (start));
  casperVoters.Stop (Minutes (stop));

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


  collectAndPrintStats(stats, totalNoNodes, noMiners,
                          systemId, systemCount, nodesInSystemId0,
                          tStart, tStartSimulation, tFinish, stop,
                          minConnectionsPerNode, maxConnectionsPerNode, secsPerMin,
                          averageBlockGenIntervalMinutes, relayNetwork,
                          &bitcoinTopologyHelper);

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

std::vector<int> generateFailedNodes(std::vector<uint32_t> nodes, int failed){
    std::vector<int> failedPositions;
    std::vector<int> failedNodes;
    int total = nodes.size();
    int cnt = 0;

    // generate failed positions
    for(int i = 0; i < total ; i++){
        if(cnt < failed && i%((int) ceil(total/failed)) == 0){
            failedPositions.push_back(i);
            cnt++;
        }
    }

    // transform positions to node indexes
    for(auto i : failedPositions){
        failedNodes.push_back(nodes.at(i));
    }

    return failedNodes;
}


void collectAndPrintStats(nodeStatistics *stats, int totalNoNodes, int noMiners,
                          uint32_t systemId, uint32_t systemCount, int nodesInSystemId0,
                          double tStart, double tStartSimulation, double tFinish, double stop,
                          int minConnectionsPerNode, int maxConnectionsPerNode, int secsPerMin,
                          double averageBlockGenIntervalMinutes, bool relayNetwork,
                          BitcoinTopologyHelper *bitcoinTopologyHelper){

#ifdef MPI_TEST

    int            blocklen[46] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Aint       disp[46];
    MPI_Datatype   dtypes[46] = {MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT,
                                 MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG,
                                 MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_INT, MPI_INT, MPI_INT, MPI_LONG, MPI_LONG,
                                 MPI_INT, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_DOUBLE, MPI_INT};
    MPI_Datatype   mpi_nodeStatisticsType;

    disp[0] = offsetof(nodeStatistics, nodeId);
    disp[1] = offsetof(nodeStatistics, meanBlockReceiveTime);
    disp[2] = offsetof(nodeStatistics, meanBlockPropagationTime);
    disp[3] = offsetof(nodeStatistics, meanBlockSize);
    disp[4] = offsetof(nodeStatistics, totalBlocks);
    disp[5] = offsetof(nodeStatistics, staleBlocks);
    disp[6] = offsetof(nodeStatistics, miner);
    disp[7] = offsetof(nodeStatistics, minerGeneratedBlocks);
    disp[8] = offsetof(nodeStatistics, minerAverageBlockGenInterval);
    disp[9] = offsetof(nodeStatistics, minerAverageBlockSize);
    disp[10] = offsetof(nodeStatistics, hashRate);
    disp[11] = offsetof(nodeStatistics, attackSuccess);
    disp[12] = offsetof(nodeStatistics, invReceivedBytes);
    disp[13] = offsetof(nodeStatistics, invSentBytes);
    disp[14] = offsetof(nodeStatistics, getHeadersReceivedBytes);
    disp[15] = offsetof(nodeStatistics, getHeadersSentBytes);
    disp[16] = offsetof(nodeStatistics, headersReceivedBytes);
    disp[17] = offsetof(nodeStatistics, headersSentBytes);
    disp[18] = offsetof(nodeStatistics, getDataReceivedBytes);
    disp[19] = offsetof(nodeStatistics, getDataSentBytes);
    disp[20] = offsetof(nodeStatistics, blockReceivedBytes);
    disp[21] = offsetof(nodeStatistics, blockSentBytes);
    disp[22] = offsetof(nodeStatistics, extInvReceivedBytes);
    disp[23] = offsetof(nodeStatistics, extInvSentBytes);
    disp[24] = offsetof(nodeStatistics, extGetHeadersReceivedBytes);
    disp[25] = offsetof(nodeStatistics, extGetHeadersSentBytes);
    disp[26] = offsetof(nodeStatistics, extHeadersReceivedBytes);
    disp[27] = offsetof(nodeStatistics, extHeadersSentBytes);
    disp[28] = offsetof(nodeStatistics, extGetDataReceivedBytes);
    disp[29] = offsetof(nodeStatistics, extGetDataSentBytes);
    disp[30] = offsetof(nodeStatistics, chunkReceivedBytes);
    disp[31] = offsetof(nodeStatistics, chunkSentBytes);
    disp[32] = offsetof(nodeStatistics, longestFork);
    disp[33] = offsetof(nodeStatistics, blocksInForks);
    disp[34] = offsetof(nodeStatistics, connections);
    disp[35] = offsetof(nodeStatistics, blockTimeouts);
    disp[36] = offsetof(nodeStatistics, chunkTimeouts);
    disp[37] = offsetof(nodeStatistics, minedBlocksInMainChain);
    disp[38] = offsetof(nodeStatistics, voteReceivedBytes);
    disp[39] = offsetof(nodeStatistics, voteSentBytes);
    disp[38] = offsetof(nodeStatistics, voteReceivedBytes);
    disp[39] = offsetof(nodeStatistics, voteSentBytes);
    disp[40] = offsetof(nodeStatistics, totalFinalizedBlocks);
    disp[41] = offsetof(nodeStatistics, totalCheckpoints);
    disp[42] = offsetof(nodeStatistics, totalFinalizedCheckpoints);
    disp[43] = offsetof(nodeStatistics, totalJustifiedCheckpoints);
    disp[44] = offsetof(nodeStatistics, maxBlockPropagationTime);
    disp[45] = offsetof(nodeStatistics, isFailed);

    MPI_Type_create_struct (46, blocklen, disp, dtypes, &mpi_nodeStatisticsType);
    MPI_Type_commit (&mpi_nodeStatisticsType);

    if (systemId != 0 && systemCount > 1)
    {
        /**
         * Sent all the systemId stats to systemId == 0
         */
        /* std::cout << "SystemId = " << systemId << "\n"; */

        for(int i = 0; i < totalNoNodes; i++)
        {
            Ptr<Node> targetNode = bitcoinTopologyHelper->GetNode (i);

            if (systemId == targetNode->GetSystemId())
            {
                MPI_Send(&stats[i], 1, mpi_nodeStatisticsType, 0, 8888, MPI_COMM_WORLD);
            }
        }
    }
    else if (systemId == 0 && systemCount > 1)
    {
        int count = nodesInSystemId0;

        while (count < totalNoNodes)
        {
            MPI_Status status;
            nodeStatistics recv;

            /* std::cout << "SystemId = " << systemId << "\n"; */
            MPI_Recv(&recv, 1, mpi_nodeStatisticsType, MPI_ANY_SOURCE, 8888, MPI_COMM_WORLD, &status);

/* 	  std::cout << "SystemId 0 received: statistics for node " << recv.nodeId
                <<  " from systemId = " << status.MPI_SOURCE << "\n"; */
            stats[recv.nodeId].nodeId = recv.nodeId;
            stats[recv.nodeId].meanBlockReceiveTime = recv.meanBlockReceiveTime;
            stats[recv.nodeId].meanBlockPropagationTime = recv.meanBlockPropagationTime;
            stats[recv.nodeId].maxBlockPropagationTime = recv.maxBlockPropagationTime;
            stats[recv.nodeId].meanBlockSize = recv.meanBlockSize;
            stats[recv.nodeId].totalBlocks = recv.totalBlocks;
            stats[recv.nodeId].staleBlocks = recv.staleBlocks;
            stats[recv.nodeId].totalFinalizedBlocks = recv.totalFinalizedBlocks;
            stats[recv.nodeId].totalCheckpoints = recv.totalCheckpoints;
            stats[recv.nodeId].totalFinalizedCheckpoints = recv.totalFinalizedCheckpoints;
            stats[recv.nodeId].totalJustifiedCheckpoints = recv.totalJustifiedCheckpoints;
            stats[recv.nodeId].miner = recv.miner;
            stats[recv.nodeId].minerGeneratedBlocks = recv.minerGeneratedBlocks;
            stats[recv.nodeId].minerAverageBlockGenInterval = recv.minerAverageBlockGenInterval;
            stats[recv.nodeId].minerAverageBlockSize = recv.minerAverageBlockSize;
            stats[recv.nodeId].hashRate = recv.hashRate;
            stats[recv.nodeId].invReceivedBytes = recv.invReceivedBytes;
            stats[recv.nodeId].invSentBytes = recv.invSentBytes;
            stats[recv.nodeId].getHeadersReceivedBytes = recv.getHeadersReceivedBytes;
            stats[recv.nodeId].getHeadersSentBytes = recv.getHeadersSentBytes;
            stats[recv.nodeId].headersReceivedBytes = recv.headersReceivedBytes;
            stats[recv.nodeId].headersSentBytes = recv.headersSentBytes;
            stats[recv.nodeId].getDataReceivedBytes = recv.getDataReceivedBytes;
            stats[recv.nodeId].getDataSentBytes = recv.getDataSentBytes;
            stats[recv.nodeId].blockReceivedBytes = recv.blockReceivedBytes;
            stats[recv.nodeId].blockSentBytes = recv.blockSentBytes;
            stats[recv.nodeId].extInvReceivedBytes = recv.extInvReceivedBytes;
            stats[recv.nodeId].extInvSentBytes = recv.extInvSentBytes;
            stats[recv.nodeId].extGetHeadersReceivedBytes = recv.extGetHeadersReceivedBytes;
            stats[recv.nodeId].extGetHeadersSentBytes = recv.extGetHeadersSentBytes;
            stats[recv.nodeId].extHeadersReceivedBytes = recv.extHeadersReceivedBytes;
            stats[recv.nodeId].extHeadersSentBytes = recv.extHeadersSentBytes;
            stats[recv.nodeId].extGetDataReceivedBytes = recv.extGetDataReceivedBytes;
            stats[recv.nodeId].extGetDataSentBytes = recv.extGetDataSentBytes;
            stats[recv.nodeId].chunkReceivedBytes = recv.chunkReceivedBytes;
            stats[recv.nodeId].chunkSentBytes = recv.chunkSentBytes;
            stats[recv.nodeId].longestFork = recv.longestFork;
            stats[recv.nodeId].blocksInForks = recv.blocksInForks;
            stats[recv.nodeId].connections = recv.connections;
            stats[recv.nodeId].blockTimeouts = recv.blockTimeouts;
            stats[recv.nodeId].chunkTimeouts = recv.chunkTimeouts;
            stats[recv.nodeId].minedBlocksInMainChain = recv.minedBlocksInMainChain;
            stats[recv.nodeId].voteReceivedBytes = recv.voteReceivedBytes;
            stats[recv.nodeId].voteSentBytes = recv.voteSentBytes;
            stats[recv.nodeId].isFailed = recv.isFailed;
            count++;
        }
    }
#endif

    if (systemId == 0)
    {
        tFinish=get_wall_time();

        PrintStatsForEachNode(stats, totalNoNodes);
        PrintTotalStats(stats, totalNoNodes, tStartSimulation, tFinish, averageBlockGenIntervalMinutes, relayNetwork);

        std::cout << "\nThe simulation ran for " << tFinish - tStart << "s simulating "
                  << stop << "mins. Performed " << stop * secsPerMin / (tFinish - tStart)
                  << " faster than realtime.\n" << "Setup time = " << tStartSimulation - tStart << "s\n"
                  <<"It consisted of " << totalNoNodes << " nodes (" << noMiners << " miners) with minConnectionsPerNode = "
                  << minConnectionsPerNode << " and maxConnectionsPerNode = " << maxConnectionsPerNode
                  << ".\nThe averageBlockGenIntervalMinutes was " << averageBlockGenIntervalMinutes << "min.\n";

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

std::string pretty_bytes(long bytes)
{
    const char* suffixes[7];
    suffixes[0] = "B";
    suffixes[1] = "KB";
    suffixes[2] = "MB";
    suffixes[3] = "GB";
    suffixes[4] = "TB";
    suffixes[5] = "PB";
    suffixes[6] = "EB";
    uint s = 0; // which suffix to use
    long double count = bytes;
    while (count >= 1024 && s < 7)
    {
        s++;
        count /= 1024;
    }
    static char output[200];
    if (count - floor(count) == 0.0)
        sprintf(output, "%Ld %s", (long)count, suffixes[s]);
    else
        sprintf(output, "%.1Lf %s", count, suffixes[s]);

    std::string str(output);
    return str ;
}

void PrintStatsForEachNode (nodeStatistics *stats, int totalNodes)
{
  int secPerMin = 60;

  for (int it = 0; it < totalNodes; it++ )
  {
    std::cout << "\nNode " << stats[it].nodeId << " statistics:\n";
    std::cout << "Connections = " << stats[it].connections << "\n";
      std::cout << "Is Failed = " << stats[it].isFailed << "\n";
    std::cout << "Mean Block Receive Time = " << stats[it].meanBlockReceiveTime << " or "
              << static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin << "min and "
              << stats[it].meanBlockReceiveTime - static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
    std::cout << "Mean Block Propagation Time = " << stats[it].meanBlockPropagationTime << "s\n";
    std::cout << "Max Block Propagation Time = " << stats[it].maxBlockPropagationTime << "s\n";
    std::cout << "Mean Block Size = " << stats[it].meanBlockSize << " Bytes\n";
    std::cout << "Total Blocks = " << stats[it].totalBlocks << "\n";
    std::cout << "Stale Blocks = " << stats[it].staleBlocks << " ("
              << 100. * stats[it].staleBlocks / stats[it].totalBlocks << "%)\n";
      std::cout << "Total Finalized Blocks = " << stats[it].totalFinalizedBlocks << "\n";
      std::cout << "Total Checkpoints = " << stats[it].totalCheckpoints << "\n";
      std::cout << "Total Finalized Checkpoints = " << stats[it].totalFinalizedCheckpoints << "\n";
      std::cout << "Total Justified Checkpoints = " << stats[it].totalJustifiedCheckpoints << "\n";
    std::cout << "The size of the longest fork was " << stats[it].longestFork << " blocks\n";
    std::cout << "There were in total " << stats[it].blocksInForks << " blocks in forks\n";
    std::cout << "The total received INV messages were " << stats[it].invReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_HEADERS messages were " << stats[it].getHeadersReceivedBytes << " Bytes\n";
    std::cout << "The total received HEADERS messages were " << stats[it].headersReceivedBytes << " Bytes\n";
    std::cout << "The total received GET_DATA messages were " << stats[it].getDataReceivedBytes << " Bytes\n";
    std::cout << "The total received BLOCK messages were " << stats[it].blockReceivedBytes << " Bytes\n";
      std::cout << "The total received VOTE messages were " << stats[it].voteReceivedBytes << " Bytes\n";
    std::cout << "The total sent INV messages were " << stats[it].invSentBytes << " Bytes\n";
    std::cout << "The total sent GET_HEADERS messages were " << stats[it].getHeadersSentBytes << " Bytes\n";
    std::cout << "The total sent HEADERS messages were " << stats[it].headersSentBytes << " Bytes\n";
    std::cout << "The total sent GET_DATA messages were " << stats[it].getDataSentBytes << " Bytes\n";
    std::cout << "The total sent BLOCK messages were " << stats[it].blockSentBytes << " Bytes\n";
      std::cout << "The total sent VOTE messages were " << stats[it].voteSentBytes << " Bytes\n";
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
  double     maxBlockPropagationTime = 0;
  double     meanMinersBlockPropagationTime = 0;
  double     maxMinersBlockPropagationTime = 0;
  double     meanBlockSize = 0;
  double     totalBlocks = 0;
  double     staleBlocks = 0;
    double     totalFinalizedBlocks = 0;
    double     totalCheckpoints = 0;
    double     totalFinalizedCheckpoints = 0;
    double     totalJustifiedCheckpoints = 0;
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
    double     voteReceivedBytes = 0;
    double     voteSentBytes = 0;
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

    int      failedNodes = 0;
    int      failedMiners = 0;
    int      nonFailed = 0;

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
      if(stats[it].isFailed && stats[it].miner == 0) {
          failedNodes++;
          continue;
      }else if(stats[it].isFailed && stats[it].miner == 1) {
          failedMiners++;
          continue;
      }

      meanBlockReceiveTime = meanBlockReceiveTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                             + stats[it].meanBlockReceiveTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
      meanBlockPropagationTime = meanBlockPropagationTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                                 + stats[it].meanBlockPropagationTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
      maxBlockPropagationTime = maxBlockPropagationTime > stats[it].maxBlockPropagationTime ? maxBlockPropagationTime : stats[it].maxBlockPropagationTime;
      meanBlockSize = meanBlockSize*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                      + stats[it].meanBlockSize*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
      totalBlocks += stats[it].totalBlocks;
      staleBlocks += stats[it].staleBlocks;
      totalFinalizedBlocks += stats[it].totalFinalizedBlocks;
      totalCheckpoints += stats[it].totalCheckpoints;
      totalFinalizedCheckpoints += stats[it].totalFinalizedCheckpoints;
      totalJustifiedCheckpoints += stats[it].totalJustifiedCheckpoints;
      invReceivedBytes = invReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].invReceivedBytes/static_cast<double>(nonFailed + 1);
      invSentBytes = invSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].invSentBytes/static_cast<double>(nonFailed + 1);
      getHeadersReceivedBytes = getHeadersReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].getHeadersReceivedBytes/static_cast<double>(nonFailed + 1);
      getHeadersSentBytes = getHeadersSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].getHeadersSentBytes/static_cast<double>(nonFailed + 1);
      headersReceivedBytes = headersReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].headersReceivedBytes/static_cast<double>(nonFailed + 1);
      headersSentBytes = headersSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].headersSentBytes/static_cast<double>(nonFailed + 1);
      getDataReceivedBytes = getDataReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].getDataReceivedBytes/static_cast<double>(nonFailed + 1);
      getDataSentBytes = getDataSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].getDataSentBytes/static_cast<double>(nonFailed + 1);
      blockReceivedBytes = blockReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].blockReceivedBytes/static_cast<double>(nonFailed + 1);
      blockSentBytes = blockSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].blockSentBytes/static_cast<double>(nonFailed + 1);
      voteReceivedBytes = voteReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].voteReceivedBytes/static_cast<double>(nonFailed + 1);
      voteSentBytes = voteSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].voteSentBytes/static_cast<double>(nonFailed + 1);
      extInvReceivedBytes = extInvReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extInvReceivedBytes/static_cast<double>(nonFailed + 1);
      extInvSentBytes = extInvSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extInvSentBytes/static_cast<double>(nonFailed + 1);
      extGetHeadersReceivedBytes = extGetHeadersReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extGetHeadersReceivedBytes/static_cast<double>(nonFailed + 1);
      extGetHeadersSentBytes = extGetHeadersSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extGetHeadersSentBytes/static_cast<double>(nonFailed + 1);
      extHeadersReceivedBytes = extHeadersReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extHeadersReceivedBytes/static_cast<double>(nonFailed + 1);
      extHeadersSentBytes = extHeadersSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extHeadersSentBytes/static_cast<double>(nonFailed + 1);
      extGetDataReceivedBytes = extGetDataReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extGetDataReceivedBytes/static_cast<double>(nonFailed + 1);
      extGetDataSentBytes = extGetDataSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].extGetDataSentBytes/static_cast<double>(nonFailed + 1);
      chunkReceivedBytes = chunkReceivedBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].chunkReceivedBytes/static_cast<double>(nonFailed + 1);
      chunkSentBytes = chunkSentBytes*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].chunkSentBytes/static_cast<double>(nonFailed + 1);
      longestFork = longestFork*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].longestFork/static_cast<double>(nonFailed + 1);
      blocksInForks = blocksInForks*nonFailed/static_cast<double>(nonFailed + 1) + stats[it].blocksInForks/static_cast<double>(nonFailed + 1);

      propagationTimes.push_back(stats[it].meanBlockPropagationTime);

      download = stats[it].invReceivedBytes + stats[it].getHeadersReceivedBytes + stats[it].headersReceivedBytes
                 + stats[it].getDataReceivedBytes + stats[it].blockReceivedBytes
                 + stats[it].extInvReceivedBytes + stats[it].extGetHeadersReceivedBytes + stats[it].extHeadersReceivedBytes
                 + stats[it].extGetDataReceivedBytes + stats[it].chunkReceivedBytes + stats[it].voteReceivedBytes;
      upload = stats[it].invSentBytes + stats[it].getHeadersSentBytes + stats[it].headersSentBytes
               + stats[it].getDataSentBytes + stats[it].blockSentBytes
               + stats[it].extInvSentBytes + stats[it].extGetHeadersSentBytes + stats[it].extHeadersSentBytes
               + stats[it].extGetDataSentBytes + stats[it].chunkSentBytes + stats[it].voteSentBytes;
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
          maxMinersBlockPropagationTime = maxMinersBlockPropagationTime > stats[it].maxBlockPropagationTime ? maxMinersBlockPropagationTime : stats[it].maxBlockPropagationTime;
          minersPropagationTimes.push_back(stats[it].meanBlockPropagationTime);
          miners++;
      }

      nonFailed++;
  }

  averageBandwidthPerNode = invReceivedBytes + invSentBytes + getHeadersReceivedBytes + getHeadersSentBytes + headersReceivedBytes
                            + headersSentBytes + getDataReceivedBytes + getDataSentBytes + blockReceivedBytes + blockSentBytes
                            + extInvReceivedBytes + extInvSentBytes + extGetHeadersReceivedBytes + extGetHeadersSentBytes + extHeadersReceivedBytes
                            + extHeadersSentBytes + extGetDataReceivedBytes + extGetDataSentBytes + chunkReceivedBytes + chunkSentBytes
                            + voteReceivedBytes + voteSentBytes;

  totalBlocks /= nonFailed;
  staleBlocks /= nonFailed;
    totalFinalizedBlocks /= nonFailed;
    totalCheckpoints /= nonFailed;
    totalFinalizedCheckpoints /= nonFailed;
    totalJustifiedCheckpoints /= nonFailed;
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
    std::cout << "Total Nodes = " << totalNodes << "\n";
    std::cout << "Miners = " << miners << "\n";
    std::cout << "Voters = " << (totalNodes-miners) << "\n";
    std::cout << "Failed Voters = " << failedNodes << "\n";
    std::cout << "Failed Miners = " << failedMiners << "\n";
  std::cout << "Average Connections/node = " << connectionsPerNode << "\n";
  std::cout << "Average Connections/miner = " << connectionsPerMiner << "\n";
  std::cout << "Mean Block Receive Time = " << meanBlockReceiveTime << " or "
            << static_cast<int>(meanBlockReceiveTime) / secPerMin << "min and "
            << meanBlockReceiveTime - static_cast<int>(meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
  std::cout << "Mean Block Propagation Time = " << meanBlockPropagationTime << "s\n";
  std::cout << "Max Block Propagation Time = " << maxBlockPropagationTime << "s\n";
  std::cout << "Median Block Propagation Time = " << median << "s\n";
  std::cout << "10% percentile of Block Propagation Time = " << p_10 << "s\n";
  std::cout << "25% percentile of Block Propagation Time = " << p_25 << "s\n";
  std::cout << "75% percentile of Block Propagation Time = " << p_75 << "s\n";
  std::cout << "90% percentile of Block Propagation Time = " << p_90 << "s\n";
  std::cout << "Miners Mean Block Propagation Time = " << meanMinersBlockPropagationTime << "s\n";
  std::cout << "Miners Max Block Propagation Time = " << maxMinersBlockPropagationTime << "s\n";
  std::cout << "Miners Median Block Propagation Time = " << minersMedian << "s\n";
  std::cout << "Mean Block Size = " << meanBlockSize << " Bytes\n";
  std::cout << "Total Blocks = " << totalBlocks << "\n";
  std::cout << "Stale Blocks = " << staleBlocks << " ("
            << 100. * staleBlocks / totalBlocks << "%)\n";
    std::cout << "Total Finalized Blocks = " << totalFinalizedBlocks << "\n";
    std::cout << "Total Checkpoints = " << totalCheckpoints << "\n";
    std::cout << "Total Finalized Checkpoints = " << totalFinalizedCheckpoints << "\n";
    std::cout << "Total Justified Checkpoints = " << totalJustifiedCheckpoints << "\n";
    std::cout << "The size of the longest fork was " << longestFork << " blocks\n";
  std::cout << "There were in total " << blocksInForks << " blocks in forks\n";
  std::cout << "The average received INV messages were " <<  pretty_bytes(invReceivedBytes) << " ("
            << 100. * invReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_HEADERS messages were " <<  pretty_bytes(getHeadersReceivedBytes) << " ("
            << 100. * getHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received HEADERS messages were " <<  pretty_bytes(headersReceivedBytes) << " ("
            << 100. * headersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received GET_DATA messages were " <<  pretty_bytes(getDataReceivedBytes) << " ("
            << 100. * getDataReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received BLOCK messages were " <<  pretty_bytes(blockReceivedBytes) << " ("
            << 100. * blockReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received VOTE messages were " << pretty_bytes(voteReceivedBytes) << " ("
              << 100. * voteReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent INV messages were " <<  pretty_bytes(invSentBytes) << " ("
            << 100. * invSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_HEADERS messages were " <<  pretty_bytes(getHeadersSentBytes) << " ("
            << 100. * getHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent HEADERS messages were " <<  pretty_bytes(headersSentBytes) << " ("
            << 100. * headersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent GET_DATA messages were " <<  pretty_bytes(getDataSentBytes) << " ("
            << 100. * getDataSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent BLOCK messages were " <<  pretty_bytes(blockSentBytes) << " ("
            << 100. * blockSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent VOTE messages were " << pretty_bytes(voteSentBytes) << " ("
              << 100. * voteSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_INV messages were " <<  pretty_bytes(extInvReceivedBytes) << " ("
            << 100. * extInvReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_GET_HEADERS messages were " <<  pretty_bytes(extGetHeadersReceivedBytes) << " ("
            << 100. * extGetHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_HEADERS messages were " <<  pretty_bytes(extHeadersReceivedBytes) << " ("
            << 100. * extHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received EXT_GET_DATA messages were " <<  pretty_bytes(extGetDataReceivedBytes) << " ("
            << 100. * extGetDataReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average received CHUNK messages were " <<  pretty_bytes(chunkReceivedBytes) << " ("
            << 100. * chunkReceivedBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_INV messages were " <<  pretty_bytes(extInvSentBytes) << " ("
            << 100. * extInvSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_GET_HEADERS messages were " <<  pretty_bytes(extGetHeadersSentBytes) << " ("
            << 100. * extGetHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_HEADERS messages were " <<  pretty_bytes(extHeadersSentBytes) << " ("
            << 100. * extHeadersSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent EXT_GET_DATA messages were " <<  pretty_bytes(extGetDataSentBytes) << " ("
            << 100. * extGetDataSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "The average sent CHUNK messages were " <<  pretty_bytes(chunkSentBytes) << " ("
            << 100. * chunkSentBytes / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to INV messages = " <<  pretty_bytes(invReceivedBytes +  invSentBytes) << " ("
            << 100. * (invReceivedBytes +  invSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to GET_HEADERS messages = " <<  pretty_bytes(getHeadersReceivedBytes +  getHeadersSentBytes) << " ("
            << 100. * (getHeadersReceivedBytes +  getHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to HEADERS messages = " <<  pretty_bytes(headersReceivedBytes +  headersSentBytes) << " ("
            << 100. * (headersReceivedBytes +  headersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to GET_DATA messages = " <<  pretty_bytes(getDataReceivedBytes +  getDataSentBytes) << " ("
            << 100. * (getDataReceivedBytes +  getDataSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to BLOCK messages = " <<  pretty_bytes(blockReceivedBytes +  blockSentBytes) << " ("
            << 100. * (blockReceivedBytes +  blockSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to VOTE messages = " <<  pretty_bytes(voteReceivedBytes +  voteSentBytes) << " ("
              << 100. * (voteReceivedBytes +  voteSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_INV messages = " <<  pretty_bytes(extInvReceivedBytes +  extInvSentBytes) << " ("
            << 100. * (extInvReceivedBytes +  extInvSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_GET_HEADERS messages = " <<  pretty_bytes(extGetHeadersReceivedBytes +  extGetHeadersSentBytes) << " ("
            << 100. * (extGetHeadersReceivedBytes +  extGetHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_HEADERS messages = " <<  pretty_bytes(extHeadersReceivedBytes +  extHeadersSentBytes) << " ("
            << 100. * (extHeadersReceivedBytes +  extHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to EXT_GET_DATA messages = " <<  pretty_bytes(extGetDataReceivedBytes +  extGetDataSentBytes) << " ("
            << 100. * (extGetDataReceivedBytes +  extGetDataSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic due to CHUNK messages = " <<  pretty_bytes(chunkReceivedBytes +  chunkSentBytes) << " ("
            << 100. * (chunkReceivedBytes +  chunkSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic/node = " <<  pretty_bytes(averageBandwidthPerNode) << " ("
            << averageBandwidthPerNode / (1000 *(totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8
            << " Kbps and " <<  pretty_bytes(averageBandwidthPerNode / (1000 * (totalBlocks - 1))) << "/block)\n";
    std::cout << "Total traffic due to BLOCK messages = " << pretty_bytes((blockReceivedBytes +  blockSentBytes)*(nonFailed)) << " ("
              << 100. * ((blockReceivedBytes +  blockSentBytes)*(nonFailed) )/ (averageBandwidthPerNode*(nonFailed)) << "%)\n";
    std::cout << "Total traffic due to VOTE messages = " << pretty_bytes((voteReceivedBytes +  voteSentBytes)*(nonFailed)) << " ("
              << 100. * ((voteReceivedBytes +  voteSentBytes)*(nonFailed)) / (averageBandwidthPerNode*(nonFailed)) << "%)\n";
    std::cout << "Total traffic = " << pretty_bytes(averageBandwidthPerNode*(nonFailed)) << "\n";
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

