/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "RFNoC_DefaultPersona.h"

#include <frontend/frontend.h>

PREPARE_LOGGING(RFNoC_DefaultPersona_i)

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::~RFNoC_DefaultPersona_i()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->resourceManager) {
        delete this->resourceManager;
    }
}

void RFNoC_DefaultPersona_i::construct()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->hw_load_status.hardware_id = "E310";
    this->hw_load_status.load_filepath = "/usr/share/uhd/images/usrp_e310_fpga.bit";
    this->hw_load_status.request_id = "";
    this->hw_load_status.requester_id = "";
    this->hw_load_status.state = 0;

    LOG_INFO(RFNoC_DefaultPersona_i, "Hardware ID: " << hw_load_status.hardware_id);

    this->setThreadDelay(1.0);

    this->start();
}

int RFNoC_DefaultPersona_i::serviceFunction()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (not this->usrp) {
        return NOOP;
    }

    boost::mutex::scoped_lock lock(this->resourceLock);

    while (this->terminateWaiting or this->resourceHeld) {
        this->resourceAvailable.wait(lock);
    }

    this->resourceHeld = true;

    lock.unlock();

    this->resourceManager->update();

    // Iterate over the resources to determine connections
    /*for (std::map<std::string, ResourceInfo *>::iterator it = this->IDToResourceInfo.begin(); it != this->IDToResourceInfo.end(); it++) {
        ResourceInfo *resourceInfo = it->second;
        Resource_impl *resource = resourceInfo->resource;

        LOG_DEBUG(RFNoC_DefaultPersona_i, "Checking " << resource->_identifier << " for connections");

        // Iterate over the uses ports
        for (size_t i = 0; i < resourceInfo->usesPorts.size(); ++i) {
            BULKIO::UsesPortStatisticsProvider_ptr port = resourceInfo->usesPorts[i];

            if (port->_non_existent()) {
                LOG_DEBUG(RFNoC_DefaultPersona_i, "The port no longer exists");
                continue;
            }

            ExtendedCF::UsesConnectionSequence *connections = port->connections();
            bool foundConnection = false;

            // This resource has connections, though not necessarily to another of this Device's resources
            if (connections->length() != 0) {
                // Iterate over the connections
                for (size_t i = 0; i < connections->length(); ++i) {
                    ExtendedCF::UsesConnection connection = port->connections()->operator [](i);
                    CORBA::ULong providesPortHash = connection.port->_hash(1024);
                    std::map<CORBA::ULong, ResourceInfo *>::iterator it2 = this->hashToResourceInfo.find(providesPortHash);

                    LOG_DEBUG(RFNoC_DefaultPersona_i, "Checking port hash " << providesPortHash << " for connections");

                    // That port maps to one of this Device's resources
                    if (it2 != this->hashToResourceInfo.end()) {
                        ResourceInfo *providesResourceInfo = it2->second;
                        Resource_impl *providesResource = providesResourceInfo->resource;
                        std::string providesResourceID = providesResource->_identifier;

                        LOG_DEBUG(RFNoC_DefaultPersona_i, "Found " << providesResource->_identifier << ", checking if already connected");

                        std::pair<CORBA::ULong, std::string> hashID = std::make_pair(providesPortHash, providesResourceID);
                        std::map<std::pair<CORBA::ULong, std::string>, bool>::iterator connected = this->areConnected.find(hashID);

                        // They aren't connected yet
                        if (connected == this->areConnected.end()) {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, resource->_identifier << " was not connected to " << providesResource->_identifier);

                            // Connect things
                            if (resourceInfo->blockIDs.empty() or providesResourceInfo->blockIDs.empty()) {
                                LOG_DEBUG(RFNoC_DefaultPersona_i, "RF-NoC Block IDs aren't set yet, cannot connect");
                                continue;
                            }

                            uhd::rfnoc::block_id_t providesBlockID = providesResourceInfo->blockIDs.front();
                            uhd::rfnoc::block_id_t usesBlockID = resourceInfo->blockIDs.back();
                            bool foundProvides = (this->blockToGraph.find(providesBlockID) != this->blockToGraph.end());
                            bool foundUses = (this->blockToGraph.find(usesBlockID) != this->blockToGraph.end());

                            // Create a new graph
                            if (not foundProvides and not foundUses) {
                                LOG_DEBUG(RFNoC_DefaultPersona_i, "Creating a new graph");

                                uhd::rfnoc::graph::sptr graph = this->usrp->create_graph(frontend::uuidGenerator());

                                graph->connect(usesBlockID, providesBlockID);

                                this->blockToGraph[usesBlockID] = graph;
                                this->blockToGraph[providesBlockID] = graph;

                                std::list<std::string> *blockList = new std::list<std::string>;

                                blockList->push_back(usesBlockID);
                                blockList->push_back(providesBlockID);

                                this->blockToList[usesBlockID] = blockList;
                                this->blockToList[providesBlockID] = blockList;
                                this->graphToList[graph->get_name()] = blockList;
                                this->graphUpdated[graph->get_name()] = true;
                            }
                            // Merge the two lists
                            else if (foundProvides and foundUses) {
                                LOG_DEBUG(RFNoC_DefaultPersona_i, "Connecting two graphs?");

                                uhd::rfnoc::graph::sptr usesGraph = this->blockToGraph[usesBlockID];
                                uhd::rfnoc::graph::sptr providesGraph = this->blockToGraph[providesBlockID];

                                usesGraph->connect(usesBlockID, providesBlockID);

                                std::list<std::string> *usesBlockList = this->blockToList[usesBlockID];
                                std::list<std::string> *providesBlockList = this->blockToList[providesBlockID];

                                usesBlockList->insert(usesBlockList->end(), providesBlockList->begin(), providesBlockList->end());

                                for (std::list<std::string>::iterator listIt = providesBlockList->begin(); listIt != providesBlockList->end(); ++listIt) {
                                    this->blockToList[*listIt] = usesBlockList;
                                }

                                delete providesBlockList;

                                this->graphToList[providesGraph->get_name()] = usesBlockList;
                                this->graphUpdated[usesGraph->get_name()] = true;
                                this->graphUpdated[providesGraph->get_name()] = false;
                            }
                            // Add the provides block to the uses graph
                            else if (foundUses) {
                                LOG_DEBUG(RFNoC_DefaultPersona_i, "Using the uses graph");

                                uhd::rfnoc::graph::sptr graph = this->blockToGraph[usesBlockID];

                                graph->connect(usesBlockID, providesBlockID);

                                this->blockToGraph[providesBlockID] = graph;

                                std::list<std::string> *blockList = this->blockToList[usesBlockID];

                                std::list<std::string>::iterator blockLoc = std::find(blockList->begin(), blockList->end(), usesBlockID);

                                if (blockLoc != blockList->end()) {
                                    blockLoc++;
                                    blockList->insert(blockLoc, providesBlockID);
                                } else {
                                    blockList->push_back(providesBlockID);
                                }

                                this->blockToList[providesBlockID] = blockList;
                                this->graphUpdated[graph->get_name()] = true;
                            }
                            // Add the uses block to the provides graph
                            else if (foundProvides) {
                                LOG_DEBUG(RFNoC_DefaultPersona_i, "Using the provides graph");

                                uhd::rfnoc::graph::sptr graph = this->blockToGraph[providesBlockID];

                                graph->connect(usesBlockID, providesBlockID);

                                this->blockToGraph[usesBlockID] = graph;

                                std::list<std::string> *blockList = this->blockToList[providesBlockID];

                                std::list<std::string>::iterator blockLoc = std::find(blockList->begin(), blockList->end(), providesBlockID);

                                blockList->insert(blockLoc, usesBlockID);

                                this->blockToList[providesBlockID] = blockList;
                                this->graphUpdated[graph->get_name()] = true;
                            }

                            this->areConnected[hashID] = true;
                            foundConnection = true;
                        }
                    }
                }
            }

            // This resource has no connections or is not connected to another of this Device's resources
            if (connections->length() == 0 or not foundConnection) {
                LOG_DEBUG(RFNoC_DefaultPersona_i, resource->_identifier << " has no connections to another of this Device's resources");

                if (resourceInfo->blockIDs.empty()) {
                    LOG_DEBUG(RFNoC_DefaultPersona_i, "RF-NoC Block ID(s) not set yet, cannot create graph");
                }

                uhd::rfnoc::block_id_t usesBlockID = resourceInfo->blockIDs.back();

                // Connect things
                if (usesBlockID.to_string().empty()) {
                    LOG_DEBUG(RFNoC_DefaultPersona_i, "Last RF-NoC Block ID is not set yet, cannot create graph");
                    continue;
                }

                bool foundGraph = (this->blockToGraph.find(usesBlockID) != this->blockToGraph.end());

                if (not foundGraph) {
                    LOG_DEBUG(RFNoC_DefaultPersona_i, "Creating a new graph");

                    uhd::rfnoc::graph::sptr graph = this->usrp->create_graph(frontend::uuidGenerator());

                    this->blockToGraph[usesBlockID] = graph;

                    std::list<std::string> *blockList = new std::list<std::string>;

                    blockList->push_back(usesBlockID);

                    this->blockToList[usesBlockID] = blockList;
                    this->graphToList[graph->get_name()] = blockList;
                    this->graphUpdated[graph->get_name()] = true;
                }
            }
        }
    }

    // Iterate over the lists
    LOG_DEBUG(RFNoC_DefaultPersona_i, "Iterating over graphs and lists");

    for (std::map<std::string, std::list<std::string> *>::iterator it = this->graphToList.begin(); it != this->graphToList.end(); ++it) {
        if (this->graphUpdated[it->first]) {
            LOG_DEBUG(RFNoC_DefaultPersona_i, "Graph: " << it->first);

            std::stringstream ss;

            for(std::list<std::string>::iterator it2 = it->second->begin(); it2 != it->second->end(); ++it2) {
                if (it2 != it->second->begin()) {
                    ss << " -> ";
                }

                ss << *it2;

                // Update the resource
                this->blockToResourceInfo[*it2]->setRxStreamerCb(false);
                this->blockToResourceInfo[*it2]->setTxStreamerCb(false);
            }

            // Try to connect to programmable device first
            // RX Radio
            uhd::rfnoc::block_id_t firstBlockID(it->second->front());
            ResourceInfo *firstResourceInfo = this->blockToResourceInfo[firstBlockID.to_string()];
            bool firstConnected = false;

            for (size_t i = 0; i < firstResourceInfo->providesPortHashes.size(); ++i) {
                CORBA::ULong hash = firstResourceInfo->providesPortHashes[i];

                if (this->connectRadioRXCb(hash, firstBlockID, uhd::rfnoc::ANY_PORT)) {
                    firstConnected = true;
                    break;
                }
            }

            if (not firstConnected) {
                this->blockToResourceInfo[it->second->front()]->setTxStreamerCb(true);
            }

            // TX Radio
            uhd::rfnoc::block_id_t lastBlockID(it->second->back());
            ResourceInfo *lastResourceInfo = this->blockToResourceInfo[lastBlockID.to_string()];
            bool lastConnected = false;

            for (size_t i = 0; i < lastResourceInfo->usesPorts.size(); ++i) {
                ExtendedCF::UsesConnectionSequence *connections = lastResourceInfo->usesPorts[i]->connections();

                for (size_t j = 0; j < connections->length(); ++j) {
                    ExtendedCF::UsesConnection connection = connections->operator [](j);

                    if (this->connectRadioTXCb(connection.connectionId._ptr, lastBlockID, uhd::rfnoc::ANY_PORT)) {
                        lastConnected = true;
                        break;
                    }
                }
            }

            if (not lastConnected) {
                this->blockToResourceInfo[it->second->back()]->setRxStreamerCb(true);
            }

            LOG_DEBUG(RFNoC_DefaultPersona_i, ss.str());
            LOG_DEBUG(RFNoC_DefaultPersona_i, "");

            this->graphUpdated[it->first] = false;
        }
    }*/

    lock.lock();

    this->resourceHeld = false;

    this->resourceAvailable.notify_all();

    lock.unlock();

    return NOOP;
}

