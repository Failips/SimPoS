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
