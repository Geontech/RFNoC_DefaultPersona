#ifndef RFNOC_DEFAULTPERSONA_I_IMPL_H
#define RFNOC_DEFAULTPERSONA_I_IMPL_H

#include "RFNoC_DefaultPersona_persona_base.h"
#include "RFNoC_Persona.h"
#include "RFNoC_ResourceManager.h"

class RFNoC_DefaultPersona_i;

class RFNoC_DefaultPersona_i : public RFNoC_DefaultPersona_persona_base, public RFNoC_Persona
{
    ENABLE_LOGGING
    public:
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~RFNoC_DefaultPersona_i();

        void constructor();

        int serviceFunction() { return FINISH; }
        CORBA::Boolean allocateCapacity(const CF::Properties& capacities) 
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException);
        void deallocateCapacity(const CF::Properties& capacities) 
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException);

        virtual CF::ExecutableDevice::ProcessID_Type execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
            throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
                    CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
                    CORBA::SystemException);
        virtual void terminate (CF::ExecutableDevice::ProcessID_Type processId)
                    throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException);

        BlockInfo getBlockInfoFromHash(const CORBA::ULong &portHash);

    protected:
        Resource_impl* generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void hwLoadRequest(CF::Properties& request);

    private:
        CF::Device::UsageType updateUsageState();

    private:
        void loadFilepathChanged(const std::string &oldValue, const std::string &newValue);

    private:
        bool enabled;
        RFNoC_ResourceManager *resourceManager;

        std::map<CF::ExecutableDevice::ProcessID_Type, std::string> pidToComponentID;
};

#endif // RFNOC_DEFAULTPERSONA_I_IMPL_H
