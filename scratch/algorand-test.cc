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
  enum Cryptocurrency  cryptocurrency = ALGORAND;
  double tStart = get_wall_time(), tStartSimulation, tFinish;
  const int secsPerMin = 60;
  const uint16_t bitcoinPort = 8333;
  const double realAverageBlockGenIntervalMinutes = 10; //minutes
  int start = 0;
  double stop = 0.30;

  long blockSize = -1;
  long stakeSize = -1;
  int totalNoNodes = 16;
  int minConnectionsPerNode = -1;
  int maxConnectionsPerNode = -1;
  double *minersHash;
  enum BitcoinRegion *minersRegions;
  int noMiners = 16;
  bool allPrint = false;

  bool attack = false;
  int noAttackers = 1;
  double attackPower = 0.3;

  // intervals between phases (in seconds)
  double intervalBP = 4;
  double intervalSV = 4;
  double intervalCV = 4;

  // Thresholds for VRF output Y in each phase -> this values are just example and will be changed
  // Block proposal - in test with 100 nodes was 1-5 chosen
  unsigned char vrfThresholdBP[64] = {0x08,0xff,0xff,0x65,0xa9,0xac,0xb1,0x3d,0x05,0x15,0xa4,0x22,0xa9,0xfa,0x35,0x7d,0x1b,0x53,0x33,0x86,0xa5,0xe6,0x74,0x51,0x06,0x6a,0x8e,0xd3,0x11,0x38,0x10,0x18,0x65,0x25,0x76,0xe7,0x95,0xc1,0x32,0x85,0x28,0x0c,0x14,0xf7,0x0e,0x5c,0x4c,0x91,0x37,0xb5,0x68,0x47,0x85,0xef,0x7e,0x21,0x75,0x79,0x5e,0x83,0x3a,0x35,0xc6,0x44};
  // Soft Vote - in test with 100 nodes were 30-40 chosen
  unsigned char vrfThresholdSV[64] = {0x66,0xff,0xff,0x65,0xa9,0xac,0xb1,0x3d,0x05,0x15,0xa4,0x22,0xa9,0xfa,0x35,0x7d,0x1b,0x53,0x33,0x86,0xa5,0xe6,0x74,0x51,0x06,0x6a,0x8e,0xd3,0x11,0x38,0x10,0x18,0x65,0x25,0x76,0xe7,0x95,0xc1,0x32,0x85,0x28,0x0c,0x14,0xf7,0x0e,0x5c,0x4c,0x91,0x37,0xb5,0x68,0x47,0x85,0xef,0x7e,0x21,0x75,0x79,0x5e,0x83,0x3a,0x35,0xc6,0x44};
  // Certify - in test with 100 nodes were 10-20 chosen
  unsigned char vrfThresholdCV[64] = {0x33,0xff,0xff,0x65,0xa9,0xac,0xb1,0x3d,0x05,0x15,0xa4,0x22,0xa9,0xfa,0x35,0x7d,0x1b,0x53,0x33,0x86,0xa5,0xe6,0x74,0x51,0x06,0x6a,0x8e,0xd3,0x11,0x38,0x10,0x18,0x65,0x25,0x76,0xe7,0x95,0xc1,0x32,0x85,0x28,0x0c,0x14,0xf7,0x0e,0x5c,0x4c,0x91,0x37,0xb5,0x68,0x47,0x85,0xef,0x7e,0x21,0x75,0x79,0x5e,0x83,0x3a,0x35,0xc6,0x44};

  int leadingZerosVrfBP = 5;
  int leadingZerosVrfSV = 2;
  int leadingZerosVrfCV = 4;

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
  cmd.AddValue ("blockSize", "The the fixed block size (Bytes)", blockSize);
  cmd.AddValue ("stakeSize", "The the fixed stake size", stakeSize);
  cmd.AddValue ("nodes", "The total number of nodes in the network", totalNoNodes);
