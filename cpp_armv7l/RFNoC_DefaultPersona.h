#ifndef RFNOC_DEFAULTPERSONA_I_IMPL_H
#define RFNOC_DEFAULTPERSONA_I_IMPL_H

#include "RFNoC_DefaultPersona_persona_base.h"
#include "HwLoadStatus.h"

#include <uhd/device3.hpp>

class RFNoC_DefaultPersona_i;

class RFNoC_DefaultPersona_i : public RFNoC_DefaultPersona_persona_base
{
    ENABLE_LOGGING
    public:
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~RFNoC_DefaultPersona_i();

        virtual void construct();

        int serviceFunction();
        CORBA::Boolean allocateCapacity(const CF::Properties& capacities) 
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException);
        void deallocateCapacity(const CF::Properties& capacities) 
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException);

        CF::ExecutableDevice::ProcessID_Type execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
            throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
                    CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
                    CORBA::SystemException);
        void terminate (CF::ExecutableDevice::ProcessID_Type processId)
                    throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException);

        void setHwLoadStatusCallback(hwLoadStatusCallback cb);
        void setUsrp(uhd::device3::sptr usrp);

    protected:
        Resource_impl* generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void hwLoadRequest(CF::Properties& request);

    private:
        std::vector<std::string> listNoCBlocks();

    private:
        std::map<CF::ExecutableDevice::ProcessID_Type, Resource_impl *> resourceMap;
        uhd::device3::sptr usrp;
};

#endif // RFNOC_DEFAULTPERSONA_I_IMPL_H
