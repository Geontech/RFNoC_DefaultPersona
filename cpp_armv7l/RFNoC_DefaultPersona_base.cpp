#include "RFNoC_DefaultPersona_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the device class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

RFNoC_DefaultPersona_base::RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    Device_impl(devMgr_ior, id, lbl, sftwrPrfl),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

RFNoC_DefaultPersona_base::RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    Device_impl(devMgr_ior, id, lbl, sftwrPrfl, compDev),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

RFNoC_DefaultPersona_base::RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    Device_impl(devMgr_ior, id, lbl, sftwrPrfl, capacities),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

RFNoC_DefaultPersona_base::RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    Device_impl(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev),
    AggregateDevice_impl(),
    ThreadedComponent()
{
    construct();
}

RFNoC_DefaultPersona_base::~RFNoC_DefaultPersona_base()
{
}

void RFNoC_DefaultPersona_base::construct()
{
    loadProperties();

}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void RFNoC_DefaultPersona_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    Device_impl::start();
    ThreadedComponent::startThread();
}

void RFNoC_DefaultPersona_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    Device_impl::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void RFNoC_DefaultPersona_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the device running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    Device_impl::releaseObject();
}

void RFNoC_DefaultPersona_base::loadProperties()
{
    addProperty(device_kind,
                "PERSONA",
                "DCE:cdc5ee18-7ceb-4ae6-bf4c-31f983179b4d",
                "device_kind",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(device_model,
                "DEFAULT",
                "DCE:0f99b2e4-9903-4631-9846-ff349d18ecfb",
                "device_model",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(processor_name,
                "RF-NoC-Default",
                "DCE:9B445600-6C7F-11d4-A226-0050DA314CD6",
                "processor_name",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(os_name,
                "Linux",
                "DCE:80BF17F0-6C7F-11d4-A226-0050DA314CD6",
                "os_name",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(os_version,
                "3.14.2-xilinx",
                "DCE:0f3a9a37-a342-43d8-9b7f-78dc6da74192",
                "os_version",
                "readonly",
                "",
                "eq",
                "allocation");

    addProperty(hw_load_status,
                hw_load_status_struct(),
                "hw_load_status",
                "",
                "readonly",
                "",
                "external",
                "configure");

}


