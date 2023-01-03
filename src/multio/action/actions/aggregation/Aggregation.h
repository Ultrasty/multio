/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Domokos Sarmany
/// @author Simon Smart
/// @author Tiago Quintino

/// @date Jan 2019

#pragma once

#include <iosfwd>
#include <unordered_map>

#include "multio/action/ChainedAction.h"
#include "multio/util/PrecisionTag.h"

namespace multio {
namespace action {

using message::Message;

class MessageMap : private std::map<std::string, Message> {
public:
    using std::map<std::string, Message>::at;
    using std::map<std::string, Message>::size;
    using std::map<std::string, Message>::begin;
    using std::map<std::string, Message>::end;

    bool contains(const std::string& key) const {
        return find(key) != end() && processedParts_.find(key) != std::end(processedParts_);
    }

    std::size_t partsCount(const std::string& key) const {
        ASSERT(contains(key));
        return processedParts_.at(key).size();
    }

    void bookProcessedPart(const std::string& key, message::Peer peer) {
        ASSERT(contains(key));
        auto ret = processedParts_.at(key).insert(std::move(peer));
        if (not ret.second) {
            eckit::Log::warning() << " Field " << key << " has been aggregated already" << std::endl;
        }
    }

    void addNew(const Message& msg) {
        ASSERT(not contains(msg.fieldId()));
        if (msg.metadata().has("precision")) {
            switch (multio::util::decodePrecisionTag(msg.metadata().getString("precision"))) {
                case multio::util::PrecisionTag::Float: {
                    emplace(msg.fieldId(),
                            Message{Message::Header{msg.header()}, eckit::Buffer{msg.globalSize() * sizeof(float)}});
                    processedParts_.emplace(msg.fieldId(), std::set<message::Peer>{});
                }; break;
                case multio::util::PrecisionTag::Double: {
                    emplace(msg.fieldId(),
                            Message{Message::Header{msg.header()}, eckit::Buffer{msg.globalSize() * sizeof(double)}});
                    processedParts_.emplace(msg.fieldId(), std::set<message::Peer>{});
                }; break;
                default:
                    throw eckit::BadValue("Action::Aggregation :: Unsupported datatype for input message",
                                          eckit::CodeLocation(__FILE__, __LINE__, __FUNCTION__));
            }
        }
        else {
            throw eckit::SeriousBug("Action::Aggregation :: Unable to find \"precision\" keyword in input metadata",
                                    eckit::CodeLocation(__FILE__, __LINE__, __FUNCTION__));
        }
    }

    void reset(const std::string& key) {
        erase(key);
        processedParts_.erase(key);
    }

private:
    std::map<std::string, std::set<message::Peer>> processedParts_;
};


class Aggregation : public ChainedAction {
public:
    explicit Aggregation(const ConfigurationContext& confCtx);

    void executeImpl(Message msg) const override;

private:
    void print(std::ostream& os) const override;

    bool handleField(const Message& msg) const;
    bool handleFlush(const Message& msg) const;

    Message createGlobalField(const Message& msg) const;
    bool allPartsArrived(const Message& msg) const;

    mutable MessageMap msgMap_;
    mutable std::map<std::string, unsigned int> flushes_;
};

}  // namespace action
}  // namespace multio
