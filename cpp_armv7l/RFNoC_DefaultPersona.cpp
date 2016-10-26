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
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::~RFNoC_DefaultPersona_i()
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

void RFNoC_DefaultPersona_i::construct()
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->hw_load_status.hardware_id = "E310";
    this->hw_load_status.load_filepath = "/usr/share/uhd/images/usrp_e310_fpga.bit";
    this->hw_load_status.request_id = "";
    this->hw_load_status.requester_id = "";
    this->hw_load_status.state = 0;

    LOG_INFO(RFNoC_DefaultPersona_i, "Hardware ID: " << hw_load_status.hardware_id);

    this->start();
}

/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
                std::string stream_id = "testStream";
                BULKIO::StreamSRI sri = bulkio::sri::create(stream_id);

	Time:
	    To create a PrecisionUTCTime object, use the following code:
                BULKIO::PrecisionUTCTime tstamp = bulkio::time::utils::now();

        
    Ports:

        Data is passed to the serviceFunction through the getPacket call (BULKIO only).
        The dataTransfer class is a port-specific class, so each port implementing the
        BULKIO interface will have its own type-specific dataTransfer.

        The argument to the getPacket function is a floating point number that specifies
        the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.  Constants have been defined for these values, bulkio::Const::BLOCKING and
        bulkio::Const::NON_BLOCKING.

        Each received dataTransfer is owned by serviceFunction and *MUST* be
        explicitly deallocated.

        To send data using a BULKIO interface, a convenience interface has been added 
        that takes a std::vector as the data input

        NOTE: If you have a BULKIO dataSDDS or dataVITA49 port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // this example assumes that the device has two ports:
            //  A provides (input) port of type bulkio::InShortPort called short_in
            //  A uses (output) port of type bulkio::OutFloatPort called float_out
            // The mapping between the port and the class is found
            // in the device base class header file

            bulkio::InShortPort::dataTransfer *tmp = short_in->getPacket(bulkio::Const::BLOCKING);
            if (not tmp) { // No data is available
                return NOOP;
            }

            std::vector<float> outputData;
            outputData.resize(tmp->dataBuffer.size());
            for (unsigned int i=0; i<tmp->dataBuffer.size(); i++) {
                outputData[i] = (float)tmp->dataBuffer[i];
            }

            // NOTE: You must make at least one valid pushSRI call
            if (tmp->sriChanged) {
                float_out->pushSRI(tmp->SRI);
            }
            float_out->pushPacket(outputData, tmp->T, tmp->EOS, tmp->streamID);

            delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
            return NORMAL;

        If working with complex data (i.e., the "mode" on the SRI is set to
        true), the std::vector passed from/to BulkIO can be typecast to/from
        std::vector< std::complex<dataType> >.  For example, for short data:

            bulkio::InShortPort::dataTransfer *tmp = myInput->getPacket(bulkio::Const::BLOCKING);
            std::vector<std::complex<short> >* intermediate = (std::vector<std::complex<short> >*) &(tmp->dataBuffer);
            // do work here
            std::vector<short>* output = (std::vector<short>*) intermediate;
            myOutput->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);

        Interactions with non-BULKIO ports are left up to the device developer's discretion
        
    Messages:
    
        To receive a message, you need (1) an input port of type MessageEvent, (2) a message prototype described
        as a structure property of kind message, (3) a callback to service the message, and (4) to register the callback
        with the input port.
        
        Assuming a property of type message is declared called "my_msg", an input port called "msg_input" is declared of
        type MessageEvent, create the following code:
        
        void RFNoC_DefaultPersona_i::my_message_callback(const std::string& id, const my_msg_struct &msg){
        }
        
        Register the message callback onto the input port with the following form:
        this->msg_input->registerMessage("my_msg", this, &RFNoC_DefaultPersona_i::my_message_callback);
        
        To send a message, you need to (1) create a message structure, (2) a message prototype described
        as a structure property of kind message, and (3) send the message over the port.
        
        Assuming a property of type message is declared called "my_msg", an output port called "msg_output" is declared of
        type MessageEvent, create the following code:
        
        ::my_msg_struct msg_out;
        this->msg_output->sendMessage(msg_out);

    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given the property id as its name.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (RFNoC_DefaultPersona_persona_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
              
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        A callback method can be associated with a property so that the method is
        called each time the property value changes.  This is done by calling 
        setPropertyChangeListener(<property name>, this, &RFNoC_DefaultPersona_i::<callback method>)
        in the constructor.
            
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            
        //Add to RFNoC_DefaultPersona.cpp
        RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(const char *uuid, const char *label) :
            RFNoC_DefaultPersona_persona_base(uuid, label)
        {
            setPropertyChangeListener("scaleValue", this, &RFNoC_DefaultPersona_i::scaleChanged);
        }

        void RFNoC_DefaultPersona_i::scaleChanged(const std::string& id){
            std::cout << "scaleChanged scaleValue " << scaleValue << std::endl;
        }
            
        //Add to RFNoC_DefaultPersona.h
        void scaleChanged(const std::string&);
        
        
************************************************************************************************/
int RFNoC_DefaultPersona_i::serviceFunction()
{
    boost::mutex::scoped_lock lock(this->resourceLock);

    // Iterate over the resources to determine connections
    for (std::map<std::string, ResourceInfo *>::iterator it = this->IDToResourceInfo.begin(); it != this->IDToResourceInfo.end(); it++) {
        ResourceInfo *resourceInfo = it->second;
        Resource_impl *resource = resourceInfo->resource;

        LOG_DEBUG(RFNoC_DefaultPersona_i, "Checking " << resource->_identifier << " for connections");

        // Iterate over the uses ports
        for (size_t i = 0; i < resourceInfo->usesPorts.size(); ++i) {
            BULKIO::UsesPortStatisticsProvider_ptr port = resourceInfo->usesPorts[i];

            // Iterate over the connections
            for (size_t i = 0; i < port->connections()->length(); ++i) {
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
                        if (resourceInfo->blockID.empty() or providesResourceInfo->blockID.empty()) {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, "RF-NoC Block IDs aren't set yet, cannot connect");
                            continue;
                        }

                        bool foundProvides = (this->blockToGraph.find(providesResourceInfo->blockID) != this->blockToGraph.end());
                        bool foundUses = (this->blockToGraph.find(resourceInfo->blockID) != this->blockToGraph.end());

                        // Create a new graph
                        if (not foundProvides and not foundUses) {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, "Creating a new graph");

                            uhd::rfnoc::graph::sptr graph = this->usrp->create_graph(frontend::uuidGenerator());

                            graph->connect(resourceInfo->blockID, providesResourceInfo->blockID);

                            this->blockToGraph[resourceInfo->blockID] = graph;
                            this->blockToGraph[providesResourceInfo->blockID] = graph;

                            std::list<std::string> *blockList = new std::list<std::string>;

                            blockList->push_back(resourceInfo->blockID);
                            blockList->push_back(providesResourceInfo->blockID);

                            this->graphToList[graph->get_name()] = blockList;
                            this->graphUpdated[graph->get_name()] = true;
                        }
                        // Add the provides block to the uses graph
                        else if (foundUses) {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, "Using the uses graph");

                            uhd::rfnoc::graph::sptr graph = this->blockToGraph[resourceInfo->blockID];

                            graph->connect(resourceInfo->blockID, providesResourceInfo->blockID);

                            this->blockToGraph[providesResourceInfo->blockID] = graph;

                            std::list<std::string> *blockList = this->blockToList[resourceInfo->blockID];

                            std::list<std::string>::iterator blockLoc = std::find(blockList->begin(), blockList->end(), resourceInfo->blockID);

                            blockList->insert(++blockLoc, providesResourceInfo->blockID);

                            this->graphUpdated[graph->get_name()] = true;
                        }
                        // Add the uses block to the provides graph
                        else if (foundProvides) {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, "Using the provides graph");

                            uhd::rfnoc::graph::sptr graph = this->blockToGraph[providesResourceInfo->blockID];

                            graph->connect(resourceInfo->blockID, providesResourceInfo->blockID);

                            this->blockToGraph[resourceInfo->blockID] = graph;

                            std::list<std::string> *blockList = this->blockToList[providesResourceInfo->blockID];

                            std::list<std::string>::iterator blockLoc = std::find(blockList->begin(), blockList->end(), providesResourceInfo->blockID);

                            blockList->insert(blockLoc, resourceInfo->blockID);

                            this->graphUpdated[graph->get_name()] = true;
                        }
                        // Merge the two graphs?
                        else {
                            LOG_DEBUG(RFNoC_DefaultPersona_i, "Connecting two graphs?");

                            uhd::rfnoc::graph::sptr graph = this->blockToGraph[resourceInfo->blockID];

                            graph->connect(resourceInfo->blockID, providesResourceInfo->blockID);

                            std::list<std::string> *blockList = this->blockToList[providesResourceInfo->blockID];

                            std::list<std::string>::iterator blockLoc = std::find(blockList->begin(), blockList->end(), providesResourceInfo->blockID);

                            blockList->insert(blockLoc, resourceInfo->blockID);

                            this->graphUpdated[graph->get_name()] = true;
                        }

                        this->areConnected[hashID] = true;
                    }
                }
            }
        }
    }

    // Iterate over the lists
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

            this->blockToResourceInfo[it->second->front()]->setTxStreamerCb(true);
            this->blockToResourceInfo[it->second->back()]->setRxStreamerCb(true);

            LOG_DEBUG(RFNoC_DefaultPersona_i, ss.str());
            LOG_DEBUG(RFNoC_DefaultPersona_i, "");

            this->graphUpdated[it->first] = false;
        }
    }

    return NOOP;
}