CORBA::Boolean RFNoC_DefaultPersona_i::allocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException) 
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    bool allocationSuccess = false;

    if (isBusy() || isLocked()) {
        LOG_WARN(RFNoC_DefaultPersona_i, __FUNCTION__ << 
            ": Cannot allocate capacities... Device state is locked and/or busy");
        return false;
    }

    allocationSuccess = true;

    if (allocationSuccess) {
        allocationSuccess = attemptToProgramParent();

        try {
            LOG_DEBUG(RFNoC_DefaultPersona_i, "Successfully programmed parent, attempting to retrieve USRP pointer");
            this->usrp = uhd::device3::make(this->usrpAddress);
            this->enabled = true;
        } catch(...) {
            LOG_ERROR(RFNoC_DefaultPersona_i, "Failed to retrieve USRP pointer");
            allocationSuccess = false;
            attemptToUnprogramParent();
        }

        if (allocationSuccess) {
            LOG_DEBUG(RFNoC_DefaultPersona_i, "Instantiating RF-NoC Resource Manager");
            this->resourceManager = new RFNoC_ResourceManager(this->usrp);
        }
    }

    return allocationSuccess;
}

void RFNoC_DefaultPersona_i::deallocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException) 
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->resourceManager) {
        delete this->resourceManager;
        this->resourceManager = NULL;
    }

    this->enabled = false;
    this->usrp.reset();

    attemptToUnprogramParent();
}