//  cmd.AddValue ("miners", "The total number of miners in the network", noMiners);
  cmd.AddValue ("minConnections", "The minConnectionsPerNode of the grid", minConnectionsPerNode);
  cmd.AddValue ("maxConnections", "The maxConnectionsPerNode of the grid", maxConnectionsPerNode);
  cmd.AddValue ("test", "Test the scalability of the simulation", testScalability);
  cmd.AddValue ("unsolicited", "Change the miners block broadcast type to UNSOLICITED", unsolicited);
  cmd.AddValue ("relayNetwork", "Change the miners block broadcast type to RELAY_NETWORK", relayNetwork);
  cmd.AddValue ("unsolicitedRelayNetwork", "Change the miners block broadcast type to UNSOLICITED_RELAY_NETWORK", unsolicitedRelayNetwork);
  cmd.AddValue ("stop", "Stop simulation after X simulation minutes", stop);
  cmd.AddValue ("intervalBP", "Interval between block proposal phase and soft vote phase", intervalBP);
  cmd.AddValue ("intervalSV", "Interval between soft vote phase and certify vote phase", intervalSV);
  cmd.AddValue ("intervalCV", "Interval between certify vote phase and block proposal phase", intervalCV);
  cmd.AddValue ("lzBP", "Leading zeros in VRF threshold used in choosing committee for block proposal phase", leadingZerosVrfBP);
  cmd.AddValue ("lzSV", "Leading zeros in VRF threshold used in choosing committee for soft vote phase", leadingZerosVrfSV);
  cmd.AddValue ("lzCV", "Leading zeros in VRF threshold used in choosing committee for certify vote phase", leadingZerosVrfCV);
  cmd.AddValue ("allPrint", "On the end of simulation, each participant will print its blockchain stats", allPrint);
  cmd.AddValue ("attack", "Provide attack scenario when attacker was chosen to soft vote committee", attack);
  cmd.AddValue ("attackPower", "Wanted attack power (attacker stake : total stakes) of the attackers vote", attackPower);

  cmd.Parse(argc, argv);

  // all nodes are participants
  noMiners = totalNoNodes;

  // create VRF thresholds for each phase, based on choosed leading zeros by user
  createVRFThreshold(vrfThresholdBP, leadingZerosVrfBP);
  createVRFThreshold(vrfThresholdSV, leadingZerosVrfSV);
  createVRFThreshold(vrfThresholdCV, leadingZerosVrfCV);


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
  AlgorandParticipantHelper algorandVoterHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), bitcoinPort),
                                          nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0],
                                          nodesInternetSpeeds[0], stats);
  ApplicationContainer algorandVoters;
  int count = 0;

  for(auto &miner : miners)
  {
	Ptr<Node> targetNode = bitcoinTopologyHelper.GetNode (miner);

	if (systemId == targetNode->GetSystemId())
	{
      algorandVoterHelper.SetAttribute("Cryptocurrency", UintegerValue(ALGORAND));

      if (blockSize != -1)
        algorandVoterHelper.SetAttribute("FixedBlockSize", UintegerValue(blockSize));
      if (stakeSize != -1)
        algorandVoterHelper.SetAttribute("FixedStakeSize", UintegerValue(stakeSize));

      if(systemId == 0 && attack && noAttackers != 0){
        algorandVoterHelper.SetAttribute("IsAttacker", BooleanValue(true));
        algorandVoterHelper.SetAttribute("AttackPower", DoubleValue(attackPower));
        noAttackers--;
      }else{
        algorandVoterHelper.SetAttribute("IsAttacker", BooleanValue(false));
      }

      algorandVoterHelper.SetPeersAddresses (nodesConnections[miner]);
	  algorandVoterHelper.SetPeersDownloadSpeeds (peersDownloadSpeeds[miner]);
	  algorandVoterHelper.SetPeersUploadSpeeds (peersUploadSpeeds[miner]);
	  algorandVoterHelper.SetNodeInternetSpeeds (nodesInternetSpeeds[miner]);
	  algorandVoterHelper.SetNodeStats (&stats[miner]);

	  if(unsolicited)
	    algorandVoterHelper.SetBlockBroadcastType (UNSOLICITED);
	  if(relayNetwork)
	    algorandVoterHelper.SetBlockBroadcastType (RELAY_NETWORK);
	  if(unsolicitedRelayNetwork)
	    algorandVoterHelper.SetBlockBroadcastType (UNSOLICITED_RELAY_NETWORK);

	  algorandVoterHelper.SetVrfThresholdBP(vrfThresholdBP);
	  algorandVoterHelper.SetVrfThresholdSV(vrfThresholdSV);
	  algorandVoterHelper.SetVrfThresholdCV(vrfThresholdCV);

	  algorandVoterHelper.SetIntervalBP(intervalBP);
	  algorandVoterHelper.SetIntervalSV(intervalSV);
	  algorandVoterHelper.SetIntervalCV(intervalCV);

	  algorandVoterHelper.SetAllPrint(allPrint);

	  algorandVoters.Add(algorandVoterHelper.Install (targetNode));

	  if (systemId == 0)
        nodesInSystemId0++;
	}
	count++;
  }
  algorandVoters.Start (Seconds (start));
  algorandVoters.Stop (Minutes (stop));


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
        bitcoinNodeHelper.SetAttribute("Cryptocurrency", UintegerValue(ALGORAND));

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

  collectAndPrintStats(stats, totalNoNodes, noMiners,
                          systemId, systemCount, nodesInSystemId0,
                          tStart, tStartSimulation, tFinish, stop,
                          minConnectionsPerNode, maxConnectionsPerNode, secsPerMin,
                          (intervalBP+intervalSV+intervalCV)/60, relayNetwork,
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


void collectAndPrintStats(nodeStatistics *stats, int totalNoNodes, int noMiners,
                          uint32_t systemId, uint32_t systemCount, int nodesInSystemId0,
                          double tStart, double tStartSimulation, double tFinish, double stop,
                          int minConnectionsPerNode, int maxConnectionsPerNode, int secsPerMin,
                          double averageBlockGenIntervalMinutes, bool relayNetwork,
                          BitcoinTopologyHelper *bitcoinTopologyHelper){

#ifdef MPI_TEST

    int            blocklen[49] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1 ,1,
                                   1, 1, 1, 1, 1, 0, 1, 1, 1};
    MPI_Aint       disp[49];
    MPI_Datatype   dtypes[49] = {MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT,
                                 MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG,
                                 MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_INT, MPI_INT, MPI_INT, MPI_LONG, MPI_LONG, MPI_INT, MPI_LONG,MPI_LONG,
                                 MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
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
    disp[40] = offsetof(nodeStatistics, maxBlockPropagationTime);
    disp[41] = offsetof(nodeStatistics, meanBPCommitteeSize);
    disp[42] = offsetof(nodeStatistics, meanSVCommitteeSize);
    disp[43] = offsetof(nodeStatistics, meanCVCommitteeSize);
    disp[44] = offsetof(nodeStatistics, meanSVStakeSize);
    disp[45] = offsetof(nodeStatistics, isAttacker);
    disp[46] = offsetof(nodeStatistics, countSVCommitteeMember);
    disp[47] = offsetof(nodeStatistics, successfulInsertions);
    disp[48] = offsetof(nodeStatistics, successfulInsertionBlocks);

    MPI_Type_create_struct (49, blocklen, disp, dtypes, &mpi_nodeStatisticsType);
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
            stats[recv.nodeId].meanBPCommitteeSize = recv.meanBPCommitteeSize;
            stats[recv.nodeId].meanSVCommitteeSize = recv.meanSVCommitteeSize;
            stats[recv.nodeId].meanCVCommitteeSize = recv.meanCVCommitteeSize;
            stats[recv.nodeId].meanSVStakeSize = recv.meanSVStakeSize;
            stats[recv.nodeId].isAttacker = recv.isAttacker;
            stats[recv.nodeId].countSVCommitteeMember = recv.countSVCommitteeMember;
            stats[recv.nodeId].successfulInsertions = recv.successfulInsertions;
            stats[recv.nodeId].successfulInsertionBlocks = recv.successfulInsertionBlocks;
            count++;
        }
    }
