/*
 * RFNoC_ResourceManager.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#include "RFNoC_ResourceManager.h"

PREPARE_LOGGING(RFNoC_ResourceManager)

RFNoC_ResourceManager::RFNoC_ResourceManager(Device_impl *parent, uhd::device3::sptr usrp, connectRadioRXCallback rxCb, connectRadioTXCallback txCb) :
    connectRadioRXCb(rxCb),
    connectRadioTXCb(txCb),
    parent(parent),
    usrp(usrp)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    this->connectionThread = new boost::thread(boost::bind(&RFNoC_ResourceManager::connectionHandler, this));
    this->graph = this->usrp->create_graph("RFNoC_ResourceManager_" + ossie::generateUUID());
}

RFNoC_ResourceManager::~RFNoC_ResourceManager()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    this->connectionCondition.notify_one();
    this->connectionThread->join();
    delete this->connectionThread;

    for (RFNoC_ResourceMap::iterator it = this->idToResource.begin(); it != this->idToResource.end(); ++it) {
        delete it->second;
    }
}

Resource_impl* RFNoC_ResourceManager::addResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    boost::mutex::scoped_lock lock(this->resourceLock);

    std::string resourceID;

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "COMPONENT_IDENTIFIER") == 0) {
            resourceID = argv[i+1];
            break;
        }
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Adding Resource with ID: " << resourceID);

    RFNoC_ResourceMap::iterator resourceMapIt = this->idToResource.find(resourceID);

    if (resourceMapIt != this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to add a Resource already tracked by the Resource Manager.");
        return NULL;
    }

    // Instantiate the resource
    LOG_DEBUG(RFNoC_ResourceManager, "Instantiating new RFNoC_Resource");

    RFNoC_Resource *rfNocResource = new RFNoC_Resource(resourceID, this, this->graph, this->connectRadioRXCb, this->connectRadioTXCb);

    if (not rfNocResource) {
        LOG_ERROR(RFNoC_ResourceManager, "Failed to instantiate new RFNoC_Resource");
        return NULL;
    }

    // Map the RFNoC_Resource
    this->idToResource[resourceID] = rfNocResource;

    // Instantiate the resource
    Resource_impl *resource = rfNocResource->instantiate(argc, argv, fnptr, libraryName);

    if (not resource) {
        LOG_ERROR(RFNoC_ResourceManager, "Failed to instantiate REDHAWK Resource");
        delete rfNocResource;
        this->idToResource.erase(resourceID);
        return NULL;
    }

    return resource;
}

void RFNoC_ResourceManager::removeResource(const std::string &resourceID)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    boost::mutex::scoped_lock lock(this->resourceLock);

    LOG_DEBUG(RFNoC_ResourceManager, "Removing Resource with ID: " << resourceID);

    if (this->idToResource.find(resourceID) == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to remove a Resource not tracked by the Resource Manager");
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Unmapping Resource from RFNoC_ResourceManager");

    delete this->idToResource[resourceID];

    this->idToResource.erase(resourceID);
}

BlockInfo RFNoC_ResourceManager::getBlockInfoFromHash(const CORBA::ULong &hash) const
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    for (RFNoC_ResourceMap::const_iterator it = this->idToResource.begin(); it != this->idToResource.end(); ++it) {
        if (it->second->hasHash(hash)) {
            return it->second->getProvidesBlock();
        }
    }

    return BlockInfo();
}

void RFNoC_ResourceManager::registerIncomingConnection(IncomingConnection connection)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    boost::mutex::scoped_lock lock(this->connectionLock);

    LOG_DEBUG(RFNoC_ResourceManager, "Got lock");

    this->pendingConnections.push_back(connection);

    LOG_DEBUG(RFNoC_ResourceManager, "Pushed connection");

    this->connectionCondition.notify_one();

    LOG_DEBUG(RFNoC_ResourceManager, "Notified");
}

void RFNoC_ResourceManager::setBlockInfoMapping(const std::string &resourceID, const std::vector<BlockInfo> &blockInfos)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set Block ID for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting block IDs for Resource");

    it->second->setBlockInfos(blockInfos);
}

void RFNoC_ResourceManager::setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set setRxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting setSetRxStreamer callback for Resource");

    it->second->setSetRxStreamer(cb);
}

void RFNoC_ResourceManager::setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set setTxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting setSetTxStreamer callback for Resource");

    it->second->setSetTxStreamer(cb);
}

void RFNoC_ResourceManager::connectionHandler()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    while (true) {
        LOG_DEBUG(RFNoC_ResourceManager, "Before lock in connectionHandler");

        boost::mutex::scoped_lock lock(this->connectionLock);

        LOG_DEBUG(RFNoC_ResourceManager, "After lock in connectionHandler");

        if (boost::this_thread::interruption_requested()) {
            LOG_DEBUG(RFNoC_ResourceManager, "Interruption requested in connectionHandler");
            return;
        }

        while (this->pendingConnections.empty()) {
            LOG_DEBUG(RFNoC_ResourceManager, "Empty, waiting in connectionHandler");
            this->connectionCondition.wait(lock);
            LOG_DEBUG(RFNoC_ResourceManager, "Condition received in connectionHandler");

            if (boost::this_thread::interruption_requested()) {
                LOG_DEBUG(RFNoC_ResourceManager, "Interruption requested in connectionHandler");
                return;
            }
        }

        LOG_DEBUG(RFNoC_ResourceManager, "Copying over pending connections");

        std::vector<IncomingConnection> copy = this->pendingConnections;

        LOG_DEBUG(RFNoC_ResourceManager, "Clearing pending connections in connectionHandler");

        this->pendingConnections.clear();

        LOG_DEBUG(RFNoC_ResourceManager, "Cleared pending connections in connectionHandler");

        LOG_DEBUG(RFNoC_ResourceManager, "Releasing lock");

        lock.unlock();

        LOG_DEBUG(RFNoC_ResourceManager, "Iterating over copy");

        for (size_t i = 0; i < copy.size(); ++i) {
            IncomingConnection connection = copy[i];

            RFNoC_Resource *resource = this->idToResource[connection.resourceID];

            LOG_DEBUG(RFNoC_ResourceManager, "About to handle incoming connection in connectionHandler");
            resource->handleIncomingConnection(connection.streamID, connection.portHash);
            LOG_DEBUG(RFNoC_ResourceManager, "Handled incoming connection in connectionHandler");
        }
    }
}
