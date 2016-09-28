#ifndef RFNOC_DEFAULTPERSONA_I_IMPL_H
#define RFNOC_DEFAULTPERSONA_I_IMPL_H

#include "RFNoC_DefaultPersona_persona_base.h"
#include "HwLoadStatus.h"

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

        void setHwLoadStatusCallback(hwLoadStatusCallback cb);

    protected:
        void hwLoadRequest(CF::Properties& request);
};

#endif // RFNOC_DEFAULTPERSONA_I_IMPL_H
