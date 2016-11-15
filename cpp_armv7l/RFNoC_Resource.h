/*
 * RFNoC_Resource.h
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#ifndef RFNOC_RESOURCE_H
#define RFNOC_RESOURCE_H

#include <bulkio/bulkio.h>
#include <ossie/debug.h>
#include <ossie/CF/QueryablePort.h>
#include <ossie/Resource_impl.h>
#include <string>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/graph.hpp>

#include "entry_point.h"
#include "RFNoC_Component.h"
#include "RFNoC_ResourceList.h"
#include "RFNoC_ResourceManager.h"

#define LOG_TRACE_ID(classname, id, expression)     LOG_TRACE(classname, id << ":" << expression)
#define LOG_DEBUG_ID(classname, id, expression)     LOG_DEBUG(classname, id << ":" << expression)
#define LOG_INFO_ID(classname, id, expression)      LOG_INFO(classname, id << ":" << expression)
#define LOG_WARN_ID(classname, id, expression)      LOG_WARN(classname, id << ":" << expression)
#define LOG_ERROR_ID(classname, id, expression)     LOG_ERROR(classname, id << ":" << expression)
#define LOG_FATAL_ID(classname, id, expression)     LOG_FATAL(classname, id << ":" << expression)

// Forward declaration of other classes
class RFNoC_ResourceManager;

class RFNoC_Resource
{
    ENABLE_LOGGING
    public:
        RFNoC_Resource(std::string resourceID, RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph);
        virtual ~RFNoC_Resource();

        bool connect(const RFNoC_Resource &provides);
        std::vector<std::string> getConnectionIDs() const;
        uhd::rfnoc::block_id_t getProvidesBlock() const;
        std::vector<CORBA::ULong> getProvidesHashes() const { return this->providesHashes; }
        uhd::rfnoc::block_id_t getUsesBlock() const;
        bool hasHash(CORBA::ULong hash) const;
        std::string id() const { return this->ID; }
        Resource_impl *instantiate(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void setRxStreamer(bool enable);
        void setTxStreamer(bool enable);
        bool update();

        bool operator==(const RFNoC_Resource *rhs) const;

        void setBlockIDs(const std::vector<uhd::rfnoc::block_id_t> &blockIDs);
        void setSetRxStreamer(setStreamerCallback cb);
        void setSetTxStreamer(setStreamerCallback cb);

    private:
        typedef std::map<CORBA::ULong, ExtendedCF::UsesConnectionSequence *> hashToConnectionSequence;
        typedef std::pair<CORBA::ULong, CORBA::ULong> portHashPair;

        std::vector<uhd::rfnoc::block_id_t> blockIDs;
        std::vector<portHashPair> connected;
        uhd::rfnoc::graph::sptr graph;
        std::string ID;
        bool isRxStreamer;
        bool isTxStreamer;
        std::vector<CORBA::ULong> providesHashes;
        RFNoC_ResourceManager *resourceManager;
        Resource_impl *rhResource;
        setStreamerCallback setRxStreamerCb;
        setStreamerCallback setTxStreamerCb;
        hashToConnectionSequence usesHashToPreviousConnections;
        std::vector<BULKIO::UsesPortStatisticsProvider_ptr> usesPorts;
};

#endif