CORBA::Boolean RFNoC_DefaultPersona_i::allocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException) 
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    bool allocationSuccess = false;

    if (isBusy() || isLocked()) {
        LOG_WARN(RFNoC_DefaultPersona_i, __FUNCTION__ << 
            ": Cannot allocate capacities... Device state is locked and/or busy");
        return false;
    }

    allocationSuccess = true;

    if (allocationSuccess) {
        allocationSuccess = attemptToProgramParent();
    }
    return allocationSuccess;
}

void RFNoC_DefaultPersona_i::deallocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException) 
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    /*
     * Do deallocation work here...
     */


    attemptToUnprogramParent();
}

CF::ExecutableDevice::ProcessID_Type RFNoC_DefaultPersona_i::execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
    throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
            CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
            CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

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

    this->pidToID[pid] = componentIdentifier;

    return pid;
}

void RFNoC_DefaultPersona_i::terminate (CF::ExecutableDevice::ProcessID_Type processId)
                    throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Get the component identifier associated with this PID, then erase the mapping
    std::string ID = this->pidToID[processId];
    this->pidToID.erase(processId);

    // Lock to prevent the service function from using this resource
    boost::mutex::scoped_lock lock(this->resourceLock);

    ResourceInfo *resourceInfo = this->IDToResourceInfo[ID];

    // Unmap the port hashes from this resource
    for (size_t i = 0; i < resourceInfo->usesPorts.size(); ++i) {
        BULKIO::UsesPortStatisticsProvider_ptr port = resourceInfo->usesPorts[i];
        CORBA::ULong hash = port->_hash(1024);

        this->hashToResourceInfo.erase(hash);
    }

    // Unmap the block ID from the resource info
    this->blockToResourceInfo.erase(resourceInfo->blockID);

    // Unmap the block ID from the graph
    std::string graphName = this->blockToGraph[resourceInfo->blockID]->get_name();
    this->blockToGraph.erase(resourceInfo->blockID);

    // Clean up the list
    std::list<std::string> *list = this->blockToList[resourceInfo->blockID];
    std::list<std::string>::iterator it = std::find(list->begin(), list->end(), resourceInfo->blockID);

    list->erase(it);

    if (list->empty()) {
        delete list;
        this->graphToList.erase(graphName);
    }

    this->blockToList.erase(resourceInfo->blockID);

    // Delete and remove the resource info mapping
    delete this->IDToResourceInfo[ID];
    this->IDToResourceInfo.erase(ID);

    // Call the parent terminate
    RFNoC_DefaultPersona_persona_base::terminate(processId);
}

