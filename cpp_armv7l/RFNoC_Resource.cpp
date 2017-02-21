/*
 * RFNoC_Resource.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#include "RFNoC_Resource.h"

PREPARE_LOGGING(RFNoC_Resource)

RFNoC_Resource::RFNoC_Resource(std::string resourceID, RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph, connectRadioRXCallback connectRadioRxCb, connectRadioTXCallback connectRadioTxCb) :
    connectRadioRxCb(connectRadioRxCb),
    connectRadioTxCb(connectRadioTxCb),
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

BlockInfo RFNoC_Resource::getProvidesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockInfos.front();
}

std::vector<CORBA::ULong> RFNoC_Resource::getProvidesHashes() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->providesHashes;
}

BlockInfo RFNoC_Resource::getUsesBlock() const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return this->blockInfos.back();
}

bool RFNoC_Resource::hasHash(CORBA::ULong hash) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return (std::find(this->providesHashes.begin(), this->providesHashes.end(), hash) != this->providesHashes.end());
}

Resource_impl* RFNoC_Resource::instantiate(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    blockInfoCallback blockInfoCb = boost::bind(&RFNoC_ResourceManager::setBlockInfoMapping, this->resourceManager, _1, _2);
    connectionCallback newIncomingConnectionCb = boost::bind(&RFNoC_Resource::newIncomingConnection, this, _1);
    connectionCallback newOutgoingConnectionCb = boost::bind(&RFNoC_Resource::newOutgoingConnection, this, _1);
    connectionCallback removedIncomingConnectionCb = boost::bind(&RFNoC_Resource::removedIncomingConnection, this, _1);
    connectionCallback removedOutgoingConnectionCb = boost::bind(&RFNoC_Resource::removedOutgoingConnection, this, _1);
    setSetStreamerCallback setSetRxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetRxStreamer, this->resourceManager, _1, _2);
    setSetStreamerCallback setSetTxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetTxStreamer, this->resourceManager, _1, _2);

    // Attempt to instantiate the resource
    bool failed = false;

    try {
        this->rhResource = fnptr(argc, argv, this->resourceManager->getParent(), this->resourceManager->getUsrp(), blockInfoCb, newIncomingConnectionCb, newOutgoingConnectionCb, removedIncomingConnectionCb, removedOutgoingConnectionCb, setSetRxStreamerCb, setSetTxStreamerCb);

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
            CORBA::ULong hash = info.obj_ptr->_hash(HASH_SIZE);
            BULKIO::ProvidesPortStatisticsProvider_ptr providesPort = BULKIO::ProvidesPortStatisticsProvider::_narrow(this->rhResource->getPort(info.name._ptr));

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Adding provides port with hash: " << hash);

            this->providesHashes.push_back(hash);
            this->providesPorts.push_back(providesPort);
        }

        // Store the uses port pointers
        if (strstr(info.direction._ptr, "Uses") && strstr(info.repid._ptr, "BULKIO")) {
            BULKIO::UsesPortStatisticsProvider_ptr usesPort = BULKIO::UsesPortStatisticsProvider::_narrow(this->rhResource->getPort(info.name._ptr));

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Adding uses port");

            this->usesPorts.push_back(usesPort);
        }
    }

    delete portSet;

    return this->rhResource;
}

void RFNoC_Resource::newIncomingConnection(const std::string &ID)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "New incoming connection with stream ID: " << ID);

    // Make sure this isn't a duplicate
    if (this->streamIdToConnectionType.find(ID) != this->streamIdToConnectionType.end()) {
        LOG_WARN_ID(RFNoC_Resource, this->ID, "That stream ID already exists");
        return;
    }

    this->streamIdToConnectionType[ID] = NONE;

    // Check all of the uses ports for the new connection ID
    for (size_t i = 0; i < this->providesPorts.size(); ++i) {
        CORBA::ULong hash = this->providesPorts[i]->_hash(HASH_SIZE);

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Searching for new stream ID on port with hash: " << hash);

        BULKIO::PortStatistics *statistics = this->providesPorts[i]->statistics();

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "There are " << statistics->streamIDs.length() << " stream IDs");

        for (size_t j = 0; j < statistics->streamIDs.length(); ++j) {
            std::string streamID = statistics->streamIDs[j]._retn();

            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Checking stream ID: " << streamID);

            // This connection ID matches
            if (ID == streamID) {
                LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Found correct stream ID");

                // Try connecting to the RX radio, and if that fails, set as a streamer
                LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Provides port for this connection is not managed by this RF-NoC Persona. Attempting to connect to RX Radio");

                if (this->connectRadioRxCb) {
                    BlockInfo providesBlockInfo = getProvidesBlock();

                    if (this->connectRadioRxCb(hash, providesBlockInfo.blockID, providesBlockInfo.port)) {
                        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Successfully connected to RX radio");
                        this->streamIdToConnectionType[ID] = RADIO;
                    }
                }

                if (streamIdToConnectionType[ID] == NONE) {
                    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Could not connect to TX radio. Setting as TX streamer");

                    setTxStreamer(true);

                    this->streamIdToConnectionType[ID] = STREAMER;
                }

                return;
            }
        }

        delete statistics;
    }
}

void RFNoC_Resource::newOutgoingConnection(const std::string &ID)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "New outgoing connection with connection ID: " << ID);

    // Make sure this isn't a duplicate
    if (this->connectionIdToConnectionType.find(ID) != this->connectionIdToConnectionType.end()) {
        LOG_WARN_ID(RFNoC_Resource, this->ID, "That connection ID already exists");
        return;
    }

    this->connectionIdToConnectionType[ID] = NONE;

    // Check all of the uses ports for the new connection ID
    for (size_t i = 0; i < this->usesPorts.size(); ++i) {
        CORBA::ULong hash = this->usesPorts[i]->_hash(HASH_SIZE);

        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Searching for new connection ID on port with hash: " << hash);

        ExtendedCF::UsesConnectionSequence *connections = usesPorts[i]->connections();

        for (size_t j = 0; j < connections->length(); ++j) {
            ExtendedCF::UsesConnection connection = (*connections)[j];

            // This connection ID matches
            if (ID == connection.connectionId._ptr) {
                CORBA::ULong providesHash = connection.port._ptr->_hash(HASH_SIZE);
                LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Found correct connection ID with hash: " << providesHash);

                // First ask the resource manager if it knows about the provides hash
                BlockInfo providesBlockInfo = this->resourceManager->getBlockInfoFromHash(providesHash);

                // If not, try connecting to the TX radio, and if that fails, set as a streamer
                if (not uhd::rfnoc::block_id_t::is_valid_block_id(providesBlockInfo.blockID)) {
                    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Provides port for this connection is not managed by this RF-NoC Persona. Attempting to connect to TX Radio");

                    if (this->connectRadioTxCb) {
                        BlockInfo usesBlockInfo = getUsesBlock();

                        if (this->connectRadioTxCb(ID, usesBlockInfo.blockID, usesBlockInfo.port)) {
                            LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Successfully connected to TX radio");
                            this->connectionIdToConnectionType[ID] = RADIO;
                        }
                    }

                    if (this->connectionIdToConnectionType[ID] == NONE) {
                        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Could not connect to TX radio. Setting as RX streamer");

                        setRxStreamer(true);

                        this->connectionIdToConnectionType[ID] = STREAMER;
                    }
                }
                // If so, connect to the provides RF-NoC block
                else {
                    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Provides port for this connection is managed by this RF-NoC Persona. Attempting to connect to " << providesBlockInfo.blockID.to_string());

                    BlockInfo usesBlockInfo = getUsesBlock();

                    this->graph->connect(usesBlockInfo.blockID, usesBlockInfo.port, providesBlockInfo.blockID, providesBlockInfo.port);

                    this->connectionIdToConnectionType[ID] = FABRIC;
                }

                return;
            }
        }

        delete connections;
    }
}

void RFNoC_Resource::removedIncomingConnection(const std::string &ID)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Removed incoming connection with stream ID: " << ID);

    // Make sure this isn't a duplicate
    if (this->streamIdToConnectionType.find(ID) == this->streamIdToConnectionType.end()) {
        LOG_WARN_ID(RFNoC_Resource, this->ID, "That stream ID is not in use");
        return;
    }

    // Respond to the disconnect appropriately
    if (this->streamIdToConnectionType[ID] == NONE) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Unknown connection type");
    } else if (this->streamIdToConnectionType[ID] == FABRIC) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was in fabric. Disconnect somehow...");
    } else if (this->streamIdToConnectionType[ID] == RADIO) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was in fabric to radio. Disconnect somehow...");
    } else if (this->streamIdToConnectionType[ID] == STREAMER) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was a streamer. Issue stream command...");
        setTxStreamer(false);
    }

    // Unmap this connection ID
    this->streamIdToConnectionType.erase(ID);
}

void RFNoC_Resource::removedOutgoingConnection(const std::string &ID)
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Removed outgoing connection with connection ID: " << ID);

    // Make sure this isn't a duplicate
    if (this->connectionIdToConnectionType.find(ID) == this->connectionIdToConnectionType.end()) {
        LOG_WARN_ID(RFNoC_Resource, this->ID, "That connection ID is not in use");
        return;
    }

    // Respond to the disconnect appropriately
    if (this->connectionIdToConnectionType[ID] == NONE) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Unknown connection type");
    } else if (this->connectionIdToConnectionType[ID] == FABRIC) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was in fabric. Disconnect somehow...");
    } else if (this->connectionIdToConnectionType[ID] == RADIO) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was in fabric to radio. Disconnect somehow...");
    } else if (this->connectionIdToConnectionType[ID] == STREAMER) {
        LOG_DEBUG_ID(RFNoC_Resource, this->ID, "Connection was a streamer. Issue stream command...");
        setRxStreamer(false);
    }

    // Unmap this connection ID
    this->connectionIdToConnectionType.erase(ID);
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

/*bool RFNoC_Resource::update()
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
}*/

/*bool RFNoC_Resource::operator==(const RFNoC_Resource &rhs) const
{
    LOG_TRACE_ID(RFNoC_Resource, this->ID, __PRETTY_FUNCTION__);

    return (this->blockInfos == rhs.blockInfos);
}*/

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