CF::ExecutableDevice::ProcessID_Type RFNoC_DefaultPersona_i::execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
    throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
            CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
            CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (not this->enabled) {
        LOG_ERROR(RFNoC_DefaultPersona_i, "This persona has not been loaded yet");
        throw CF::ExecutableDevice::ExecuteFail();
    }

    // Call the parent execute
    CF::ExecutableDevice::ProcessID_Type pid = RFNoC_DefaultPersona_persona_base::execute(name, options, parameters);

    // Map the PID to the component identifier
    std::string componentIdentifier;

    for (size_t i = 0; i < parameters.length(); ++i) {
        std::string id = parameters[i].id._ptr;

        if (id == "COMPONENT_IDENTIFIER") {
            componentIdentifier += ossie::any_to_string(parameters[i].value);
            break;
        }
    }

    this->pidToComponentID[pid] = componentIdentifier;

    return pid;
}

void RFNoC_DefaultPersona_i::terminate (CF::ExecutableDevice::ProcessID_Type processId)
                    throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Lock to prevent the service function from using this resource
    boost::mutex::scoped_lock lock(this->resourceLock);

    this->terminateWaiting = true;

    while (this->resourceHeld) {
        this->resourceAvailable.wait(lock);
    }

    this->resourceHeld = true;

    if (this->pidToComponentID.find(processId) == this->pidToComponentID.end()) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to terminate a process with an ID not tracked by this Device");
        this->resourceHeld = false;
        this->terminateWaiting = false;
        this->resourceAvailable.notify_all();
        lock.unlock();
        throw CF::ExecutableDevice::InvalidProcess();
    }

    // Get the component identifier associated with this PID
    std::string componentID = this->pidToComponentID[processId];

    this->resourceManager->removeResource(componentID);

    LOG_DEBUG(RFNoC_DefaultPersona_i, "Terminating component: " << componentID);

    // Unmap the PID from the component identifier
    this->pidToComponentID.erase(processId);

    this->resourceHeld = false;
    this->terminateWaiting = false;
    this->resourceAvailable.notify_all();
    lock.unlock();

    // Call the parent terminate
    RFNoC_DefaultPersona_persona_base::terminate(processId);
}

