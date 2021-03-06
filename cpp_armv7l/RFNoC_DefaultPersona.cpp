// Class Include
#include "RFNoC_DefaultPersona.h"

// Boost Include(s)
#include <boost/filesystem.hpp>

PREPARE_LOGGING(RFNoC_DefaultPersona_i)

/*
 * Constructor(s) and/or Destructor
 */

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl),
    enabled(false)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    enabled(false)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    enabled(false)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    enabled(false)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::~RFNoC_DefaultPersona_i()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

/*
 * Public Method(s)
 */

CORBA::Boolean RFNoC_DefaultPersona_i::allocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (isBusy() || isLocked())
    {
        LOG_WARN(RFNoC_DefaultPersona_i, __FUNCTION__ <<
            ": Cannot allocate capacities... Device state is locked and/or busy");

        return false;
    }

    // Program the parent and instantiate a resource manager, if necessary
    bool allocationSuccess = attemptToProgramParent();

    if (allocationSuccess and not this->enabled)
    {
        this->enabled = true;
    }

    return allocationSuccess;
}

void RFNoC_DefaultPersona_i::deallocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->enabled = false;

    attemptToUnprogramParent();
}

CF::ExecutableDevice::ProcessID_Type RFNoC_DefaultPersona_i::execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
    throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
            CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
            CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (not this->enabled)
    {
        LOG_ERROR(RFNoC_DefaultPersona_i, "This persona has not been loaded yet");

        throw CF::ExecutableDevice::ExecuteFail();
    }

    // Call the parent execute
    CF::ExecutableDevice::ProcessID_Type pid = RFNoC_DefaultPersona_persona_base::execute(name, options, parameters);

    // Map the PID to the component identifier
    std::string componentIdentifier;

    for (size_t i = 0; i < parameters.length(); ++i)
    {
        std::string id = parameters[i].id._ptr;

        if (id == "COMPONENT_IDENTIFIER")
        {
            componentIdentifier = ossie::any_to_string(parameters[i].value);

            break;
        }
    }

    this->pidToComponentID[pid] = componentIdentifier;

    return pid;
}

void RFNoC_DefaultPersona_i::releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Terminate all children that were executed_
    std::map<CF::ExecutableDevice::ProcessID_Type, std::string>::iterator iter;

    for (iter = this->pidToComponentID.begin(); iter != this->pidToComponentID.end(); iter++)
    {
        this->terminate(iter->first);
    }

    // This function clears the device running condition so main shuts down everything
    try
    {
        stop();
    }
    catch (CF::Resource::StopError& ex)
    {
        // TODO - this should probably be logged instead of ignored
    }

    // SR:419
    LOG_DEBUG(RFNoC_DefaultPersona_i, "Receive releaseObject call");

    if (this->_adminState == CF::Device::UNLOCKED)
    {
        LOG_DEBUG(RFNoC_DefaultPersona_i, "Releasing Device")
        setAdminState(CF::Device::SHUTTING_DOWN);

        // SR:418
        if (!CORBA::is_nil(this->_aggregateDevice))
        {
            try
            {
                this->_aggregateDevice->removeDevice(this->_this());
            }
            catch (...)
            {
            }
        }

        // SR:420
        setAdminState(CF::Device::LOCKED);

        try
        {
            // SR:422
            LOG_DEBUG(RFNoC_DefaultPersona_i, "Unregistering Device")

            this->_deviceManager->unregisterDevice(this->_this());
        }
        catch (...)
        {
            // SR:423
            throw CF::LifeCycle::ReleaseError();
        }

        LOG_DEBUG(RFNoC_DefaultPersona_i, "Done Releasing Device")
    }

    RH_NL_DEBUG("Device", "Clean up IDM_CHANNEL. DEV-ID:"  << _identifier );

    if ( idm_publisher )  idm_publisher.reset();

    delete this->getDeviceManager(); // Somewhat evil

    releasePorts();
    stopPropertyChangeMonitor();

    // This is a singleton shared by all code running in this process
    //redhawk::events::Manager::Terminate();
    PortableServer::POA_ptr root_poa = ossie::corba::RootPOA();
    PortableServer::ObjectId_var oid = root_poa->servant_to_id(this);
    root_poa->deactivate_object(oid);

    component_running.signal();
}

void RFNoC_DefaultPersona_i::terminate (CF::ExecutableDevice::ProcessID_Type processId)
                    throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->pidToComponentID.find(processId) == this->pidToComponentID.end())
    {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to terminate a process with an ID not tracked by this Device");

        throw CF::ExecutableDevice::InvalidProcess();
    }

    // Get the component identifier associated with this PID
    std::string componentID = this->pidToComponentID[processId];

    LOG_DEBUG(RFNoC_DefaultPersona_i, "Terminating component: " << componentID);

    // Unmap the PID from the component identifier
    this->pidToComponentID.erase(processId);

    // Call the parent terminate
    RFNoC_DefaultPersona_persona_base::terminate(processId);
}

/*
 * Public RFNoC_Persona Method(s)
 */

