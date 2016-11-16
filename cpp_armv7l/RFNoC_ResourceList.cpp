/*
 * RFNoC_ResourceList.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: Patrick
 */

#include "RFNoC_ResourceList.h"

PREPARE_LOGGING(RFNoC_ResourceList)

RFNoC_ResourceList::RFNoC_ResourceList(RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph) :
    graph(graph),
    resourceManager(resourceManager)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);
}

RFNoC_ResourceList::~RFNoC_ResourceList()
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    for (RFNoC_ResourceMap::iterator it = this->idToResource.begin(); it != this->idToResource.end(); ++it) {
        delete it->second;
    }
}

Resource_impl* RFNoC_ResourceList::addResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName, std::string resourceID)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    LOG_DEBUG(RFNoC_ResourceList, "Adding Resource with ID: " << resourceID);

    RFNoC_ResourceMap::iterator resourceMapIt = this->idToResource.find(resourceID);

    if (resourceMapIt != this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceList, "Attempted to add a Resource already tracked by the RFNoC_ResourceList.");
        return NULL;
    }

    LOG_DEBUG(RFNoC_ResourceList, "Instantiating new RFNoC_Resource");

    RFNoC_Resource *newResource = new RFNoC_Resource(resourceID, this->resourceManager, this->graph);

    LOG_DEBUG(RFNoC_ResourceList, "Mapping Resource ID to RFNoC_Resource");

    this->idToResource[resourceID] = newResource;

    LOG_DEBUG(RFNoC_ResourceList, "Instantiating Resource");

    Resource_impl* resource = newResource->instantiate(argc, argv, fnptr, libraryName);

    if (not resource) {
        LOG_ERROR(RFNoC_ResourceList, "Failed to add new resource, cleaning up");
        delete newResource;
        this->idToResource.erase(resourceID);
    }

    // The first resource always goes in the list
    if (this->resourceList.empty()) {
        this->resourceList.push_back(newResource);
    }

    return resource;
}

bool RFNoC_ResourceList::connect(RFNoC_ResourceList &providesList)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    RFNoC_Resource *providesResource = providesList.getProvidesResource();
    RFNoC_Resource *usesResource = this->getUsesResource();

    return usesResource->connect(*providesResource);
}

bool RFNoC_ResourceList::empty()
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    return resourceList.empty();
}

std::vector<std::string> RFNoC_ResourceList::getIDs()
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    std::vector<std::string> resourceIDs;

    for (RFNoC_ResourceMap::iterator it = this->idToResource.begin(); it != this->idToResource.end(); ++it) {
        resourceIDs.push_back(it->first);
    }

    return resourceIDs;
}

RFNoC_Resource* RFNoC_ResourceList::getProvidesResource() const
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    return this->resourceList.front();
}

RFNoC_Resource* RFNoC_ResourceList::getUsesResource() const
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    return this->resourceList.back();
}

bool RFNoC_ResourceList::hasResource(const RFNoC_Resource *resource)
{
    return (this->idToResource.find(resource->id()) != this->idToResource.end());
}

void RFNoC_ResourceList::merge(RFNoC_ResourceList *providesList)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    this->resourceList.insert(this->resourceList.end(), providesList->resourceList.begin(), providesList->resourceList.end());

    providesList->resourceList.clear();

    for (RFNoC_ResourceMap::iterator it = providesList->idToResource.begin(); it != providesList->idToResource.end(); ++it) {
        this->idToResource[it->first] = it->second;
    }

    providesList->idToResource.clear();
}

void RFNoC_ResourceList::removeResource(const std::string &resourceID)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    LOG_DEBUG(RFNoC_ResourceList, "Removing Resource with ID: " << resourceID);

    if (this->idToResource.find(resourceID) == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceList, "Attempted to remove a Resource not tracked by the RFNoC_ResourceList");
        return;
    }

    RFNoC_Resource *resource = this->idToResource[resourceID];

    std::list<RFNoC_Resource *>::iterator listIt = std::find(this->resourceList.begin(), this->resourceList.end(), resource);

    if (listIt == this->resourceList.end()) {
        LOG_INFO(RFNoC_ResourceList, "Removing Resource which was never connected");
    } else {
        LOG_DEBUG(RFNoC_ResourceList, "Removing Resource from list");
        this->resourceList.erase(listIt);
    }

    LOG_DEBUG(RFNoC_ResourceList, "Deleting the RFNoC_Resource");

    delete resource;

    LOG_DEBUG(RFNoC_ResourceList, "Unmapping the RFNoC_Resource");

    this->idToResource.erase(resourceID);
}

