/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "RFNoC_DefaultPersona.h"

#include <boost/filesystem.hpp>
#include <frontend/frontend.h>

PREPARE_LOGGING(RFNoC_DefaultPersona_i)

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    enabled(false),
    resourceManager(NULL)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

RFNoC_DefaultPersona_i::~RFNoC_DefaultPersona_i()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->resourceManager) {
        delete this->resourceManager;
    }
}

void RFNoC_DefaultPersona_i::constructor()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Set the hardware load status property
    this->hw_load_status.hardware_id = "E310";
    this->hw_load_status.load_filepath = this->loadFilepath;
    this->hw_load_status.request_id = "";
    this->hw_load_status.requester_id = "";
    this->hw_load_status.state = 0;

    // Let the programmable device know about this device's hardware status
    hw_load_status_object hwLoadStatusObject;

    hwLoadStatusObject.hardware_id = this->hw_load_status.hardware_id;
    hwLoadStatusObject.load_filepath = this->hw_load_status.load_filepath;
    hwLoadStatusObject.request_id = this->hw_load_status.request_id;
    hwLoadStatusObject.requester_id = this->hw_load_status.requester_id;
    hwLoadStatusObject.state = this->hw_load_status.state;

    this->hwLoadStatusCb(hwLoadStatusObject);

    LOG_INFO(RFNoC_DefaultPersona_i, "Hardware ID: " << hw_load_status.hardware_id);

    this->setThreadDelay(1.0);

    this->addPropertyListener(this->loadFilepath, this, &RFNoC_DefaultPersona_i::loadFilepathChanged);

    this->start();
}

int RFNoC_DefaultPersona_i::serviceFunction()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->resourceManager) {
        this->resourceManager->update();
    }

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

        if (allocationSuccess) {
            LOG_DEBUG(RFNoC_DefaultPersona_i, "Instantiating RF-NoC Resource Manager");
            this->resourceManager = new RFNoC_ResourceManager(this, this->getUsrpCb(), this->connectRadioRXCb, this->connectRadioTXCb);

            this->enabled = true;
        }
    }

    this->_usageState = updateUsageState();

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

    attemptToUnprogramParent();

    this->_usageState = updateUsageState();
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
            componentIdentifier = ossie::any_to_string(parameters[i].value);
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

    if (this->pidToComponentID.find(processId) == this->pidToComponentID.end()) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to terminate a process with an ID not tracked by this Device");
        throw CF::ExecutableDevice::InvalidProcess();
    }

    // Get the component identifier associated with this PID
    std::string componentID = this->pidToComponentID[processId];

    this->resourceManager->removeResource(componentID);

    LOG_DEBUG(RFNoC_DefaultPersona_i, "Terminating component: " << componentID);

    // Unmap the PID from the component identifier
    this->pidToComponentID.erase(processId);

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

void RFNoC_DefaultPersona_i::setGetUsrp(getUsrpCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->getUsrpCb = cb;
}

void RFNoC_DefaultPersona_i::setHwLoadStatusCallback(hwLoadStatusCallback cb)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->hwLoadStatusCb = cb;
}

Resource_impl* RFNoC_DefaultPersona_i::generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    // Construct the resource
    Resource_impl *resource;

    try {
        resource = this->resourceManager->addResource(argc, argv, fnptr, libraryName);

        if (not resource) {
            LOG_ERROR(RFNoC_DefaultPersona_i, "Resource Manager returned NULL resource");
            throw std::exception();
        }
    } catch (...) {
        LOG_ERROR(RFNoC_DefaultPersona_i, "Resource Manager threw an exception");

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

CF::Device::UsageType RFNoC_DefaultPersona_i::updateUsageState()
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->enabled) {
        return CF::Device::ACTIVE;
    }

    return CF::Device::IDLE;
}

void RFNoC_DefaultPersona_i::loadFilepathChanged(const std::string &oldValue, const std::string &newValue)
{
    LOG_TRACE(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    if (this->enabled) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to change the load filepath while allocated. Please deallocate and try again");
        this->loadFilepath = oldValue;
        return;
    }

    if (not boost::filesystem::exists(newValue)) {
        LOG_WARN(RFNoC_DefaultPersona_i, "Attempted to change load filepath to a path which doesn't exist");
        this->loadFilepath = oldValue;
        return;
    }

    this->hw_load_status.load_filepath = newValue;

    // Let the programmable device know about this device's hardware status
    hw_load_status_object hwLoadStatusObject;

    hwLoadStatusObject.hardware_id = this->hw_load_status.hardware_id;
    hwLoadStatusObject.load_filepath = this->hw_load_status.load_filepath;
    hwLoadStatusObject.request_id = this->hw_load_status.request_id;
    hwLoadStatusObject.requester_id = this->hw_load_status.requester_id;
    hwLoadStatusObject.state = this->hw_load_status.state;

    this->hwLoadStatusCb(hwLoadStatusObject);
}
