
#include "MultioServer.h"

#include <fstream>
#include <iomanip>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/log/Statistics.h"
#include "eckit/types/DateTime.h"

#include "multio/LibMultio.h"
#include "multio/transport/Transport.h"

namespace multio::server {

using config::ComponentConfiguration;
using transport::TransportFactory;


namespace {

eckit::LocalConfiguration getServerConf(const MultioConfiguration& multioConf) {
    if (multioConf.parsedConfig().has("server")) {
        return multioConf.parsedConfig().getSubConfiguration("server");
    }

    std::ostringstream oss;
    oss << "Configuration 'server' not found in configuration file " << multioConf.configFile();
    throw eckit::UserError(oss.str());
}

}  // namespace

MultioServer::MultioServer(const eckit::LocalConfiguration& conf, MultioConfiguration&& multioConf) :
    MultioConfigurationHolder(std::move(multioConf), config::LocalPeerTag::Server),
    FailureAware(config::ComponentConfiguration(conf, multioConfig())),
    transport_{
        TransportFactory::instance().build(conf.getString("transport"), ComponentConfiguration(conf, multioConfig()))},
    listener_{ComponentConfiguration(conf, multioConfig()), *transport_} {
    LOG_DEBUG_LIB(multio::LibMultio) << "Server config: " << conf << std::endl;

    eckit::Log::info() << "Server start listening..." << std::endl;
    withFailureHandling([&]() { listener_.start(); });
    eckit::Log::info() << "Listening loop has stopped" << std::endl;
}

MultioServer::MultioServer(MultioConfiguration&& multioConf) :
    MultioServer(getServerConf(multioConf), std::move(multioConf)) {}

MultioServer::~MultioServer() = default;

util::FailureHandlerResponse MultioServer::handleFailure(util::OnServerError t, const util::FailureContext& c,
                                                         util::DefaultFailureState&) const {
    // Last cascading instace - print nested contexts
    eckit::Log::error() << c << std::endl;
    eckit::Log::flush();

    if (t == util::OnServerError::AbortTransport) {
        transport_->abort(c.eptr);
    }
    return util::FailureHandlerResponse::Rethrow;
};

}  // namespace multio::server
