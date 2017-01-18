#include <iostream>
#include "ossie/ossieSupport.h"

#include "RFNoC_DefaultPersona.h"

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
    Device_impl* construct(int argc, char* argv[], Device_impl* parentDevice, connectRadioRXCallback connectRadioRXCb, connectRadioTXCallback connectRadioTXCb, getUsrpCallback getUsrpCb, hwLoadStatusCallback hwLoadStatusCb, setGetBlockInfoFromHashCallback setGetBlockInfoFromHashCb) {

        struct sigaction sa;
        sa.sa_handler = signal_catcher;
        sa.sa_flags = 0;
        devicePtr = 0;

        Device_impl::start_device(&devicePtr, sa, argc, argv);

        // Any addition parameters passed into construct can now be
        // set directly onto devicePtr since it is the instantiated
        // Redhawk device
        //      Example:
        //         devicePtr->setSharedAPI(sharedAPI);
        devicePtr->setParentDevice(parentDevice);
        devicePtr->setConnectRadioRXCallback(connectRadioRXCb);
        devicePtr->setConnectRadioTXCallback(connectRadioTXCb);
        devicePtr->setGetUsrp(getUsrpCb);
        devicePtr->setHwLoadStatusCallback(hwLoadStatusCb);
        setGetBlockInfoFromHashCb(devicePtr->_identifier, boost::bind(&RFNoC_DefaultPersona_i::getBlockInfoFromHash, devicePtr, _1));

        return devicePtr;
    }
}

