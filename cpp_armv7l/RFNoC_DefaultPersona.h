#ifndef RFNOC_DEFAULTPERSONA_I_IMPL_H
#define RFNOC_DEFAULTPERSONA_I_IMPL_H

// Base Include(s)
#include "RFNoC_DefaultPersona_persona_base.h"
#include "RFNoC_Persona.h"

// Forward Declaration(s)
class RFNoC_DefaultPersona_i;

class RFNoC_DefaultPersona_i : public RFNoC_DefaultPersona_persona_base, public RFNoC_RH::RFNoC_Persona
{
    ENABLE_LOGGING

	// Constructor(s) and/or Destructor
    public:
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        RFNoC_DefaultPersona_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);

        ~RFNoC_DefaultPersona_i();

    // Public Method(s)
    public:
        CORBA::Boolean allocateCapacity(const CF::Properties& capacities)
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CF::Device::InsufficientCapacity, CORBA::SystemException);

        void deallocateCapacity(const CF::Properties& capacities)
            throw (CF::Device::InvalidState, CF::Device::InvalidCapacity, CORBA::SystemException);

        virtual CF::ExecutableDevice::ProcessID_Type execute (const char* name, const CF::Properties& options, const CF::Properties& parameters)
			throw ( CF::ExecutableDevice::ExecuteFail, CF::InvalidFileName, CF::ExecutableDevice::InvalidOptions,
					CF::ExecutableDevice::InvalidParameters, CF::ExecutableDevice::InvalidFunction, CF::Device::InvalidState,
					CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

		virtual void terminate (CF::ExecutableDevice::ProcessID_Type processId)
			throw ( CF::Device::InvalidState, CF::ExecutableDevice::InvalidProcess, CORBA::SystemException);

	// Public RFNoC_Persona Method(s)
    public:
		bool connectBlocks(const RFNoC_RH::BlockDescriptor &outputDescriptor, const RFNoC_RH::BlockDescriptor &inputDescriptor);

		/*
		 * This method should return an RF-NoC block to the caller. If the port was unset by the
		 * caller, the port used will be placed into blockDescriptor.
		 */
		uhd::rfnoc::block_ctrl_base::sptr getBlock(RFNoC_RH::BlockDescriptor &blockDescriptor);

		/*
		 * This method should return the block descriptor corresponding to the
		 * given port hash.
		 */
		RFNoC_RH::BlockDescriptor getBlockDescriptorFromHash(RFNoC_RH::PortHashType portHash);

		void incomingConnectionAdded(const std::string &resourceId,
									 const std::string &streamId,
									 RFNoC_RH::PortHashType portHash);

		void incomingConnectionRemoved(const std::string &resourceId,
									   const std::string &streamId,
									   RFNoC_RH::PortHashType portHash);

		void outgoingConnectionAdded(const std::string &resourceId,
									 const std::string &connectionId,
									 RFNoC_RH::PortHashType portHash);

		void outgoingConnectionRemoved(const std::string &resourceId,
									   const std::string &connectionId,
									   RFNoC_RH::PortHashType portHash);

		void setRxStreamDescriptor(const std::string &resourceId, const RFNoC_RH::StreamDescriptor &streamDescriptor);

		void setTxStreamDescriptor(const std::string &resourceId, const RFNoC_RH::StreamDescriptor &streamDescriptor);

    // Protected Method(s)
    protected:
        void constructor();

        // Don't use the default serviceFunction for clarity
        int serviceFunction() { return FINISH; }

    // Protected RFNoC_DefaultPersona_persona_base Method(s)
    protected:
        Resource_impl* generateResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);

        void hwLoadRequest(CF::Properties& request);

    // Private Method(s)
    private:
        void loadFilepathChanged(const std::string &oldValue, const std::string &newValue);

        CF::Device::UsageType updateUsageState();

    // Private Member(s)
    private:
        bool enabled;
        std::map<CF::ExecutableDevice::ProcessID_Type, std::string> pidToComponentID;
};

#endif
