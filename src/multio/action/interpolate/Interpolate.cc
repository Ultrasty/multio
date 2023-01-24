/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "multio/action/interpolate/Interpolate.h"

#include <fstream>
#include <type_traits>
#include <vector>

#include "eckit/exception/Exceptions.h"

#include "mir/api/MIRJob.h"
#include "mir/input/RawInput.h"
#include "mir/output/RawOutput.h"
#include "mir/param/SimpleParametrisation.h"

#include "multio/LibMultio.h"
#include "multio/util/PrecisionTag.h"


namespace multio::action::interpolate {


void fill(const eckit::LocalConfiguration& sub, mir::param::SimpleParametrisation& destination) {
    if (sub.has("loadFromFile")) {
        auto cfg = sub.getSubConfiguration("loadFromFile");

        for (const auto& key : cfg.keys()) {
            std::fstream inputFile;
            inputFile.open(cfg.getString(key));
            ASSERT(inputFile.is_open());

            std::vector<long> val;
            std::string str;
            while (getline(inputFile, str)) {
                val.push_back(long(atoi(str.c_str())));
            }
            destination.set(key, val);
        }
    };

    if (sub.has("setByValue")) {
        auto cfg = sub.getSubConfiguration("setByValue");

        for (const auto& key : cfg.keys()) {
            eckit::Value cfgVal = cfg.getSubConfiguration(key).get();
            if (cfgVal.isMap()) {
                throw eckit::NotImplemented("Action::Interpolate :: Nested forwarding is not supported", Here());
            }

            if (cfgVal.isList()) {
                if (cfgVal.head().isDouble()) {
                    destination.set(key, cfg.getDoubleVector(key));
                }
                else if (cfgVal.head().isNumber()) {
                    destination.set(key, cfg.getLongVector(key));
                }
                else if (cfgVal.head().isString()) {
                    destination.set(key, cfg.getStringVector(key));
                }
                else {
                    throw eckit::NotImplemented("Action::Interpolate :: Unsupported datatype", Here());
                };
            }
            else if (cfgVal.isBool()) {
                destination.set(key, cfg.getBool(key));
            }
            else if (cfgVal.isDouble()) {
                destination.set(key, cfg.getDouble(key));
            }
            else if (cfgVal.isNumber()) {
                destination.set(key, cfg.getInt(key));
            }
            else if (cfgVal.isString()) {
                destination.set(key, cfg.getString(key).c_str());
            }
            else {
                throw eckit::NotImplemented("Action::Interpolate :: Unsupported datatype", Here());
            }
        }
    };
};


template <typename A, typename B>
message::Message convert_precision(message::Message&& msg) {
    const size_t N = msg.payload().size() / sizeof(A);
    eckit::Buffer buffer(N * sizeof(B));

    auto md = msg.metadata();
    md.set("globalSize", buffer.size());
    md.set("precision", std::is_same<B, double>::value ? "double" : std::is_same<B, float>::value ? "single" : NOTIMP);

    const auto* a = reinterpret_cast<const A*>(msg.payload().data());
    auto* b = reinterpret_cast<B*>(buffer.data());
    for (size_t i = 0; i < N; ++i) {
        *(b++) = static_cast<B>(*(a++));
    }

    return {message::Message::Header{msg.tag(), msg.source(), msg.destination(), std::move(md)}, std::move(buffer)};
}


message::Message Interpolate::InterpolateInSinglePrecision(message::Message&& msg) const {
    // convert single/double precision, interpolate, convert double/single
    return convert_precision<double, float>(
        InterpolateInDoublePrecision(convert_precision<float, double>(std::move(msg))));
}


message::Message Interpolate::InterpolateInDoublePrecision(message::Message&& msg) const {
    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: Metadata of the input message :: " << std::endl
                             << msg.metadata() << std::endl
                             << std::endl;

    const auto& config = Action::confCtx_.config();

    const double* data = reinterpret_cast<const double*>(msg.payload().data());
    const size_t size = msg.payload().size() / sizeof(double);

    mir::param::SimpleParametrisation inputPar;
    fill(config.getSubConfiguration("inputConfiguration"), inputPar);

    mir::input::RawInput input(data, size, inputPar);

    mir::api::MIRJob job;
    fill(config.getSubConfiguration("jobConfiguration"), job);

    if (msg.metadata().has("missingValue")) {
        job.set("missing_value", msg.metadata().getDouble("missingValue"));
    }

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: input :: " << std::endl << inputPar << std::endl << std::endl;

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: job " << std::endl << job << std::endl << std::endl;

    std::vector<double> outData;
    mir::param::SimpleParametrisation outMetadata;
    mir::output::ResizableOutput output(outData, outMetadata);

    job.execute(input, output);

    message::Metadata md;
    md.set("globalSize", outData.size());
    md.set("precision", "double");

    eckit::Buffer buffer(reinterpret_cast<const char*>(outData.data()), outData.size() * sizeof(double));

    LOG_DEBUG_LIB(LibMultio) << "Interpolate :: Metadata of the output message :: " << std::endl
                             << md << std::endl
                             << std::endl;

    return {message::Message::Header{message::Message::Tag::Field, msg.source(), msg.destination(), std::move(md)},
            std::move(buffer)};
}


void Interpolate::executeImpl(message::Message msg) const {
    switch (msg.tag()) {
        case (message::Message::Tag::Field): {
            executeNext(util::dispatchPrecisionTag(msg.precision(), [&](auto pt) -> message::Message {
                using PT = typename decltype(pt)::type;
                if (std::is_same<double, PT>::value) {
                    return InterpolateInDoublePrecision(std::move(msg));
                }
                else if (std::is_same<float, PT>::value) {
                    return InterpolateInSinglePrecision(std::move(msg));
                }
                NOTIMP;
            }));
            break;
        };
        case (message::Message::Tag::StepComplete): {
            executeNext(msg);
            break;
        }
        default: {
            throw eckit::BadValue("Action::Interpolate :: Unsupported message tag", Here());
            break;
        }
    };
}


void Interpolate::print(std::ostream& os) const {
    os << "Interpolate";
}


static ActionBuilder<Interpolate> InterpolateBuilder("interpolate");


}  // namespace multio::action::interpolate
