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

#ifndef multio_sandbox_ThreadTransport_H
#define multio_sandbox_ThreadTransport_H

#include <map>
#include <mutex>
#include <thread>

#include "eckit/container/Queue.h"

#include "sandbox/Transport.h"

namespace multio {
namespace sandbox {

class ThreadTransport final : public Transport {
public:
    ThreadTransport(const eckit::Configuration& config);
    ~ThreadTransport() override;

private:
    Message receive() override;

    void send(const Message& message) override;

    void print(std::ostream& os) const override;

    Peer localPeer() const override;

    eckit::Queue<Message>& receiveQueue(Peer to);

    std::map<Peer, std::unique_ptr<eckit::Queue<Message>>> queues_;

    std::mutex mutex_;

    size_t messageQueueSize_;
};

}  // namespace sandbox
}  // namespace multio

#endif
