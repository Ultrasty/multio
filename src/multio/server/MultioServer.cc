
#include "MultioServer.h"

#include <fstream>
#include <iomanip>

#include "eckit/config/YAMLConfiguration.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/log/Statistics.h"
#include "eckit/types/DateTime.h"

#include "multio/LibMultio.h"
#include "multio/transport/Transport.h"
#include "multio/util/logfile_name.h"

namespace multio {
namespace server {

using transport::TransportFactory;

MultioServer::MultioServer(const eckit::Configuration& config) :
    transport_{TransportFactory::instance().build(config.getString("transport"), config)},
    listener_{config, *transport_} {
    LOG_DEBUG_LIB(multio::LibMultio) << "Server config: " << config << std::endl;

    std::ofstream logFile{util::logfile_name(), std::ios_base::app};

    struct ::timeval tstamp;
    ::gettimeofday(&tstamp, 0);
    auto mSecs = tstamp.tv_usec;

    logFile << "MultioServer starts at "
            << eckit::DateTime{static_cast<double>(tstamp.tv_sec)}.time().now() << ":"
            << std::setw(6) << std::setfill('0') << mSecs << " -- ";


    listener_.start();
    eckit::Log::info() << "Listening loop has stopped" << std::endl;
}

MultioServer::~MultioServer() {
    std::ofstream logFile{util::logfile_name(), std::ios_base::app};

    struct ::timeval tstamp;
    ::gettimeofday(&tstamp, 0);
    auto mSecs = tstamp.tv_usec;

    logFile << "MultioServer stops at "
            << eckit::DateTime{static_cast<double>(tstamp.tv_sec)}.time().now() << ":"
            << std::setw(6) << std::setfill('0') << mSecs << std::endl;

}

}  // namespace server
}  // namespace multio