void RFNoC_DefaultPersona_i::setBlockIDMapping(const std::string &componentID, const std::string &blockID)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    std::map<std::string, ResourceInfo *>::iterator it = this->IDToResourceInfo.find(componentID);

    if (it == this->IDToResourceInfo.end()) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to set Block ID for unknown component: " << componentID);
        return;
    }

    LOG_INFO(RFNoC_DefaultPersona_i, componentID << " -> " << blockID);

    this->IDToResourceInfo[componentID]->blockID = blockID;
    this->blockToResourceInfo[blockID] = this->IDToResourceInfo[componentID];
}

void RFNoC_DefaultPersona_i::setHwLoadStatusCallback(hwLoadStatusCallback cb)
{
    hw_load_status_object hwLoadStatusObject;

    hwLoadStatusObject.hardware_id = this->hw_load_status.hardware_id;
    hwLoadStatusObject.load_filepath = this->hw_load_status.load_filepath;
    hwLoadStatusObject.request_id = this->hw_load_status.request_id;
    hwLoadStatusObject.requester_id = this->hw_load_status.requester_id;
    hwLoadStatusObject.state = this->hw_load_status.state;

    cb(hwLoadStatusObject);
}

void RFNoC_DefaultPersona_i::setSetRxStreamer(const std::string &componentID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    std::map<std::string, ResourceInfo *>::iterator it = this->IDToResourceInfo.find(componentID);

    if (it == this->IDToResourceInfo.end()) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to set setRxStreamer for unknown component: " << componentID);
        return;
    }

    this->IDToResourceInfo[componentID]->setRxStreamerCb = cb;
}

