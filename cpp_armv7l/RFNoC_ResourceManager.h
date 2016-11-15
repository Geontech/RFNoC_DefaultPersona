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

#include "RFNoC_ResourceList.h"

// Forward declaration of other classes
class RFNoC_ResourceList;

class RFNoC_ResourceManager
{
    ENABLE_LOGGING
    public:
        RFNoC_ResourceManager(Device_impl *parent, uhd::device3::sptr usrp, uhd::device_addr_t usrpAddress);
        ~RFNoC_ResourceManager();

        Resource_impl* addResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName);
        void removeResource(const std::string &resourceID);
        bool update();

        Device_impl* getParent() const { return this->parent; }
        uhd::device_addr_t getUsrpAddress() const { return this->usrpAddress; }

        void setBlockIDMapping(const std::string &resourceID, const std::vector<uhd::rfnoc::block_id_t> &blockIDs);
        void setSetRxStreamer(const std::string &componentID, setStreamerCallback cb);
        void setSetTxStreamer(const std::string &componentID, setStreamerCallback cb);

    protected:
        typedef std::map<std::string, RFNoC_ResourceList *> RFNoC_ListMap;

        uhd::rfnoc::graph::sptr graph;
        RFNoC_ListMap idToList;
        Device_impl *parent;
        uhd::device3::sptr usrp;
        uhd::device_addr_t usrpAddress;
};

#endif