bool RFNoC_ResourceList::update()
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    std::vector<RFNoC_Resource *> updatedResources;

    // Check if any of the resources in this list changed
    for (RFNoC_ResourceMap::iterator it = this->idToResource.begin(); it != this->idToResource.end(); ++it) {
        if (it->second->update()) {
            updatedResources.push_back(it->second);
        }
    }

    for (std::vector<RFNoC_Resource *>::iterator it = updatedResources.begin(); it != updatedResources.end(); ++it) {
        RFNoC_Resource *updatedResource = *it;

        bool foundConnection = false;

        for (RFNoC_ResourceMap::iterator it2 = this->idToResource.begin(); it2 != this->idToResource.end(); ++it2) {
            if (updatedResource->id() == it2->second->id()) {
                LOG_DEBUG(RFNoC_ResourceList, "Skipping check for connect for this resource. Feedback not currently available.");
                continue;
            }

            if (updatedResource->connect(*it2->second)) {
                std::list<RFNoC_Resource *>::iterator providesListIt, usesListIt;

                providesListIt = std::find(this->resourceList.begin(), this->resourceList.end(), it2->second);
                usesListIt = std::find(this->resourceList.begin(), this->resourceList.end(), updatedResource);

                bool foundProvides, foundUses;

                foundProvides = (providesListIt != this->resourceList.end());
                foundUses = (usesListIt != this->resourceList.end());

                uhd::rfnoc::block_id_t providesBlockID, usesBlockID;

                providesBlockID = it2->second->getProvidesBlock();
                usesBlockID = updatedResource->getUsesBlock();

                LOG_DEBUG(RFNoC_ResourceList, "Connecting RFNoC_Resources: " << usesBlockID.to_string() << " -> " << providesBlockID.to_string());

                if (not foundProvides and not foundUses) {
                    LOG_DEBUG(RFNoC_ResourceList, "Inserting both RFNoC_Resources into the list");

                    this->resourceList.push_back(updatedResource);
                    this->resourceList.push_back(it2->second);
                } else if (foundProvides and foundUses) {
                    // TODO: Make this less interesting
                    LOG_DEBUG(RFNoC_ResourceList, "Interesting case for later...");
                } else if (foundProvides) {
                    LOG_DEBUG(RFNoC_ResourceList, "Inserting uses RFNoC_Resource into the list");

                    this->resourceList.insert(providesListIt, updatedResource);
                } else if (foundUses) {
                    LOG_DEBUG(RFNoC_ResourceList, "Inserting provides RFNoC_Resource into the list");

                    if (usesListIt != this->resourceList.end()) {
                        usesListIt++;
                        this->resourceList.insert(usesListIt, it2->second);
                    } else {
                        this->resourceList.push_back(it2->second);
                    }
                }

                foundConnection = true;

                break;
            }
        }

        if (foundConnection) {
            updatedResources.erase(it);
        }
    }

    return (updatedResources.size() != 0);
}

void RFNoC_ResourceList::setBlockIDMapping(const std::string &resourceID, const std::vector<uhd::rfnoc::block_id_t> &blockIDs)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceList, "Attempted to set Block ID for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceList, "Setting block IDs for Resource");

    it->second->setBlockIDs(blockIDs);
}

void RFNoC_ResourceList::setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceList, "Attempted to set setRxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceList, "Setting setRxStreamer callback for Resource");

    it->second->setSetRxStreamer(cb);
}

void RFNoC_ResourceList::setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_ResourceList, __PRETTY_FUNCTION__);

    RFNoC_ResourceMap::iterator it = this->idToResource.find(resourceID);

    if (it == this->idToResource.end()) {
        LOG_WARN(RFNoC_ResourceList, "Attempted to set setTxStreamer for unknown Resource: " << resourceID);
        return;
    }

    LOG_DEBUG(RFNoC_ResourceList, "Setting setTxStreamer callback for Resource");

    it->second->setSetTxStreamer(cb);
}