void RFNoC_DefaultPersona_i::setSetTxStreamer(const std::string &componentID, setStreamerCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    std::map<std::string, ResourceInfo *>::iterator it = this->IDToResourceInfo.find(componentID);

    if (it == this->IDToResourceInfo.end()) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to set setTxStreamer for unknown component: " << componentID);
        return;
    }

    this->IDToResourceInfo[componentID]->setTxStreamerCb = cb;
}

void RFNoC_DefaultPersona_i::setUsrp(uhd::device3::sptr usrp)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->usrp = usrp;

    std::vector<std::string> NoCBlocks = listNoCBlocks();
}

Resource_impl* RFNoC_DefaultPersona_i::generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    // Touch the usrp pointer to validate it
    this->usrp->get_tree();

    // Create a new resource info
    ResourceInfo *resourceInfo = new ResourceInfo;

    // Map the component ID to the resource info
    std::string componentIdentifier;

    for (size_t i = 0; i < size_t(argc); i+=2) {
        if (strcmp(argv[i], "COMPONENT_IDENTIFIER") == 0) {
            componentIdentifier = argv[i+1];
            break;
        }
    }

    // Lock to prevent the service function from using this resource
    boost::mutex::scoped_lock lock(this->resourceLock);

    this->IDToResourceInfo[componentIdentifier] = resourceInfo;

    // Construct the resource
    Resource_impl *resource;

    try {
        blockIDCallback blockIdCb = boost::bind(&RFNoC_DefaultPersona_i::setBlockIDMapping, this, _1, _2);
        setSetStreamerCallback setSetRxStreamerCb = boost::bind(&RFNoC_DefaultPersona_i::setSetRxStreamer, this, _1, _2);
        setSetStreamerCallback setSetTxStreamerCb = boost::bind(&RFNoC_DefaultPersona_i::setSetTxStreamer, this, _1, _2);

        resource = fnptr(argc, argv, this, this->usrp, blockIdCb, setSetRxStreamerCb, setSetTxStreamerCb);

        if (not resource) {
            LOG_ERROR(RFNoC_DefaultPersona_i, "Constructor returned NULL resource");
            throw std::exception();
        }
    } catch (...) {
        LOG_ERROR(RFNoC_DefaultPersona_i, "Constructor threw an exception");

        delete resourceInfo;

        this->IDToResourceInfo.erase(componentIdentifier);

        return NULL;
    }

    resourceInfo->resource = resource;

    // Map the port hashes
    CF::PortSet::PortInfoSequence *portSet = resource->getPortSet();

    LOG_DEBUG(RFNoC_DefaultPersona_i, resource->_identifier);

    for (size_t i = 0; i < portSet->length(); ++i) {
        CF::PortSet::PortInfoType info = portSet->operator [](i);

        LOG_DEBUG(RFNoC_DefaultPersona_i, "Port Name: " << info.name._ptr);
        LOG_DEBUG(RFNoC_DefaultPersona_i, "Port Direction: " << info.direction._ptr);
        LOG_DEBUG(RFNoC_DefaultPersona_i, "Port Repository: " << info.repid._ptr);

        this->hashToResourceInfo[info.obj_ptr->_hash(1024)] = resourceInfo;

        // Store the uses port pointers
        if (strstr(info.direction._ptr, "Uses") && strstr(info.repid._ptr, "BULKIO")) {
            BULKIO::UsesPortStatisticsProvider_ptr usesPort = BULKIO::UsesPortStatisticsProvider::_narrow(resource->getPort(info.name._ptr));

            resourceInfo->usesPorts.push_back(usesPort);
        }
    }

    return resource;
}

void RFNoC_DefaultPersona_i::hwLoadRequest(CF::Properties& request) {
/*
    // Simple example of a single hw_load_request
    request.length(4);
    request[0].id = CORBA::string_dup("hw_load_request::request_id");
    request[0].value <<= ossie::generateUUID();
    request[1].id = CORBA::string_dup("hw_load_request::requester_id");
    request[1].value <<= ossie::corba::returnString(identifier());
    request[2].id = CORBA::string_dup("hw_load_request::hardware_id");
    request[2].value <<= "MY_HARDWARE_TYPE";
    request[3].id = CORBA::string_dup("hw_load_request::load_filepath");
    request[3].value <<= "/PATH/TO/HW/FILE";
*/
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

std::vector<std::string> RFNoC_DefaultPersona_i::listNoCBlocks()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    std::vector<std::string> NoCBlocks;

    uhd::property_tree::sptr tree = this->usrp->get_tree();

    std::vector<std::string> xBarItems = tree->list("/mboards/0/xbar/");

    for (size_t i = 0; i < xBarItems.size(); ++i) {
        LOG_DEBUG(RFNoC_DefaultPersona_i, xBarItems[i]);

        NoCBlocks.push_back("0/" + xBarItems[i]);
    }

    return NoCBlocks;
}
