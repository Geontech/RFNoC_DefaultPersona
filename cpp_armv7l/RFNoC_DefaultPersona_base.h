#ifndef RFNOC_DEFAULTPERSONA_BASE_IMPL_BASE_H
#define RFNOC_DEFAULTPERSONA_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/ExecutableDevice_impl.h>
#include <CF/AggregateDevices.h>
#include <ossie/AggregateDevice_impl.h>
#include <ossie/ThreadedComponent.h>

#include "struct_props.h"

class RFNoC_DefaultPersona_base : public ExecutableDevice_impl, public virtual POA_CF::AggregateExecutableDevice, public AggregateDevice_impl, protected ThreadedComponent
{
    public:
        RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RFNoC_DefaultPersona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~RFNoC_DefaultPersona_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        /// Property: device_kind
        std::string device_kind;
        /// Property: device_model
        std::string device_model;
        /// Property: processor_name
        std::string processor_name;
        /// Property: os_name
        std::string os_name;
        /// Property: os_version
        std::string os_version;
        /// Property: load_filepath
        std::string load_filepath;
        /// Property: hw_load_status
        hw_load_status_struct hw_load_status;

    private:
        void construct();
};
#endif // RFNOC_DEFAULTPERSONA_BASE_IMPL_BASE_H
