/*
 * RFNoC_ResourceManager.h
 *
 *  Created on: Nov 7, 2016
 *      Author: Patrick
 */

#ifndef RFNOC_RESOURCELIST_H
#define RFNOC_RESOURCELIST_H

/*#include <ossie/debug.h>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/graph.hpp>*/

#include <uhd/rfnoc/graph.hpp>

#include "RFNoC_Resource.h"

class RFNoC_ResourceList
{
    ENABLE_LOGGING
    public:
        RFNoC_ResourceList(uhd::rfnoc::graph::sptr graph);
        ~RFNoC_ResourceList();

        std::string addResource(Resource_impl *rhResource);
        bool connect(RFNoC_ResourceList &providesList);
        bool empty();
        std::vector<std::string> getIDs();
        RFNoC_Resource* getProvidesResource() const;
        RFNoC_Resource* getUsesResource() const;
        bool hasResource(const RFNoC_Resource *resource);
        void merge(RFNoC_ResourceList *providesList);
        void removeResource(const std::string &resourceID);
        bool update();

        void setBlockIDMapping(const std::string &resourceID, const std::vector<uhd::rfnoc::block_id_t> &blockIDs);
        void setSetRxStreamer(const std::string &resourceID, setStreamerCallback cb);
        void setSetTxStreamer(const std::string &resourceID, setStreamerCallback cb);

    protected:
        typedef std::map<std::string, RFNoC_Resource *> RFNoC_ResourceMap;

        uhd::rfnoc::graph::sptr graph;
        RFNoC_ResourceMap idToResource;
        std::list<RFNoC_Resource *> resourceList;
};

#endif
