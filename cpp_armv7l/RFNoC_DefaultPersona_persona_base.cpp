/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "RFNoC_DefaultPersona_persona_base.h"

PREPARE_LOGGING(RFNoC_DefaultPersona_persona_base)

RFNoC_DefaultPersona_persona_base::RFNoC_DefaultPersona_persona_base (
                        char            *devMgr_ior, 
                        char            *id, 
                        char            *lbl, 
                        char            *sftwrPrfl ) 
:
    RFNoC_DefaultPersona_base(devMgr_ior, id, lbl, sftwrPrfl),
    _previousRequestProps()
{
    construct();
}

RFNoC_DefaultPersona_persona_base::RFNoC_DefaultPersona_persona_base (
                        char            *devMgr_ior, 
                        char            *id, 
                        char            *lbl, 
                        char            *sftwrPrfl, 
                        char            *compDev ) 
:
    RFNoC_DefaultPersona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    _previousRequestProps()
{
    construct();
}

RFNoC_DefaultPersona_persona_base::RFNoC_DefaultPersona_persona_base (
                        char            *devMgr_ior, 
                        char            *id, 
                        char            *lbl, 
                        char            *sftwrPrfl, 
                        CF::Properties  capacities ) 
:
    RFNoC_DefaultPersona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    _previousRequestProps()
{
    construct();
}

RFNoC_DefaultPersona_persona_base::RFNoC_DefaultPersona_persona_base (
                        char            *devMgr_ior, 
                        char            *id, 
                        char            *lbl, 
                        char            *sftwrPrfl, 
                        CF::Properties  capacities, 
                        char            *compDev ) 
:
    RFNoC_DefaultPersona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    _previousRequestProps()
{
    construct();
}

void RFNoC_DefaultPersona_persona_base::construct() 
{
    // Initialize state to safe defaults
    _parentDevice = NULL;
    _parentAllocated = false;
}

// TODO: This was overriden since setting admin state is not accessible via the current IDL
void RFNoC_DefaultPersona_persona_base::adminState(CF::Device::AdminType adminState) 
    throw (CORBA::SystemException)
{
    // Force admin state to change usage state since usage state is currently protected
    switch (adminState) {
        case CF::Device::LOCKED:
            setUsageState(CF::Device::BUSY);
            break;
        case CF::Device::UNLOCKED:
            setUsageState(CF::Device::IDLE);
            break;
        case CF::Device::SHUTTING_DOWN:
            // Do nothing
            break;
    }

    RFNoC_DefaultPersona_base::adminState(adminState);
}

void RFNoC_DefaultPersona_persona_base::releaseObject() 
    throw (
        CF::LifeCycle::ReleaseError, 
        CORBA::SystemException ) 
{
    RFNoC_DefaultPersona_base::releaseObject();
}


CORBA::Boolean RFNoC_DefaultPersona_persona_base::attemptToProgramParent() 
{
    // Return false if there is no reference to the parent
    if (_parentDevice == NULL) {
        LOG_ERROR(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << 
            ": No reference to parent exists!");
        return false;
    }

    if (_parentAllocated == false) {

        LOG_DEBUG(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << 
            ": About to allocate parent device");
        
        beforeHardwareProgrammed();

        // Grab user-defined allocation request and format them properly
        CF::Properties requestProps;
        CF::Properties formattedRequestProps;

        hwLoadRequest(requestProps);
        formatRequestProps(requestProps, formattedRequestProps);

        _parentAllocated = _parentDevice->allocateCapacity(formattedRequestProps);

        if (_parentAllocated) {
            _previousRequestProps = formattedRequestProps;
            afterHardwareProgramSuccess();
        } else {
            afterHardwareProgramFailure();
        }
    }
    return _parentAllocated;
}

CORBA::Boolean RFNoC_DefaultPersona_persona_base::attemptToUnprogramParent() 
{
    // Return false if there is no reference to the parent
    if (_parentDevice == NULL) {
        LOG_ERROR(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << ": No reference to parent exists!");
        return false;
    }
    
    if (_parentAllocated) {
        beforeHardwareUnprogrammed();
        
        // Grab previous user-defined allocation request
        if (_previousRequestProps.length() == 0) {
            LOG_ERROR(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << ": Previously requested hw_load Props empty!");
            return false;
        }

        _parentDevice->deallocateCapacity(_previousRequestProps);
        _parentAllocated = false;
        afterHardwareUnprogrammed();
    }
    return !_parentAllocated;
}


// Transforms user-supplied properties into safe-usable CF::Properties
void RFNoC_DefaultPersona_persona_base::formatRequestProps(
                    const CF::Properties&       requestProps, 
                    CF::Properties&             formattedProps) 
{
    // Initialize local variables
    std::string propId;
    bool allPropsAreHwLoadRequest = false;
    bool foundRequestId = false;
    CF::Properties hwLoadRequest;

    // Sanity check - Kick out if properties are empty
    if (requestProps.length() == 0) {
        LOG_ERROR(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << 
            ": Unable to format hw_load_request properties.  Properties are empty!");
        return;
    }

    // Case 1 - Properties are already properly formatted
    if (requestProps.length() == 1) {
        propId = requestProps[0].id;
        if (propId == "hw_load_requests") {
            LOG_DEBUG(RFNoC_DefaultPersona_persona_base, __FUNCTION__ <<
                ": No formatting occurred - Request properties are properly formatted!");
            formattedProps = requestProps;
            return;
        }
    }

    // Inspect the properties further
    allPropsAreHwLoadRequest = true;
    for (size_t ii = 0; ii < requestProps.length(); ii++) {
        propId = requestProps[ii].id;

        allPropsAreHwLoadRequest &= (propId == "hw_load_request");
        foundRequestId    |= (propId == "request_id") ||
                             (propId == "hw_load_request::request_id");
    }
  
    // Case 2 - Properties are multiple hw_load_request structs
    if (allPropsAreHwLoadRequest) {
        LOG_DEBUG(RFNoC_DefaultPersona_persona_base, __FUNCTION__ << 
            ": Found hw_load_request array - Formatting to structseq");
        formattedProps.length(1);
        formattedProps[0].id = "hw_load_requests";
        formattedProps[0].value <<= requestProps;
        return;
    }

    // Case 3 - Properties reprensent the contents of a single hw_load_request
    if (foundRequestId) {
        LOG_DEBUG(RFNoC_DefaultPersona_persona_base, __FUNCTION__ <<
            ": Found hw_load_request contents - Formatting to struct and structseq");
        
        hwLoadRequest.length(1);
        hwLoadRequest[0].id = "hw_load_request";
        hwLoadRequest[0].value <<= requestProps;

        formattedProps.length(1);
        formattedProps[0].id = "hw_load_requests";
        formattedProps[0].value <<= hwLoadRequest;
        return;
    }
    
    LOG_ERROR(RFNoC_DefaultPersona_persona_base, __FUNCTION__ <<
        ": Unable to format hw_load_request properties - Format unknown!");
}
