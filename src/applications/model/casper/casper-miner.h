//
// Created by mak10_000 on 7.3.2021.
//

#ifndef SIMPOS_CASPERMINER_H
#define SIMPOS_CASPERMINER_H

#include "ns3/bitcoin-miner.h"
#include "ns3/casper-participant.h"


namespace ns3 {

    class Address;

    class Socket;

    class Packet;


    class CasperMiner : public CasperParticipant {

    public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        CasperMiner();

        virtual ~CasperMiner(void);

    protected:
        // inherited from Application base class.
        virtual void StartApplication(void);    // Called at time specified by Start
        virtual void StopApplication(void);     // Called at time specified by Stop

        virtual void DoDispose(void);

        /**
         * \brief Mines a new block and advertises it to its peers
         */
        virtual void MineBlock (void);

        /**
         * \brief Called for blocks with better score(height). Removes m_nextMiningEvent and call MineBlock again.
         * \param newBlock the new block which was received
         */
        virtual void ReceivedHigherBlock(const Block &newBlock);

        /**
         * \brief Handle a document received by the application with unknown type number
         * \param document the received document
         */
        virtual void HandleCustomRead (rapidjson::Document *document, double receivedTime, Address receivedFrom);

        /**
         * handling insertion of block to blockchain to count blocks in epoch and know when to vote
         * @param newBlock
         */
        virtual void InsertBlockToBlockchain(Block &newBlock);

        //debug
        double       m_timeStart;
        double       m_timeFinish;
        enum BlockBroadcastType   m_blockBroadcastType;      //!< the type of broadcast for votes and blocks
    };
}

#endif //SIMPOS_CASPERMINER_H
