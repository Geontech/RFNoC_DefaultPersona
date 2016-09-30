#ifndef RFNOC_DEFAULTPERSONA_ENTRY_POINTS_H
#define RFNOC_DEFAULTPERSONA_ENTRY_POINTS_H

#include <uhd/usrp/multi_usrp.hpp>

// ************* AGREED UPON METHOD TO INSTANTIATE DEVICE FROM SHARED OBJECT *************
//
//   The following typedef defines how the shared object should be constructed.  Any
//   additional parameters may be passed in as long as each persona device supports the
//   construct method signature.  Any updates to the ConstructorPtr typedef requires a
//   change to the 'generateResource' method to pass in the additional parameters
//   defined in the 'ConstructorPtr' typedef.
//
//     IE: If a specific api/interface object is to be shared with each persona device,
//         the following changes are required
//
//          Within this header file:
//          typedef void* (*ConstructorPtr)(int, char*[], SharedAPIObject*)
//
//          Within this cpp file:
//          void* personaPtr = construct(argc, argv, SharedAPIObject);
//
typedef Resource_impl* (*ConstructorPtr)(int, char*[], Device_impl* parentDevice, uhd::usrp::multi_usrp::sptr usrp);

#endif // RFNOC_DEFAULTPERSONA_ENTRY_POINTS_H