bool RFNoC_DefaultPersona_i::connectBlocks(const RFNoC_RH::BlockDescriptor &outputDescriptor, const RFNoC_RH::BlockDescriptor &inputDescriptor)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	return this->programmable->connectBlocks(outputDescriptor, inputDescriptor);
}

uhd::rfnoc::block_ctrl_base::sptr RFNoC_DefaultPersona_i::getBlock(RFNoC_RH::BlockDescriptor &blockDescriptor)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	return this->programmable->getBlock(blockDescriptor);
}

RFNoC_RH::BlockDescriptor RFNoC_DefaultPersona_i::getBlockDescriptorFromHash(RFNoC_RH::PortHashType portHash)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    //return this->resourceManager->getProvidesBlockDescriptorFromHash(portHash);
    return this->programmable->getBlockDescriptorFromHash(portHash);
}

void RFNoC_DefaultPersona_i::incomingConnectionAdded(const std::string &resourceId,
								 	 	 	 	 	 const std::string &streamId,
													 RFNoC_RH::PortHashType portHash)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->incomingConnectionAdded(resourceId, streamId, portHash);
}

void RFNoC_DefaultPersona_i::incomingConnectionRemoved(const std::string &resourceId,
								   	   	   	   	   	   const std::string &streamId,
													   RFNoC_RH::PortHashType portHash)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->incomingConnectionRemoved(resourceId, streamId, portHash);
}

void RFNoC_DefaultPersona_i::outgoingConnectionAdded(const std::string &resourceId,
								 	 	 	 	 	 const std::string &connectionId,
													 RFNoC_RH::PortHashType portHash)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->outgoingConnectionAdded(resourceId, connectionId, portHash);
}

void RFNoC_DefaultPersona_i::outgoingConnectionRemoved(const std::string &resourceId,
								   	   	   	   	   	   const std::string &connectionId,
													   RFNoC_RH::PortHashType portHash)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->outgoingConnectionRemoved(resourceId, connectionId, portHash);
}

void RFNoC_DefaultPersona_i::setRxStreamDescriptor(const std::string &resourceId, const RFNoC_RH::StreamDescriptor &streamDescriptor)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->setRxStreamDescriptor(resourceId, streamDescriptor);
}

void RFNoC_DefaultPersona_i::setTxStreamDescriptor(const std::string &resourceId, const RFNoC_RH::StreamDescriptor &streamDescriptor)
{
	LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

	this->programmable->setTxStreamDescriptor(resourceId, streamDescriptor);
}

/*
 * Protected Method(s)
 */

void RFNoC_DefaultPersona_i::constructor()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Set the hardware load status property
    this->hw_load_status.hardware_id = "xc7z020clg484-1";
    this->hw_load_status.load_filepath = this->loadFilepath;
    this->hw_load_status.request_id = "";
    this->hw_load_status.requester_id = "";
    this->hw_load_status.state = 0;

    this->setThreadDelay(1.0);

    this->addPropertyListener(this->loadFilepath, this, &RFNoC_DefaultPersona_i::loadFilepathChanged);

    this->start();
}

/*
 * Protected RFNoC_DefaultPersona_persona_base Method(s)
 */

Resource_impl* RFNoC_DefaultPersona_i::generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Construct the resource
    Resource_impl *resource;

    try
    {
        resource = this->programmable->generateResource(argc, argv, fnptr, libraryName);

        if (not resource)
        {
            LOG_ERROR(RFNoC_DefaultPersona_i, "Programmable returned NULL resource");
            throw std::exception();
        }
    }
    catch (...)
    {
        LOG_ERROR(RFNoC_DefaultPersona_i, "Programmable threw an exception");

        return NULL;
    }

    return resource;
}

void RFNoC_DefaultPersona_i::hwLoadRequest(CF::Properties& request)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    request.length(4);

    request[0].id = "request_id";
    request[0].value <<= ossie::generateUUID();
    request[1].id = "requester_id";
    request[1].value <<= ossie::corba::returnString(this->_identifier.c_str());
    request[2].id = "hardware_id";
    request[2].value <<= ossie::corba::returnString(this->hw_load_status.hardware_id.c_str());
    request[3].id = "load_filepath";
    request[3].value <<= ossie::corba::returnString(this->hw_load_status.load_filepath.c_str());
}

/*
 * Private Method(s)
 */

void RFNoC_DefaultPersona_i::loadFilepathChanged(const std::string &oldValue, const std::string &newValue)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->enabled)
    {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to change the load filepath while allocated. Please deallocate and try again");

        this->loadFilepath = oldValue;

        return;
    }

    if (not boost::filesystem::exists(newValue))
    {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to change load filepath to a path which doesn't exist");

        this->loadFilepath = oldValue;

        return;
    }

    this->hw_load_status.load_filepath = newValue;
}

CF::Device::UsageType RFNoC_DefaultPersona_i::updateUsageState()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->enabled)
    {
        return CF::Device::ACTIVE;
    }

    return CF::Device::IDLE;
}
