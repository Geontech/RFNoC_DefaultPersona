/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "RFNoC_DefaultPersona.h"

PREPARE_LOGGING(RFNoC_DefaultPersona_i)

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    RFNoC_DefaultPersona_persona_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    construct();
}

RFNoC_DefaultPersona_i::~RFNoC_DefaultPersona_i()
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

void RFNoC_DefaultPersona_i::construct()
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    this->hw_load_status.hardware_id = "E310";
    this->hw_load_status.load_filepath = "/usr/share/uhd/images/usrp_e310_fpga.bit";
    this->hw_load_status.request_id = "";
    this->hw_load_status.requester_id = "";
    this->hw_load_status.state = 0;

    LOG_INFO(RFNoC_DefaultPersona_i, "Hardware ID: " << hw_load_status.hardware_id);
}

/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
                std::string stream_id = "testStream";
                BULKIO::StreamSRI sri = bulkio::sri::create(stream_id);

	Time:
	    To create a PrecisionUTCTime object, use the following code:
                BULKIO::PrecisionUTCTime tstamp = bulkio::time::utils::now();

        
    Ports:

        Data is passed to the serviceFunction through the getPacket call (BULKIO only).
        The dataTransfer class is a port-specific class, so each port implementing the
        BULKIO interface will have its own type-specific dataTransfer.

        The argument to the getPacket function is a floating point number that specifies
        the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.  Constants have been defined for these values, bulkio::Const::BLOCKING and
        bulkio::Const::NON_BLOCKING.

        Each received dataTransfer is owned by serviceFunction and *MUST* be
        explicitly deallocated.

        To send data using a BULKIO interface, a convenience interface has been added 
        that takes a std::vector as the data input

        NOTE: If you have a BULKIO dataSDDS or dataVITA49 port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // this example assumes that the device has two ports:
            //  A provides (input) port of type bulkio::InShortPort called short_in
            //  A uses (output) port of type bulkio::OutFloatPort called float_out
            // The mapping between the port and the class is found
            // in the device base class header file

            bulkio::InShortPort::dataTransfer *tmp = short_in->getPacket(bulkio::Const::BLOCKING);
            if (not tmp) { // No data is available
                return NOOP;
            }

            std::vector<float> outputData;
            outputData.resize(tmp->dataBuffer.size());
            for (unsigned int i=0; i<tmp->dataBuffer.size(); i++) {
                outputData[i] = (float)tmp->dataBuffer[i];
            }

            // NOTE: You must make at least one valid pushSRI call
            if (tmp->sriChanged) {
                float_out->pushSRI(tmp->SRI);
            }
            float_out->pushPacket(outputData, tmp->T, tmp->EOS, tmp->streamID);

            delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
            return NORMAL;

        If working with complex data (i.e., the "mode" on the SRI is set to
        true), the std::vector passed from/to BulkIO can be typecast to/from
        std::vector< std::complex<dataType> >.  For example, for short data:

            bulkio::InShortPort::dataTransfer *tmp = myInput->getPacket(bulkio::Const::BLOCKING);
            std::vector<std::complex<short> >* intermediate = (std::vector<std::complex<short> >*) &(tmp->dataBuffer);
            // do work here
            std::vector<short>* output = (std::vector<short>*) intermediate;
            myOutput->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);

        Interactions with non-BULKIO ports are left up to the device developer's discretion
        
    Messages:
    
        To receive a message, you need (1) an input port of type MessageEvent, (2) a message prototype described
        as a structure property of kind message, (3) a callback to service the message, and (4) to register the callback
        with the input port.
        
        Assuming a property of type message is declared called "my_msg", an input port called "msg_input" is declared of
        type MessageEvent, create the following code:
        
        void RFNoC_DefaultPersona_i::my_message_callback(const std::string& id, const my_msg_struct &msg){
        }
        
        Register the message callback onto the input port with the following form:
        this->msg_input->registerMessage("my_msg", this, &RFNoC_DefaultPersona_i::my_message_callback);
        
        To send a message, you need to (1) create a message structure, (2) a message prototype described
        as a structure property of kind message, and (3) send the message over the port.
        
        Assuming a property of type message is declared called "my_msg", an output port called "msg_output" is declared of
        type MessageEvent, create the following code:
        
        ::my_msg_struct msg_out;
        this->msg_output->sendMessage(msg_out);

    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given the property id as its name.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (RFNoC_DefaultPersona_persona_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
              
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        A callback method can be associated with a property so that the method is
        called each time the property value changes.  This is done by calling 
        setPropertyChangeListener(<property name>, this, &RFNoC_DefaultPersona_i::<callback method>)
        in the constructor.
            
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            
        //Add to RFNoC_DefaultPersona.cpp
        RFNoC_DefaultPersona_i::RFNoC_DefaultPersona_i(const char *uuid, const char *label) :
            RFNoC_DefaultPersona_persona_base(uuid, label)
        {
            setPropertyChangeListener("scaleValue", this, &RFNoC_DefaultPersona_i::scaleChanged);
        }

        void RFNoC_DefaultPersona_i::scaleChanged(const std::string& id){
            std::cout << "scaleChanged scaleValue " << scaleValue << std::endl;
        }
            
        //Add to RFNoC_DefaultPersona.h
        void scaleChanged(const std::string&);
        
        
************************************************************************************************/
int RFNoC_DefaultPersona_i::serviceFunction()
{
    LOG_DEBUG(RFNoC_DefaultPersona_i, "serviceFunction() example log message");
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
    
    return NOOP;
}

CORBA::Boolean RFNoC_DefaultPersona_i::allocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException) 
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    bool allocationSuccess = false;

    if (isBusy() || isLocked()) {
        LOG_WARN(RFNoC_DefaultPersona_i, __FUNCTION__ << 
            ": Cannot allocate capacities... Device state is locked and/or busy");
        return false;
    }


    /*
     * Do allocation work here...
     */


    if (allocationSuccess) {
        allocationSuccess = attemptToProgramParent();
    }
    return allocationSuccess;
}

