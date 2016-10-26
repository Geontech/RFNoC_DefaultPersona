#ifndef RFNOCCOMPONENT_H
#define RFNOCCOMPONENT_H

#include <boost/bind.hpp>
#include <boost/function.hpp>

typedef boost::function<void(bool shouldStream)> setStreamerCallback;

typedef boost::function<void(const std::string &componentID, setStreamerCallback setStreamCb)> setSetStreamerCallback;

typedef boost::function<void(const std::string &componentID, const std::string &blockID)> blockIDCallback;

#endif