#endif

    if (systemId == 0)
    {
        tFinish=get_wall_time();

        //PrintStatsForEachNode(stats, totalNoNodes);
        PrintTotalStats(stats, totalNoNodes, tStartSimulation, tFinish, averageBlockGenIntervalMinutes, relayNetwork);

        std::cout << "\nThe simulation ran for " << tFinish - tStart << "s simulating "
                  << stop << "mins. Performed " << stop * secsPerMin / (tFinish - tStart)
                  << " faster than realtime.\n" << "Setup time = " << tStartSimulation - tStart << "s\n"
                  <<"It consisted of " << totalNoNodes << " nodes (" << noMiners << " miners) with minConnectionsPerNode = "
                  << minConnectionsPerNode << " and maxConnectionsPerNode = " << maxConnectionsPerNode
                  << ".\nThe averageBlockGenIntervalMinutes was " << averageBlockGenIntervalMinutes << "min.\n";

    }
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
    std::cout << "The total received BLOCK messages were " << stats[it].blockReceivedBytes << " Bytes\n";
    std::cout << "The total received VOTE messages were " << stats[it].voteReceivedBytes << " Bytes\n";
    std::cout << "The total sent BLOCK messages were " << stats[it].blockSentBytes << " Bytes\n";
    std::cout << "The total sent VOTE messages were " << stats[it].voteSentBytes << " Bytes\n";

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
  double     meanBlockSize = 0;
  double     totalBlocks = 0;
  double     blocksInBlockchain = 0;
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

  double   meanBPCommitteeSize = 0;
  double   meanSVCommitteeSize = 0;
  double   meanCVCommitteeSize = 0;
  double   meanAttSVStakeSize = 0;
  double   meanNonAttSVStakeSize = 0;

  int      attackers = 0;
  int      attackerCountSVCommitteeMember = 0;
  int      attackerSuccessfulInsertions = 0;
  int      attackerSuccessfulInsertionBlocks = 0;

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
    maxBlockPropagationTime = stats[it].maxBlockPropagationTime > maxBlockPropagationTime ? stats[it].maxBlockPropagationTime : maxBlockPropagationTime;

    blocksInBlockchain = stats[it].totalBlocks > blocksInBlockchain ? stats[it].totalBlocks : blocksInBlockchain;
    totalBlocks += totalBlocks;
    staleBlocks = stats[it].staleBlocks > staleBlocks ? stats[it].staleBlocks : staleBlocks;

    blockReceivedBytes = blockReceivedBytes*it/static_cast<double>(it + 1) + stats[it].blockReceivedBytes/static_cast<double>(it + 1);
    blockSentBytes = blockSentBytes*it/static_cast<double>(it + 1) + stats[it].blockSentBytes/static_cast<double>(it + 1);
    voteReceivedBytes = voteReceivedBytes*it/static_cast<double>(it + 1) + stats[it].voteReceivedBytes/static_cast<double>(it + 1);
    voteSentBytes = voteSentBytes*it/static_cast<double>(it + 1) + stats[it].voteSentBytes/static_cast<double>(it + 1);

    longestFork = longestFork*it/static_cast<double>(it + 1) + stats[it].longestFork/static_cast<double>(it + 1);
    blocksInForks = blocksInForks*it/static_cast<double>(it + 1) + stats[it].blocksInForks/static_cast<double>(it + 1);

    meanBPCommitteeSize = meanBPCommitteeSize*it/static_cast<double>(it + 1) + stats[it].meanBPCommitteeSize/static_cast<double>(it + 1);
    meanSVCommitteeSize = meanSVCommitteeSize*it/static_cast<double>(it + 1) + stats[it].meanSVCommitteeSize/static_cast<double>(it + 1);
    meanCVCommitteeSize = meanCVCommitteeSize*it/static_cast<double>(it + 1) + stats[it].meanCVCommitteeSize/static_cast<double>(it + 1);

    if(stats[it].isAttacker == 1){
        attackerCountSVCommitteeMember = attackerCountSVCommitteeMember*it/static_cast<double>(it + 1) + stats[it].countSVCommitteeMember/static_cast<double>(it + 1);
        attackerSuccessfulInsertions += stats[it].successfulInsertions;
        attackerSuccessfulInsertionBlocks += stats[it].successfulInsertionBlocks;
        meanAttSVStakeSize = meanAttSVStakeSize*it/static_cast<double>(it + 1) + stats[it].meanSVStakeSize/static_cast<double>(it + 1);
    }else{
        meanNonAttSVStakeSize = meanNonAttSVStakeSize*it/static_cast<double>(it + 1) + stats[it].meanSVStakeSize/static_cast<double>(it + 1);
    }

    propagationTimes.push_back(stats[it].meanBlockPropagationTime);

    download = stats[it].blockReceivedBytes + stats[it].voteReceivedBytes;
    upload = stats[it].blockSentBytes + stats[it].voteSentBytes;

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

  averageBandwidthPerNode = blockReceivedBytes + blockSentBytes + voteReceivedBytes + voteSentBytes;

  totalBlocks /= totalNodes;
