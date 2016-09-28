#ifndef RFNOC_DEFAULTPERSONA_BASE_IMPL_REPROG_H
#define RFNOC_DEFAULTPERSONA_BASE_IMPL_REPROG_H

#include "RFNoC_DefaultPersona_base.h"
#include "ossie/Device_impl.h"
class RFNoC_DefaultPersona_persona_base;

class RFNoC_DefaultPersona_persona_base : public RFNoC_DefaultPersona_base
{
    ENABLE_LOGGING
    public:
        RFNoC_DefaultPersona_persona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RFNoC_DefaultPersona_persona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RFNoC_DefaultPersona_persona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RFNoC_DefaultPersona_persona_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        virtual void construct();
        virtual void setParentDevice(Device_impl* parentDevice) { _parentDevice = parentDevice; };
        virtual Device_impl* getParentDevice() { return _parentDevice; };
        virtual void adminState(CF::Device::AdminType adminState) 
            throw (CORBA::SystemException);
        virtual void releaseObject() 
            throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

    protected:

        virtual void hwLoadRequest(CF::Properties& loadRequest)=0;
        virtual CORBA::Boolean attemptToProgramParent();
        virtual CORBA::Boolean attemptToUnprogramParent();

        // Lifecycle methods around the programming of parent/hardware
        //    These methods may be overriden to provide custom functionality
        //    at different stages in the lifecycle
        virtual void beforeHardwareProgrammed() {};
        virtual void afterHardwareProgramSuccess() {};
        virtual void afterHardwareProgramFailure() {};
        virtual void beforeHardwareUnprogrammed() {};
        virtual void afterHardwareUnprogrammed() {};
    private:
        Device_impl*            _parentDevice;
        bool                    _parentAllocated;
        CF::Properties          _previousRequestProps;
        virtual void formatRequestProps(const CF::Properties& requestProps, CF::Properties& formattedProps);
};

#endif // RFNOC_DEFAULTPERSONA_BASE_IMPL_REPROG_H
