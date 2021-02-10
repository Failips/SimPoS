/**
 * This file contains the definitions of the functions declared in algorand-participant-helper.h
 */

#include "algorand-participant-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/algorand-participant.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <algorithm>


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("AlgorandParticipantHelper");

    AlgorandParticipantHelper::AlgorandParticipantHelper (std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address,
                                            double> &peersUploadSpeeds, nodeInternetSpeeds &internetSpeeds, nodeStatistics *stats) :
                                            BitcoinNodeHelper (),  m_minerType (NORMAL_MINER), m_blockBroadcastType (STANDARD)

    {
        m_factory.SetTypeId ("ns3::AlgorandParticipant");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
        m_noMiners = noMiners;

        std::random_device rd; // obtain a random number from hardware
        m_generator.seed(rd()); // seed the generator

        // committee size distribution with μ set to 9 (https://www.youtube.com/watch?v=CFuzi-ZGwDY)
        std::poisson_distribution<int> committeeDistribution(9);
        m_committeeSizeDistribution = committeeDistribution;

        // block proposal member distribution (nod based on amount of stake)
        std::uniform_int_distribution<> bpDistribution(0, m_noMiners - 1);
        m_memberDistribution = bpDistribution;
    }

    Ptr<Application>
    AlgorandParticipantHelper::InstallPriv (Ptr<Node> node) //FIX ME
    {

        switch (m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<AlgorandParticipant> app = m_factory.Create<AlgorandParticipant> ();
                app->SetPeersAddresses(m_peersAddresses);
                app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
                app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
                app->SetNodeInternetSpeeds(m_internetSpeeds);
                app->SetNodeStats(m_nodeStats);
                app->SetBlockBroadcastType(m_blockBroadcastType);
                app->SetProtocolType(m_protocolType);

                app->SetHelper(this);

                node->AddApplication (app);
                return app;
            }

        }

    }

    enum MinerType
    AlgorandParticipantHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    AlgorandParticipantHelper::SetMinerType(enum MinerType m)
    {
        m_minerType = m;

        switch (m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId ("ns3::AlgorandParticipant");
                SetFactoryAttributes();
                break;
            }
        }
    }


    void
    AlgorandParticipantHelper::SetBlockBroadcastType (enum BlockBroadcastType m)
    {
        m_blockBroadcastType = m;
    }


    void
    AlgorandParticipantHelper::SetFactoryAttributes (void)
    {
        m_factory.Set ("Protocol", StringValue (m_protocol));
        m_factory.Set ("Local", AddressValue (m_address));
    }


    bool AlgorandParticipantHelper::IsChosenByVRF(int iteration, int participantId, enum AlgorandPhase algorandPhase) {
        NS_LOG_FUNCTION (this);

        // choose committee by actual phase
        std::vector<std::vector<int>> *phaseCommittee;
        if(algorandPhase == BLOCK_PROPOSAL_PHASE)
            phaseCommittee = &m_committeeBP;
        else if(algorandPhase == SOFT_VOTE_PHASE)
            phaseCommittee = &m_committeeSV;
        else
            return false;

        // check if VRF has chosen members for this iteration and if no do so
        if(phaseCommittee->size() < iteration)
            CreateCommittee(iteration, algorandPhase);

        // check if participant is member of committee
        std::vector<int> committee = phaseCommittee->at(iteration-1);
        for(auto i = committee.begin(); i != committee.end(); i++){
            if(*i == participantId)
                return true;
        }

        return false;
    }

    void AlgorandParticipantHelper::CreateCommittee(int iteration, enum AlgorandPhase algorandPhase) {
        NS_LOG_FUNCTION (this);
        int committeeSize;

        // choose committee by actual phase
        std::vector<std::vector<int>> *phaseCommittee;
        if(algorandPhase == BLOCK_PROPOSAL_PHASE)
            phaseCommittee = &m_committeeBP;
        else if(algorandPhase == SOFT_VOTE_PHASE)
            phaseCommittee = &m_committeeSV;
        else
            return;

        // resize committee vector
        int actSize = phaseCommittee->size();
        phaseCommittee->resize(iteration);

        // create committees for missing iterations
        for(int i = actSize; i<iteration; i++){
            // get random size of committee
            committeeSize = m_committeeSizeDistribution(m_generator);
            // for creating quorum we need at least 3 members
            if(committeeSize < 3 || committeeSize > m_noMiners)
                committeeSize = m_noMiners;

            NS_LOG_INFO("Committee size: " << committeeSize);

            for(int m=0; m<committeeSize; ){
                int memberId = m_memberDistribution(m_generator);// get random between 0 and participants-1

                // check if member is not already in committee
                if(std::find(phaseCommittee->at(i).begin(),
                             phaseCommittee->at(i).end(),
                             memberId) == phaseCommittee->at(i).end()){

                    // insert new member id to vector
                    phaseCommittee->at(i).push_back(memberId);
                    NS_LOG_INFO("Committee num. " << (i+1) << "; inserting value: " << memberId);
                    m++;
                }
            }
        }
    }

} // namespace ns3


