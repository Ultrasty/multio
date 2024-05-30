
#pragma once

#define DEVELOPER_MODE 0


// Managing logging stuff
// In developer mode all possible logging information is active and redirected on std::err
// No matter if debug mode is active.
// With developer mode on and MULTIO_DEBUG=0 it is easier to spot the messages from only this action
#if DEVELOPER_MODE == 0
#define IFS_METADATA_MAPPING_OUT_STREAM LOG_DEBUG_LIB(LibMultio)
#else
#define IFS_METADATA_MAPPING_OUT_STREAM std::cerr << " + IFS_METADATA_MAPPING: "
#endif