//  staleBlocks /= totalNodes;
  sort(propagationTimes.begin(), propagationTimes.end());
  sort(minersPropagationTimes.begin(), minersPropagationTimes.end());
  sort(blockTimeouts.begin(), blockTimeouts.end());
  sort(chunkTimeouts.begin(), chunkTimeouts.end());

  double median = *(propagationTimes.begin()+propagationTimes.size()/2);
  double p_10 = *(propagationTimes.begin()+int(propagationTimes.size()*.1));
  double p_25 = *(propagationTimes.begin()+int(propagationTimes.size()*.25));
  double p_75 = *(propagationTimes.begin()+int(propagationTimes.size()*.75));
  double p_90 = *(propagationTimes.begin()+int(propagationTimes.size()*.90));
  double minersMedian = minersPropagationTimes.size() ==0 ? 0 : *(minersPropagationTimes.begin()+int(minersPropagationTimes.size()/2));

  std::cout << "\nTotal Stats:\n";
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
  std::cout << "Miners Median Block Propagation Time = " << minersMedian << "s\n";
  std::cout << "Mean Block Size = " << pretty_bytes(meanBlockSize) << "\n";
  std::cout << "Total Blocks = " << blocksInBlockchain << "\n";
  std::cout << "Stale Blocks = " << staleBlocks << " ("
            << 100. * staleBlocks / blocksInBlockchain << "%)\n";
  std::cout << "The size of the longest fork was " << longestFork << " blocks\n";
  std::cout << "There were in total " << blocksInForks << " blocks in forks\n";

    std::cout << "Mean Block Proposal Committee Size = " << meanBPCommitteeSize << "\n";
    std::cout << "Mean Soft Vote Committee Size = " << meanSVCommitteeSize << "\n";
    std::cout << "Mean Certify Vote Committee Size = " << meanCVCommitteeSize << "\n";
    std::cout << "Mean NonAttacker Soft vote Stake = " << meanNonAttSVStakeSize << "\n";
    std::cout << "Mean Attacker Soft vote Stake = " << meanAttSVStakeSize << "\n";
    std::cout << "Attackers = " << attackers << "\n";
    std::cout << "Mean attacker member of Soft vote committee = " << attackerCountSVCommitteeMember << "x\n";
    std::cout << "Total Successful Insertions = " << attackerSuccessfulInsertions << "\n";
    std::cout << "Total Successful Insertion Blocks = " << attackerSuccessfulInsertionBlocks << "\n";

    std::cout << "The average received BLOCK messages were " << pretty_bytes(blockReceivedBytes) << " ("
          << 100. * blockReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent BLOCK messages were " << pretty_bytes(blockSentBytes) << " ("
          << 100. * blockSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received VOTE messages were " << pretty_bytes(voteReceivedBytes) << " ("
          << 100. * voteReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent VOTE messages were " << pretty_bytes(voteSentBytes) << " ("
          << 100. * voteSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to BLOCK messages = " << pretty_bytes(blockReceivedBytes +  blockSentBytes) << " ("
              << 100. * (blockReceivedBytes +  blockSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to VOTE messages = " << pretty_bytes(voteReceivedBytes +  voteSentBytes) << " ("
              << 100. * (voteReceivedBytes +  voteSentBytes) / averageBandwidthPerNode << "%)\n";
  std::cout << "Total average traffic/node = " << pretty_bytes(averageBandwidthPerNode) << " ("
            << averageBandwidthPerNode / (1000 *(blocksInBlockchain - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8
            << " Kbps and " << pretty_bytes(averageBandwidthPerNode / (1000 * (blocksInBlockchain - 1))) << "/block)\n";
  std::cout << (finish - start)/ (blocksInBlockchain - 1)<< "s per generated block\n";


  std::cout << "\nBlock Propagation Times = [";
  for(auto it = propagationTimes.begin(); it != propagationTimes.end(); it++)
  {
    if (it == propagationTimes.begin())
      std::cout << *it;
    else
      std::cout << ", " << *it ;
  }
  std::cout << "]\n" ;

  std::cout << "\nVoters Block Propagation Times = [";
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

  std::cout << "\n";
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

