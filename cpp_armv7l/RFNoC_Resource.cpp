/*
 * RFNoC_Resource.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#include "RFNoC_Resource.h"

PREPARE_LOGGING(RFNoC_Resource)

RFNoC_Resource::RFNoC_Resource(std::string resourceID, RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph) :
    graph(graph),
    ID(resourceID),
    isRxStreamer(false),
    isTxStreamer(false),
    resourceManager(resourceManager),
    rhResource(NULL)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);
}

RFNoC_Resource::~RFNoC_Resource()
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    for (hashToConnectionSequence::iterator it = this->usesHashToPreviousConnections.begin(); it != this->usesHashToPreviousConnections.end(); ++it) {
        delete it->second;
    }
}

bool RFNoC_Resource::connect(const RFNoC_Resource &provides)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    for (size_t i = 0; i < this->usesPorts.size(); ++i) {
        BULKIO::UsesPortStatisticsProvider_ptr port = this->usesPorts[i];
        CORBA::ULong usesHash = port->_hash(1024);
        ExtendedCF::UsesConnectionSequence *connections = port->connections();

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking connections for uses port with hash: " << usesHash);

        if (not connections) {
            LOG_ERROR(RFNoC_Resource, "Unable to get connections from uses port");
            return false;
        }

        bool connect = false;

        for (size_t j = 0; j < connections->length(); ++j) {
            ExtendedCF::UsesConnection connection = (*connections)[j];
            CORBA::ULong providesHash = connection.port->_hash(1024);

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking provides port with hash: " << providesHash);

            portHashPair hashPair = std::make_pair(usesHash, providesHash);

            std::vector<portHashPair>::iterator it = std::find(this->connected.begin(), this->connected.end(), hashPair);

            if (it != this->connected.end()) {
                LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Ports are already connected");
                continue;
            }

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connecting ports");

            this->graph->connect(this->getUsesBlock(), provides.getProvidesBlock());

            this->connected.push_back(hashPair);
            connect = true;
            break;
        }

        delete connections;

        return connect;
    }

    return false;
}

uhd::rfnoc::block_id_t RFNoC_Resource::getProvidesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockIDs.front();
}

uhd::rfnoc::block_id_t RFNoC_Resource::getUsesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockIDs.back();
}

bool RFNoC_Resource::hasHash(CORBA::ULong hash) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return std::find(this->providesHashes.begin(), this->providesHashes.end(), hash) != this->providesHashes.end();
}

Resource_impl* RFNoC_Resource::instantiate(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    blockIDCallback blockIdCb = boost::bind(&RFNoC_ResourceManager::setBlockIDMapping, this->resourceManager, _1, _2);
    setSetStreamerCallback setSetRxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetRxStreamer, this->resourceManager, _1, _2);
    setSetStreamerCallback setSetTxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetTxStreamer, this->resourceManager, _1, _2);

    // Attempt to instantiate the resource
    bool failed = false;

    try {
        this->rhResource = fnptr(argc, argv, this->resourceManager->getParent(), this->resourceManager->getUsrpAddress(), blockIdCb, setSetRxStreamerCb, setSetTxStreamerCb);

        if (not rhResource) {
            failed = true;
        }
    } catch(...) {
        failed = true;
    }

    if (failed) {
        throw std::exception();
    }

    // Map the port hashes
    CF::PortSet::PortInfoSequence *portSet = this->rhResource->getPortSet();

    LOG_DEBUG(RFNoC_Resource, this->rhResource->_identifier);

    for (size_t i = 0; i < portSet->length(); ++i) {
        CF::PortSet::PortInfoType info = portSet->operator [](i);

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Port Name: " << info.name._ptr);
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Port Direction: " << info.direction._ptr);
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Port Repository: " << info.repid._ptr);

        // Store the provides port hashes
        if (strstr(info.direction._ptr, "Provides") && strstr(info.repid._ptr, "BULKIO")) {
            CORBA::ULong hash = info.obj_ptr->_hash(1024);
            this->providesHashes.push_back(hash);
        }

        // Store the uses port pointers
        if (strstr(info.direction._ptr, "Uses") && strstr(info.repid._ptr, "BULKIO")) {
            CORBA::ULong hash = info.obj_ptr->_hash(1024);
            BULKIO::UsesPortStatisticsProvider_ptr usesPort = BULKIO::UsesPortStatisticsProvider::_narrow(this->rhResource->getPort(info.name._ptr));

            this->usesHashToPreviousConnections[hash] = usesPort->connections();
            this->usesPorts.push_back(usesPort);
        }
    }

    return this->rhResource;
}

void RFNoC_Resource::setRxStreamer(bool enable)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    if (enable and not this->isRxStreamer) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Setting resource as RX streamer");

        this->setRxStreamerCb(true);

        this->isRxStreamer = true;
    } else if (not enable and this->isRxStreamer) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Unsetting resource as RX streamer");

        this->setRxStreamerCb(false);

        this->isRxStreamer = false;
    }
}

void RFNoC_Resource::setTxStreamer(bool enable)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    if (enable and not this->isTxStreamer) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Setting resource as TX streamer");

        this->setTxStreamerCb(true);

        this->isTxStreamer = true;
    } else if (not enable and this->isTxStreamer) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Unsetting resource as TX streamer");

        this->setTxStreamerCb(false);

        this->isTxStreamer = false;
    }
}

bool RFNoC_Resource::update()
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    bool updated = false;

    for (size_t i = 0; i < this->usesPorts.size(); ++i) {
        BULKIO::UsesPortStatisticsProvider_ptr port = this->usesPorts[i];
        ExtendedCF::UsesConnectionSequence *connections = port->connections();
        CORBA::ULong hash = port->_hash(1024);
        ExtendedCF::UsesConnectionSequence *oldConnections = this->usesHashToPreviousConnections[hash];

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking connections for uses port with hash: " << hash)

        // For now we'll assume this is sufficient
        // TODO: Make this more robust (perhaps cast the sequences to sets and do a diff)
        if (oldConnections->length() != connections->length()) {
            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Length of connections has changed");
            updated = true;
        }

        delete oldConnections;
        this->usesHashToPreviousConnections[hash] = connections;
    }

    return updated;
}

bool RFNoC_Resource::operator==(const RFNoC_Resource *rhs) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return (this->ID == rhs->ID);
}

void RFNoC_Resource::setBlockIDs(const std::vector<uhd::rfnoc::block_id_t> &blockIDs)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    this->blockIDs = blockIDs;
}

void RFNoC_Resource::setSetRxStreamer(setStreamerCallback cb)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    this->setRxStreamerCb = cb;
}

void RFNoC_Resource::setSetTxStreamer(setStreamerCallback cb)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    this->setTxStreamerCb = cb;
}
