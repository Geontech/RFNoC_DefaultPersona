/*
 * RFNoC_ResourceManager.h
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#ifndef RFNOC_RESOURCEMANAGER_H
#define RFNOC_RESOURCEMANAGER_H

#include <ossie/debug.h>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/graph.hpp>

#include "RFNoC_Persona.h"

// Forward declaration of other classes
class RFNoC_Resource;

class RFNoC_ResourceManager
{
    ENABLE_LOGGING
    public:
        RFNoC_ResourceManager(Device_impl *parent, uhd::device3::sptr usrp, connectRadioRXCallback rxCb, connectRadioTXCallback txCb);
        ~RFNoC_ResourceManager();

        Resource_impl* addResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void removeResource(const std::string &resourceID);

        BlockInfo getBlockInfoFromHash(const CORBA::ULong &hash) const;
        Device_impl* getParent() const { return this->parent; }
        uhd::device3::sptr getUsrp() { return this->usrp; }

        void setBlockInfoMapping(const std::string &resourceID, const std::vector<BlockInfo> &blockInfos);
        void setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb);
        void setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb);

    protected:
        typedef std::map<std::string, RFNoC_Resource *> RFNoC_ResourceMap;

        connectRadioRXCallback connectRadioRXCb;
        connectRadioTXCallback connectRadioTXCb;
        uhd::rfnoc::graph::sptr graph;
        RFNoC_ResourceMap idToResource;
        Device_impl *parent;
        boost::mutex resourceLock;
        uhd::device3::sptr usrp;
};

#endif
