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

    this->graph = this->usrp->create_graph("RFNoC_ResourceManager_" + ossie::generateUUID());
}

RFNoC_ResourceManager::~RFNoC_ResourceManager()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

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

    RFNoC_Resource *resource = new RFNoC_Resource(resourceID, this, this->graph, this->connectRadioRXCb, this->connectRadioTXCb);

    if (not resource) {
        LOG_ERROR(RFNoC_ResourceManager, "Failed to instantiate new RFNoC_Resource");
        return NULL;
    } else if (not resource->instantiate(argc, argv, fnptr, libraryName)) {
        LOG_ERROR(RFNoC_ResourceManager, "Failed to instantiate REDHAWK Resource");
        return NULL;
    }

    // Map the RFNoC_Resource
    this->idToResource[resourceID] = resource;

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

/*bool RFNoC_ResourceManager::update()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    boost::mutex::scoped_lock lock(this->resourceLock);

    bool updatedAny = false;
    std::vector<RFNoC_ResourceList *> updatedResourceLists;

    for (RFNoC_ListMap::iterator it = this->idToList.begin(); it != this->idToList.end(); ++it) {
        bool updated = it->second->update();

        if (updated) {
            updatedResourceLists.push_back(it->second);
        }

        updatedAny |= updated;
    }

    std::map<std::string, RFNoC_ResourceList *> remappedLists;

    for (std::vector<RFNoC_ResourceList *>::iterator it = updatedResourceLists.begin(); it != updatedResourceLists.end();) {
        bool foundConnection = false;
        RFNoC_ResourceList *updatedResourceList = *it;

        LOG_DEBUG(RFNoC_ResourceManager, "Got updatedResourceList, checking if it's in the map: " << updatedResourceList->getID());

        std::map<std::string, RFNoC_ResourceList *>::iterator remappedIt = remappedLists.find(updatedResourceList->getID());

        if (remappedIt != remappedLists.end()) {
            LOG_DEBUG(RFNoC_ResourceManager, "Found a list to connect to that was remapped: " << remappedLists[updatedResourceList->getID()]->getID());
            updatedResourceList = remappedLists[updatedResourceList->getID()];
        } else {
            LOG_DEBUG(RFNoC_ResourceManager, "Found a list to connect to that wasn't remapped");
        }

        for (RFNoC_ListMap::iterator it2 = this->idToList.begin(); it2 != this->idToList.end(); ++it2) {
            LOG_DEBUG(RFNoC_ResourceManager, "Checking connection: " << updatedResourceList->getID() << " -> " << it2->second->getID());

            if (updatedResourceList->getID() == it2->second->getID()) {
                LOG_DEBUG(RFNoC_ResourceManager, "Skipping check for connect for this resource list. Feedback not currently available.");
                continue;
            }

            if (updatedResourceList->connect(*it2->second)) {
                LOG_DEBUG(RFNoC_ResourceManager, "Connected, merging: " << it2->second->getID());

                // Merge the lists
                updatedResourceList->merge(it2->second);

                std::vector<std::string> resourceIDs = it2->second->getIDs();

                for (std::vector<std::string>::iterator idIt = resourceIDs.begin(); idIt != resourceIDs.end(); ++idIt) {
                    this->resourceIdToList[*idIt] = updatedResourceList;
                }

                remappedLists[it2->second->getID()] = updatedResourceList;

                foundConnection = true;
                break;
            }
        }

        if (foundConnection) {
            it = updatedResourceLists.erase(it);
        } else {
            ++it;
        }
    }

    // Anything left should be connected to the radio or set as streamers
    for (std::vector<RFNoC_ResourceList *>::iterator it = updatedResourceLists.begin(); it != updatedResourceLists.end(); ++it) {
        // TODO: Support updating the vector of RFNoC_Resources for multiple changes at once
        RFNoC_ResourceList *updatedResourceList = *it;
        RFNoC_Resource *providesResource = updatedResourceList->getProvidesResource();
        RFNoC_Resource *usesResource = updatedResourceList->getUsesResource();

        LOG_DEBUG(RFNoC_ResourceManager, "Got the resources");

        // Try to connect to programmable device first
        // RX Radio
        std::vector<CORBA::ULong> providesHashes = providesResource->getProvidesHashes();
        bool firstConnected = false;

        LOG_DEBUG(RFNoC_ResourceManager, "Got the provides hashes");

        for (size_t j = 0; j < providesHashes.size(); ++j) {
            CORBA::ULong hash = providesHashes[j];

            LOG_DEBUG(RFNoC_ResourceManager, "Checking provides hash: " << hash);

            BlockInfo providesBlock = providesResource->getProvidesBlock();

            if (this->connectRadioRXCb(hash, providesBlock.blockID, providesBlock.port)) {
                LOG_DEBUG(RFNoC_ResourceManager, "Connection succeeded");
                firstConnected = true;
                break;
            }
        }

        if (not firstConnected) {
            LOG_DEBUG(RFNoC_ResourceManager, "No connection, setting as TX streamer");
            providesResource->setTxStreamer(true);
        }

        // TX Radio
        std::vector<std::string> connectionIDs = usesResource->getConnectionIDs();
        bool lastConnected = false;

        LOG_DEBUG(RFNoC_ResourceManager, "Got the connection IDs");

        for (size_t j = 0; j < connectionIDs.size(); ++j) {
            std::string connectionID = connectionIDs[j];

            LOG_DEBUG(RFNoC_ResourceManager, "Checking connection ID: " << connectionID);

            BlockInfo usesBlock = usesResource->getUsesBlock();

            if (this->connectRadioTXCb(connectionID, usesBlock.blockID, usesBlock.port)) {
                LOG_DEBUG(RFNoC_ResourceManager, "Connection succeeded");
                lastConnected = true;
                break;
            }
        }

        if (not lastConnected) {
            LOG_DEBUG(RFNoC_ResourceManager, "No connection, setting as RX streamer");
            usesResource->setRxStreamer(true);
        }
    }

    // Remove the remapped lists
    for (std::map<std::string, RFNoC_ResourceList *>::iterator it = remappedLists.begin(); it != remappedLists.end(); ++it) {
        this->idToList.erase(it->second->getID());
        delete it->second;
    }

    return updatedAny;
}*/

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
