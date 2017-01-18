/*
 * RFNoC_ResourceManager.h
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#ifndef RFNOC_RESOURCELIST_H
#define RFNOC_RESOURCELIST_H

#include <ossie/debug.h>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/graph.hpp>

#include "RFNoC_Resource.h"

// Forward declarations of other classes
class RFNoC_Resource;
class RFNoC_ResourceManager;

class RFNoC_ResourceList
{
    ENABLE_LOGGING
    public:
        RFNoC_ResourceList(RFNoC_ResourceManager *resourceManager, uhd::rfnoc::graph::sptr graph);
        RFNoC_ResourceList(RFNoC_ResourceList &that);
        ~RFNoC_ResourceList();

        RFNoC_ResourceList& operator=(const RFNoC_ResourceList &that);

        Resource_impl* addResource(int argc, char* argv[], ConstructorPtr fnptr, const char* libraryName, std::string resourceID);
        bool connect(RFNoC_ResourceList &providesList);
        bool empty();
        std::vector<std::string> getIDs();
        RFNoC_Resource* getProvidesResource() const;
        RFNoC_Resource* getUsesResource() const;
        bool hasHash(const CORBA::ULong &hash) const;
        bool hasResource(const RFNoC_Resource *resource);
        void merge(RFNoC_ResourceList *providesList);
        void removeResource(const std::string &resourceID);
        bool update();

        void setBlockInfoMapping(const std::string &resourceID, const std::vector<BlockInfo> &blockIDs);
        void setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb);
        void setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb);

    protected:
        typedef std::map<std::string, RFNoC_Resource *> RFNoC_ResourceMap;

        uhd::rfnoc::graph::sptr graph;
        RFNoC_ResourceMap idToResource;
        std::list<RFNoC_Resource *> resourceList;
        RFNoC_ResourceManager *resourceManager;
};

#endif
