/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Philipp Geier

/// @date Aug 2022

#ifndef multio_util_ConfigurationContext_H
#define multio_util_ConfigurationContext_H

#include <tuple>
#include "eckit/config/LocalConfiguration.h"
#include "eckit/utils/Optional.h"
#include "multio/util/ConfigurationPath.h"
#include "multio/util/IteratorMapper.h"

namespace multio {
namespace util {


enum class LocalPeerTag : unsigned
{
    Client = 0,
    Server = 1,
};

class SubContextIteratorMapper;

// Explicitly create specializations for compile time checks
class ServerConfigurationContext;
class ClientConfigurationContext;

class GlobalConfCtx; // TODO: hide internal implementation in separate namespace
using MPIParentCommInfo = eckit::Optional<std::tuple<std::string, int>>;

class ConfigurationContext {
public:
    ConfigurationContext(const eckit::PathName& pathName, const eckit::PathName& fileName,
                         LocalPeerTag clientOrServer = LocalPeerTag::Client);
    ConfigurationContext(const eckit::LocalConfiguration& config, const eckit::PathName& pathName,
                         const eckit::PathName& fileName,
                         LocalPeerTag clientOrServer = LocalPeerTag::Client);
    ConfigurationContext(const eckit::LocalConfiguration& config,
                         const eckit::LocalConfiguration& globalConfig,
                         const eckit::PathName& pathName, const eckit::PathName& fileName,
                         LocalPeerTag clientOrServer = LocalPeerTag::Client);
                         
    eckit::LocalConfiguration& config();
    const eckit::LocalConfiguration& config() const;
    const eckit::LocalConfiguration& globalConfig() const;
    const eckit::PathName& pathName() const;
    const eckit::PathName& fileName() const;

    LocalPeerTag localPeerTag() const;
    inline bool isServer() const;
    inline bool isClient() const;

    ConfigurationContext& setLocalPeerTag(LocalPeerTag clientOrServer);
    ConfigurationContext& tagServer();
    ConfigurationContext& tagClient();

    using SubConfigurationContexts =
        MappedContainer<std::vector<eckit::LocalConfiguration>, SubContextIteratorMapper>;

    ConfigurationContext subContext(const std::string& subConfiguratinKey) const;
    SubConfigurationContexts subContexts(const std::string& subConfiguratinKey) const;
    ConfigurationContext recast(const eckit::LocalConfiguration& config) const;

    const MPIParentCommInfo& getMPIParentCommInfo() const;
    ConfigurationContext& setMPIParentCommInfo(const MPIParentCommInfo& val);

protected:
    ConfigurationContext(const eckit::LocalConfiguration& config,
                         std::shared_ptr<GlobalConfCtx> globalConfCtx);


private:
    eckit::LocalConfiguration config_;
    std::shared_ptr<GlobalConfCtx> globalConfCtx_;

    friend class SubContextIteratorMapper;
};


class GlobalConfCtx {
protected:
    GlobalConfCtx(const eckit::PathName& pathName, const eckit::PathName& fileName,
                  LocalPeerTag clientOrServer = LocalPeerTag::Client);
    GlobalConfCtx(const eckit::LocalConfiguration& config, const eckit::PathName& pathName,
                  const eckit::PathName& fileName,
                  LocalPeerTag clientOrServer = LocalPeerTag::Client);

    const eckit::LocalConfiguration& globalConfig() const;
    const eckit::PathName& pathName() const;
    const eckit::PathName& fileName() const;

    LocalPeerTag localPeerTag() const;
    void setLocalPeerTag(LocalPeerTag clientOrServer);

    const MPIParentCommInfo& getMPIParentCommInfo() const;
    void setMPIParentCommInfo(const MPIParentCommInfo& val);


private:
    eckit::LocalConfiguration globalConfig_;
    eckit::PathName pathName_;
    eckit::PathName fileName_;
    LocalPeerTag localPeerTag_;

    MPIParentCommInfo mpiParentCommInfo_;

    friend class ConfigurationContext;
};


class SubContextIteratorMapper {
public:
    SubContextIteratorMapper(const ConfigurationContext& confCtx);
    SubContextIteratorMapper(ConfigurationContext&& confCtx);

    ConfigurationContext operator()(const eckit::LocalConfiguration& config) const;

private:
    ConfigurationContext confCtx_;
};


class ClientConfigurationContext : public ConfigurationContext {
public:
    ClientConfigurationContext(const ConfigurationContext& otherBase) :
        ConfigurationContext(otherBase) {
        tagClient();
    }
    ClientConfigurationContext(ConfigurationContext&& otherBase) :
        ConfigurationContext(std::move(otherBase)) {
        tagClient();
    }
    ClientConfigurationContext(const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(pathName, fileName, LocalPeerTag::Client){};
    ClientConfigurationContext(const eckit::LocalConfiguration& config,
                               const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(config, pathName, fileName, LocalPeerTag::Client){};
    ClientConfigurationContext(const eckit::LocalConfiguration& config,
                               const eckit::LocalConfiguration& globalConfig,
                               const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(config, globalConfig, pathName, fileName, LocalPeerTag::Client){};
};


class ServerConfigurationContext : public ConfigurationContext {
public:
    ServerConfigurationContext(const ConfigurationContext& otherBase) :
        ConfigurationContext(otherBase) {
        tagServer();
    }
    ServerConfigurationContext(ConfigurationContext&& otherBase) :
        ConfigurationContext(std::move(otherBase)) {
        tagServer();
    }
    ServerConfigurationContext(const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(pathName, fileName, LocalPeerTag::Server){};
    ServerConfigurationContext(const eckit::LocalConfiguration& config,
                               const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(config, pathName, fileName, LocalPeerTag::Server){};
    ServerConfigurationContext(const eckit::LocalConfiguration& config,
                               const eckit::LocalConfiguration& globalConfig,
                               const eckit::PathName& pathName, const eckit::PathName& fileName) :
        ConfigurationContext(config, globalConfig, pathName, fileName, LocalPeerTag::Server){};
};


}  // namespace util
}  // namespace multio

#endif
