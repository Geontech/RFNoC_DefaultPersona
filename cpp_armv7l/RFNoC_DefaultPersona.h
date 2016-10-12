#ifndef RFNOC_DEFAULTPERSONA_I_IMPL_H
#define RFNOC_DEFAULTPERSONA_I_IMPL_H

#include "RFNoC_DefaultPersona_persona_base.h"
#include "HwLoadStatus.h"

#include <uhd/usrp/multi_usrp.hpp>

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
        void setUsrp(uhd::usrp::multi_usrp::sptr usrp);

    protected:
        Resource_impl* generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void hwLoadRequest(CF::Properties& request);

    private:
        std::vector<std::string> listNoCBlocks();

    private:
        uhd::usrp::multi_usrp::sptr usrp;
};

#endif // RFNOC_DEFAULTPERSONA_I_IMPL_H