void RFNoC_DefaultPersona_i::setConnectRadioRXCallback(connectRadioRXCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->connectRadioRXCb = cb;
}

void RFNoC_DefaultPersona_i::setConnectRadioTXCallback(connectRadioTXCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->connectRadioTXCb = cb;
}

void RFNoC_DefaultPersona_i::setHwLoadStatusCallback(hwLoadStatusCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    hw_load_status_object hwLoadStatusObject;

    hwLoadStatusObject.hardware_id = this->hw_load_status.hardware_id;
    hwLoadStatusObject.load_filepath = this->hw_load_status.load_filepath;
    hwLoadStatusObject.request_id = this->hw_load_status.request_id;
    hwLoadStatusObject.requester_id = this->hw_load_status.requester_id;
    hwLoadStatusObject.state = this->hw_load_status.state;

    cb(hwLoadStatusObject);
}

void RFNoC_DefaultPersona_i::setUsrpAddress(uhd::device_addr_t usrpAddress)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->usrpAddress = usrpAddress;
}

Resource_impl* RFNoC_DefaultPersona_i::generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Lock to prevent the service function from using this resource
    boost::mutex::scoped_lock lock(this->resourceLock);

    // Construct the resource
    Resource_impl *resource;

    try {
        blockIDCallback blockIdCb = boost::bind(&RFNoC_ResourceManager::setBlockIDMapping, this->resourceManager, _1, _2);
        setSetStreamerCallback setSetRxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetRxStreamer, this->resourceManager, _1, _2);
        setSetStreamerCallback setSetTxStreamerCb = boost::bind(&RFNoC_ResourceManager::setSetTxStreamer, this->resourceManager, _1, _2);

        resource = fnptr(argc, argv, this, this->usrpAddress, blockIdCb, setSetRxStreamerCb, setSetTxStreamerCb);

        if (not resource) {
            LOG_ERROR(RFNoC_DefaultPersona_i, "Constructor returned NULL resource");
            throw std::exception();
        }
    } catch (...) {
        LOG_ERROR(RFNoC_DefaultPersona_i, "Constructor threw an exception");

        return NULL;
    }

    this->resourceManager->addResource(resource);

    LOG_INFO(RFNoC_DefaultPersona_i, "Casting the resource as an RF-Noc Component Interface");

    // Activate the component
    RFNoC_ComponentInterface *component = (RFNoC_ComponentInterface *) resource;

    LOG_INFO(RFNoC_DefaultPersona_i, "Activating the component");

    component->activate();

    LOG_INFO(RFNoC_DefaultPersona_i, "Component activated");

    return resource;
}

void RFNoC_DefaultPersona_i::hwLoadRequest(CF::Properties& request)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    request.length(4);

    request[0].id = CORBA::string_dup("request_id");
    request[0].value <<= ossie::generateUUID();
    request[1].id = CORBA::string_dup("requester_id");
    request[1].value <<= ossie::corba::returnString(this->_identifier.c_str());
    request[2].id = CORBA::string_dup("hardware_id");
    request[2].value <<= ossie::corba::returnString(this->hw_load_status.hardware_id.c_str());
    LOG_INFO(RFNoC_DefaultPersona_i, "Hardware ID: " << this->hw_load_status.hardware_id);
    request[3].id = CORBA::string_dup("load_filepath");
    request[3].value <<= ossie::corba::returnString(this->hw_load_status.load_filepath.c_str());
}
