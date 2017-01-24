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

            BlockInfo providesBlock, usesBlock;

            providesBlock = this->getProvidesBlock();
            usesBlock = this->getUsesBlock();

            this->graph->connect(usesBlock.blockID, usesBlock.port, providesBlock.blockID, providesBlock.port);

            this->connected.push_back(hashPair);
            connect = true;
            break;
        }

        delete connections;

        return connect;
    }

    return false;
}

std::vector<std::string> RFNoC_Resource::getConnectionIDs() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    std::vector<std::string> connectionIDs;

    for (hashToConnectionIDs::const_iterator it = this->usesHashToPreviousConnectionIDs.begin(); it != this->usesHashToPreviousConnectionIDs.end(); ++it) {
        for (size_t i = 0; i < it->second.size(); ++i) {
            connectionIDs.push_back(it->second[i]);
        }
    }

    return connectionIDs;
}

BlockInfo RFNoC_Resource::getProvidesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockInfos.front();
}

std::vector<CORBA::ULong> RFNoC_Resource::getProvidesHashes() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    std::vector<CORBA::ULong> providesHashes;

    for (hashToStreamIDs::const_iterator it = this->providesHashToPreviousStreamIDs.begin(); it != this->providesHashToPreviousStreamIDs.end(); ++it) {
        providesHashes.push_back(it->first);
    }

    return providesHashes;
}

BlockInfo RFNoC_Resource::getUsesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockInfos.back();
}

bool RFNoC_Resource::hasHash(CORBA::ULong hash) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->providesHashToPreviousStreamIDs.find(hash) != this->providesHashToPreviousStreamIDs.end();
}

Resource_impl* RFNoC_Resource::instantiate(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    blockInfoCallback blockInfoCb = boost::bind(&RFNoC_ResourceManager::setBlockInfoMapping, this->resourceManager, _1, _2);
    setSetStreamerCallback setSetRxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetRxStreamer, this->resourceManager, _1, _2);
    setSetStreamerCallback setSetTxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetTxStreamer, this->resourceManager, _1, _2);

    // Attempt to instantiate the resource
    bool failed = false;

    try {
        this->rhResource = fnptr(argc, argv, this->resourceManager->getParent(), this->resourceManager->getUsrp(), blockInfoCb, setSetRxStreamerCb, setSetTxStreamerCb);

        if (not rhResource) {
            LOG_ERROR(RFNoC_Resource, "Failed to instantiate RF-NoC resource");
            failed = true;
        }
    } catch(...) {
        LOG_ERROR(RFNoC_Resource, "Exception occurred while instantiating RF-NoC resource");
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
            BULKIO::ProvidesPortStatisticsProvider_ptr providesPort = BULKIO::ProvidesPortStatisticsProvider::_narrow(this->rhResource->getPort(info.name._ptr));

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Adding provides port with hash: " << hash);

            BULKIO::PortStatistics *statistics = providesPort->statistics();

            for (size_t j = 0; j < statistics->streamIDs.length(); ++j) {
                this->providesHashToPreviousStreamIDs[hash].push_back(statistics->streamIDs[j]._retn());
            }

            delete statistics;

            this->providesPorts.push_back(providesPort);
        }

        // Store the uses port pointers
        if (strstr(info.direction._ptr, "Uses") && strstr(info.repid._ptr, "BULKIO")) {
            CORBA::ULong hash = info.obj_ptr->_hash(1024);
            BULKIO::UsesPortStatisticsProvider_ptr usesPort = BULKIO::UsesPortStatisticsProvider::_narrow(this->rhResource->getPort(info.name._ptr));

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Adding uses port with hash: " << hash);

            ExtendedCF::UsesConnectionSequence *connections = usesPort->connections();

            for (size_t j = 0; j < connections->length(); ++j) {
                ExtendedCF::UsesConnection connection = (*connections)[j];
                this->usesHashToPreviousConnectionIDs[hash].push_back(connection.connectionId._ptr);
            }

            delete connections;

            this->usesPorts.push_back(usesPort);
        }
    }

    delete portSet;

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

    for (size_t i = 0; i < this->providesPorts.size(); ++i) {
        BULKIO::ProvidesPortStatisticsProvider_ptr port = this->providesPorts[i];

        if (not ossie::corba::objectExists(port)) {
            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Port doesn't exist, skipping");
            continue;
        }

        BULKIO::PortStatistics *statistics = port->statistics();
        CORBA::ULong hash = port->_hash(1024);
        std::vector<std::string> oldStreamIDs = this->providesHashToPreviousStreamIDs[hash];
        std::vector<std::string> newStreamIDs;

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking stream IDs for provides port with hash: " << hash)

        for (size_t j = 0; j < statistics->streamIDs.length(); ++j) {
            newStreamIDs.push_back(statistics->streamIDs[j]._retn());
        }

        // For now we'll assume this is sufficient
        // TODO: Make this more robust (perhaps cast the sequences to sets and do a diff)
        if (oldStreamIDs != newStreamIDs) {
            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Length of streamIDs has changed");
            updated = true;
        }

        delete statistics;
        this->providesHashToPreviousStreamIDs[hash] = newStreamIDs;
    }

    for (size_t i = 0; i < this->usesPorts.size(); ++i) {
        BULKIO::UsesPortStatisticsProvider_ptr port = this->usesPorts[i];

        if (not ossie::corba::objectExists(port)) {
            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Port doesn't exist, skipping");
            continue;
        }

        ExtendedCF::UsesConnectionSequence *connections = port->connections();
        CORBA::ULong hash = port->_hash(1024);
        std::vector<std::string> oldConnections = this->usesHashToPreviousConnectionIDs[hash];
        std::vector<std::string> newConnections;

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking connections for uses port with hash: " << hash)

        for (size_t j = 0; j < connections->length(); ++j) {
            ExtendedCF::UsesConnection connection = (*connections)[j];
            newConnections.push_back(connection.connectionId._ptr);
        }

        // For now we'll assume this is sufficient
        // TODO: Make this more robust (perhaps cast the sequences to sets and do a diff)
        if (oldConnections != newConnections) {
            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Length of connections has changed");
            updated = true;
        }

        delete connections;
        this->usesHashToPreviousConnectionIDs[hash] = newConnections;
    }

    return updated;
}

bool RFNoC_Resource::operator==(const RFNoC_Resource &rhs) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return (this->blockInfos == rhs.blockInfos);
}

void RFNoC_Resource::setBlockInfos(const std::vector<BlockInfo> &blockInfos)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    for (size_t i = 0; i < blockInfos.size(); ++i) {
        std::string blockID = blockInfos[i].blockID.get();

        if (blockID.find("Radio") != std::string::npos) {
            LOG_ERROR(RFNoC_Resource, "Unable to claim RF-NoC Resource with ID: " << blockID);
            throw std::exception();
        }
    }

    this->blockInfos = blockInfos;
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
