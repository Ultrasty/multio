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
/// @date   Apr 2020

#pragma once

#include <deque>


#include "multio/config/ComponentConfiguration.h"
#include "multio/maestro/CdoNamer.h"
#include "multio/maestro/MaestroCdo.h"
#include "multio/maestro/MaestroStatistics.h"
#include "multio/sink/DataSink.h"
#include "multio/util/Timing.h"


//--------------------------------------------------------------------------------------------------

namespace multio {

using config::ComponentConfiguration;

class MaestroSink : public multio::sink::DataSink {
public:
    MaestroSink(const ComponentConfiguration& config);
    ~MaestroSink() override;

private:
    void write(eckit::message::Message blob) override;

    void flush() override;

    void print(std::ostream&) const override;

    friend std::ostream& operator<<(std::ostream& s, const MaestroSink& p) {
        p.print(s);
        return s;
    }

    std::size_t cdoCount_ = 0;
    std::vector<MaestroCdo> offered_cdos_;

    CdoNamer cdo_namer_;
    util::Timing<> timing_;
    MaestroStatistics statistics_;

    bool readyCdoEnabled_ = true;
    MaestroCdo readyCdo_;
};

}  // namespace multio

//--------------------------------------------------------------------------------------------------
