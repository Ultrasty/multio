/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <unistd.h>
#include <cstring>

#include "eckit/testing/Test.h"
#include "eckit/filesystem/TmpFile.h"
#include "multio/DataSink.h"
#include "multio/FileSink.h"

#include "TestHelpers.h"

namespace multio {
namespace test {

CASE("test_contains_file_sink") {
    // DataSinkFactory::list appends the results to a ostream&, so we need to extract them.
    std::stringstream ss;
    DataSinkFactory::instance().list(ss);
    EXPECT(ss.str().find("file") != std::string::npos);
}

CASE("test_file_sink_is_created_successfully") {
    eckit::PathName file_path = eckit::TmpFile();
    auto sink = make_configured_file_sink(file_path);
    auto fileSink = dynamic_cast<FileSink*>(sink.get());
    EXPECT(fileSink);
    EXPECT(file_path.exists());
    file_path.unlink();
}

CASE("test_file_sink_writes_correctly") {
    eckit::PathName file_path = eckit::TmpFile();
    auto sink = make_configured_file_sink(file_path);
    const char quote[] =
        "All was quiet in the deep dark wood. The mouse found a nut and the nut was good.";

    eckit::DataBlobPtr stringBlob(
        eckit::DataBlobFactory::build("test_blob", quote, sizeof(quote) - 1));
    sink->write(stringBlob);

    EXPECT(file_content(file_path) == std::string(quote));
    file_path.unlink();
}

}  // namespace test
}  // namespace multio

int main(int argc, char** argv) {
    return eckit::testing::run_tests(argc, argv);
}
