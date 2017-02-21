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
#include "RFNoC_ResourceManager.h"
#include "RFNoC_Utils.h"

#define LOG_TRACE_ID(classname, id, expression)     LOG_TRACE(classname, id << ":" << expression)
#define LOG_DEBUG_ID(classname, id, expression)     LOG_DEBUG(classname, id << ":" << expression)
#define LOG_INFO_ID(classname, id, expression)      LOG_INFO(classname, id << ":" << expression)
#define LOG_WARN_ID(classname, id, expression)      LOG_WARN(classname, id << ":" << expression)
#define LOG_ERROR_ID(classname, id, expression)     LOG_ERROR(classname, id << ":" << expression)
#define LOG_FATAL_ID(classname, id, expression)     LOG_FATAL(classname, id << ":" << expression)

// An enumeration for the connection type
enum ConnectionType { NONE, FABRIC, RADIO, STREAMER };

// Forward declaration of other classes
class RFNoC_ResourceManager;

class RFNoC_Resource
{
    ENABLE_LOGGING
    public:
        RFNoC_Resource(std::string resourceID, RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph, connectRadioRXCallback connectRadioRxCb, connectRadioTXCallback connectRadioTxCb);
        virtual ~RFNoC_Resource();

        BlockInfo getProvidesBlock() const;
        std::vector<std::string> getProvidesStrings() const;
        BlockInfo getUsesBlock() const;
        bool hasString(std::string portString) const;
        std::string id() const { return this->ID; }
        Resource_impl *instantiate(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void newIncomingConnection(const std::string &ID);
        void newOutgoingConnection(const std::string &ID);
        void removedIncomingConnection(const std::string &ID);
        void removedOutgoingConnection(const std::string &ID);
        void setRxStreamer(bool enable);
        void setTxStreamer(bool enable);
        //bool update();

        //bool operator==(const RFNoC_Resource &rhs) const;

        void setBlockInfos(const std::vector<BlockInfo> &blockInfos);
        void setSetRxStreamer(setStreamerCallback cb);
        void setSetTxStreamer(setStreamerCallback cb);

    private:
        std::vector<BlockInfo> blockInfos;
        std::map<std::string, ConnectionType> connectionIdToConnectionType;
        connectRadioRXCallback connectRadioRxCb;
        connectRadioTXCallback connectRadioTxCb;
        uhd::rfnoc::graph::sptr graph;
        std::string ID;
        bool isRxStreamer;
        bool isTxStreamer;
        std::vector<std::string> providesStrings;
        std::vector<BULKIO::ProvidesPortStatisticsProvider_ptr> providesPorts;
        RFNoC_ResourceManager *resourceManager;
        Resource_impl *rhResource;
        setStreamerCallback setRxStreamerCb;
        setStreamerCallback setTxStreamerCb;
        std::map<std::string, ConnectionType> streamIdToConnectionType;
        std::vector<BULKIO::UsesPortStatisticsProvider_ptr> usesPorts;
};

#endif
