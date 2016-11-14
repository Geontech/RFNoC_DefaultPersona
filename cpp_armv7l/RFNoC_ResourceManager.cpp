/*
 * RFNoC_ResourceManager.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#include "RFNoC_ResourceManager.h"

PREPARE_LOGGING(RFNoC_ResourceManager)

RFNoC_ResourceManager::RFNoC_ResourceManager(uhd::device3::sptr usrp) :
    usrp(usrp)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    this->graph = this->usrp->create_graph("RFNoC_ResourceManager_" + ossie::generateUUID());
}

RFNoC_ResourceManager::~RFNoC_ResourceManager()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);
}

std::string RFNoC_ResourceManager::addResource(Resource_impl *rhResource)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    std::string resourceID = rhResource->_identifier;

    RFNoC_ListMap::iterator listMapIt = this->idToList.find(resourceID);

    if (listMapIt != this->idToList.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to add a Resource already tracked by the Resource Manager.");
        resourceID.clear();
        return resourceID;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Instantiating new RFNoC_ResourceList");

    RFNoC_ResourceList *newResourceList = new RFNoC_ResourceList(this->graph);

    LOG_DEBUG(RFNoC_ResourceManager, "Adding RFNoC_Resource to RFNoC_ResourceList");

    newResourceList->addResource(rhResource);

    LOG_DEBUG(RFNoC_ResourceManager, "Mapping Resource ID to RFNoC_ResourceList");

    this->idToList[resourceID] = newResourceList;

    return resourceID;
}

void RFNoC_ResourceManager::removeResource(const std::string &resourceID)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    LOG_DEBUG(RFNoC_ResourceManager, "Removing Resource with ID: " << resourceID);

    if (this->idToList.find(resourceID) == this->idToList.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to remove a Resource not tracked by the Resource Manager");
        return;
    }

    RFNoC_ResourceList *resourceList = this->idToList[resourceID];

    LOG_DEBUG(RFNoC_ResourceManager, "Removing the Resource from the RFNoC_ResourceList");

    resourceList->removeResource(resourceID);

    if (resourceList->empty()) {
        LOG_DEBUG(RFNoC_ResourceManager, "Deleting empty RFNoC_ResourceList from RFNoC_ResourceManager");
        delete resourceList;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Unmapping Resource from RFNoC_ResourceManager");

    this->idToList.erase(resourceID);
}

bool RFNoC_ResourceManager::update()
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    bool updatedAny = false;
    std::vector<RFNoC_ResourceList *> updatedResourcesLists;

    for (RFNoC_ListMap::iterator it = this->idToList.begin(); it != this->idToList.end(); ++it) {
        bool updated = it->second->update();

        if (updated) {
            updatedResourcesLists.push_back(it->second);
        }

        updatedAny |= updated;
    }

    std::map<RFNoC_ResourceList *, RFNoC_ResourceList *> remappedList;

    for (std::vector<RFNoC_ResourceList *>::iterator it = updatedResourcesLists.begin(); it != updatedResourcesLists.end(); ++it) {
        bool foundConnection = false;
        RFNoC_ResourceList *updatedResourceList = *it;

        std::map<RFNoC_ResourceList *, RFNoC_ResourceList *>::iterator remappedIt = remappedList.find(updatedResourceList);

        if (remappedIt != remappedList.end()) {
            LOG_DEBUG(RFNoC_ResourceManager, "Found a list to connect to that was remapped");
            updatedResourceList = remappedList[updatedResourceList];
        }

        for (RFNoC_ListMap::iterator it2 = this->idToList.begin(); it2 != this->idToList.end(); ++it2) {
            if (updatedResourceList->connect(*it2->second)) {
                // Merge the lists
                updatedResourceList->merge(it2->second);

                std::vector<std::string> resourceIDs = it2->second->getIDs();

                for (std::vector<std::string>::iterator idIt = resourceIDs.begin(); idIt != resourceIDs.end(); ++idIt) {
                    this->idToList[*idIt] = updatedResourceList;
                }

                remappedList[it2->second] = updatedResourceList;

                foundConnection = true;
                break;
            }
        }

        if (foundConnection) {
            updatedResourcesLists.erase(it);
        }
    }

    return updatedAny;
}

void RFNoC_ResourceManager::setBlockIDMapping(const std::string &resourceID, const std::vector<uhd::rfnoc::block_id_t> &blockIDs)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ListMap::iterator it = this->idToList.find(resourceID);

    if (it == this->idToList.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set Block ID for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting block IDs for Resource");

    it->second->setBlockIDMapping(resourceID, blockIDs);
}

void RFNoC_ResourceManager::setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ListMap::iterator it = this->idToList.find(resourceID);

    if (it == this->idToList.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set setRxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting setSetRxStreamer callback for Resource");

    it->second->setSetRxStreamer(resourceID, cb);
}

void RFNoC_ResourceManager::setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceManager, __PRETTY_FUNCTION__);

    RFNoC_ListMap::iterator it = this->idToList.find(resourceID);

    if (it == this->idToList.end()) {
        LOG_WARN(RFNoC_ResourceManager, "Attempted to set setTxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceManager, "Setting setSetTxStreamer callback for Resource");

    it->second->setSetTxStreamer(resourceID, cb);
}
