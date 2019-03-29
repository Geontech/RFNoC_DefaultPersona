// Component Include
#include "RFNoC_DefaultPersona.h"

// RF-NoC RH Include(s)
#include <RFNoC_Persona.h>
#include <RFNoC_Programmable.h>

RFNoC_DefaultPersona_i *devicePtr;

void signal_catcher(int sig)
{
    // IMPORTANT Don't call exit(...) in this function
    // issue all CORBA calls that you need for cleanup here before calling ORB shutdown
    if (devicePtr) {
        devicePtr->halt();
    }
}
int main(int argc, char* argv[])
{
    struct sigaction sa;
    sa.sa_handler = signal_catcher;
    sa.sa_flags = 0;
    devicePtr = 0;

    Device_impl::start_device(&devicePtr, sa, argc, argv);
    return 0;
}

extern "C" {
    Device_impl* construct(int argc, char* argv[], RFNoC_RH::RFNoC_Programmable *programmable)
    {
        struct sigaction sa;
        sa.sa_handler = signal_catcher;
        sa.sa_flags = 0;
        devicePtr = 0;

        Device_impl::start_device(&devicePtr, sa, argc, argv);

        RFNoC_RH::RFNoC_Persona *rfNocPersonaPtr = dynamic_cast<RFNoC_RH::RFNoC_Persona *>(devicePtr);

        if (not rfNocPersonaPtr)
        {
        	delete devicePtr;

            return NULL;
        }

        devicePtr->setParentDevice(dynamic_cast<Device_impl*>(programmable));
        rfNocPersonaPtr->setProgrammable(programmable);
        programmable->setPersonaMapping(devicePtr->_identifier, rfNocPersonaPtr);

        return devicePtr;
    }
}

