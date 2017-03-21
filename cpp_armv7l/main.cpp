#include <iostream>
#include "ossie/ossieSupport.h"

#include "RFNoC_DefaultPersona.h"
#include "RFNoC_Programmable.h"

#include <uhd/device3.hpp>

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
    Device_impl* construct(int argc, char* argv[], Device_impl *parentDevice) {

        struct sigaction sa;
        sa.sa_handler = signal_catcher;
        sa.sa_flags = 0;
        devicePtr = 0;

        Device_impl::start_device(&devicePtr, sa, argc, argv);

        RFNoC_Programmable *RFNoC_ProgrammableDevice = dynamic_cast<RFNoC_Programmable *>(parentDevice);

        if (not RFNoC_ProgrammableDevice) {
            return NULL;
        }

        devicePtr->setParentDevice(parentDevice);
        devicePtr->setProgrammable(RFNoC_ProgrammableDevice);
        RFNoC_ProgrammableDevice->setPersonaMapping(devicePtr->_identifier, (RFNoC_Persona *) devicePtr);

        return devicePtr;
    }
}