void RFNoC_DefaultPersona_i::deallocateCapacity(const CF::Properties& capacities)
        throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException) 
{
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);

    /*
     * Do deallocation work here...
     */


    attemptToUnprogramParent();
}

void RFNoC_DefaultPersona_i::hwLoadRequest(CF::Properties& request) {
/*
    // Simple example of a single hw_load_request
    request.length(4);
    request[0].id = CORBA::string_dup("hw_load_request::request_id");
    request[0].value <<= ossie::generateUUID(); 
    request[1].id = CORBA::string_dup("hw_load_request::requester_id");
    request[1].value <<= ossie::corba::returnString(identifier());
    request[2].id = CORBA::string_dup("hw_load_request::hardware_id");
    request[2].value <<= "MY_HARDWARE_TYPE";
    request[3].id = CORBA::string_dup("hw_load_request::load_filepath");
    request[3].value <<= "/PATH/TO/HW/FILE";
*/
    LOG_INFO(RFNoC_DefaultPersona_i, __PRETTY_FUNCTION__);
}

Resource_impl* RFNoC_DefaultPersona_i::generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName)
{
    Resource_impl *resource = fnptr(argc, argv, this);

    return resource;
}

void RFNoC_DefaultPersona_i::setHwLoadStatusCallback(hwLoadStatusCallback cb)
{
    hw_load_status_object hwLoadStatusObject;

    hwLoadStatusObject.hardware_id = this->hw_load_status.hardware_id;
    hwLoadStatusObject.load_filepath = this->hw_load_status.load_filepath;
    hwLoadStatusObject.request_id = this->hw_load_status.request_id;
    hwLoadStatusObject.requester_id = this->hw_load_status.requester_id;
    hwLoadStatusObject.state = this->hw_load_status.state;

    cb(hwLoadStatusObject);
}


