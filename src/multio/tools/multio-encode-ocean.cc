
#include <fstream>
#include <iomanip>

#include "eccodes.h"

#include "eckit/config/YAMLConfiguration.h"
#include "eckit/io/StdFile.h"
#include "eckit/option/CmdArgs.h"
#include "eckit/option/SimpleOption.h"

#include "multio/LibMultio.h"
#include "multio/action/Plan.h"
#include "multio/config/ComponentConfiguration.h"
#include "multio/config/PathConfiguration.h"
#include "multio/message/Message.h"
#include "multio/tools/MultioTool.h"

using multio::action::Plan;
using multio::config::ComponentConfiguration;
using multio::config::configuration_path_name;
using multio::config::MultioConfiguration;
using multio::message::Message;
using multio::message::Peer;

//----------------------------------------------------------------------------------------------------------------

namespace {

struct codes_handle_deleter {
    void operator()(codes_handle* h) { codes_handle_delete(h); }
};

struct codes_iterator_deleter {
    void operator()(codes_keys_iterator* it) { codes_keys_iterator_delete(it); }
};

void codes_check_set_string(codes_handle* handle, const std::string& key, const std::string& val) {
    auto size = val.size();
    CODES_CHECK(codes_set_string(handle, key.c_str(), val.c_str(), &size), nullptr);
}

void codes_set_latlon_dimensions(codes_handle* handle, const std::vector<int>& grid_def) {
    long ret_val = -1;
    CODES_CHECK(codes_get_long(handle, "gridDefinitionTemplateNumber", &ret_val), nullptr);
    if (ret_val == 0) {  // Only set it for lat-lon; no meaning for unstructured
        CODES_CHECK(codes_set_long(handle, "Ni", grid_def[0]), nullptr);
        CODES_CHECK(codes_set_long(handle, "Nj", grid_def[1]), nullptr);
    }
}

MultioConfiguration test_configuration() {
    return MultioConfiguration(configuration_path_name("") + "test-ocean-config.yaml");
}

}  // namespace

class MultioEncodeOcean final : public multio::MultioTool {
public:
    MultioEncodeOcean(int argc, char** artv);

private:
    void usage(const std::string& tool) const override {
        eckit::Log::info() << std::endl << "Usage: " << tool << " [options]" << std::endl;
    }

    void init(const eckit::option::CmdArgs& args) override;

    void finish(const eckit::option::CmdArgs& args) override;

    void execute(const eckit::option::CmdArgs& args) override;

    codes_handle* handle() { return handle_.get(); }
    std::unique_ptr<codes_handle, codes_handle_deleter> handle_ = nullptr;

    void setCommonMetadata();

    using NemoKey = std::string;
    struct GribData {
        long param;
        std::string gridType;
    };

    void setDomainDimensions(const GribData& gd);
    std::vector<int> readGrid(const std::string& grid_type, size_t client_id);
    void setFieldValues(const NemoKey& key);
    void executePlan();
    void printNamespace(const std::string& ns);

    std::string template_ = "GRIB1";
    std::string pathToNemoData_ = "";
    int globalSize_ = 105704;
    int level_ = 1;
    int step_ = 24;

    std::map<NemoKey, GribData> parameters_
        = {{"sst", {151129, "T grid"}}, {"ssu", {151131, "U grid"}}, {"ssv", {151132, "V grid"}}};
};

//----------------------------------------------------------------------------------------------------------------

MultioEncodeOcean::MultioEncodeOcean(int argc, char** argv) : multio::MultioTool(argc, argv) {
    options_.push_back(new eckit::option::SimpleOption<std::string>("template", "Name of grib template"));
    options_.push_back(new eckit::option::SimpleOption<std::string>("path", "Path to NEMO data"));
}

void MultioEncodeOcean::init(const eckit::option::CmdArgs& args) {
    eckit::AutoStdFile fin{configuration_path_name("") + args(0)};

    int err;
    handle_.reset(codes_handle_new_from_file(nullptr, fin, PRODUCT_GRIB, &err));
    ASSERT(handle_);

    args.get("template", template_);
    args.get("path", pathToNemoData_);
    args.get("step", step_);
}

void MultioEncodeOcean::finish(const eckit::option::CmdArgs&) {}

void MultioEncodeOcean::execute(const eckit::option::CmdArgs&) {
    setCommonMetadata();

    for (const auto& prm : parameters_) {
        setDomainDimensions(prm.second);
        setFieldValues(prm.first);
        executePlan();
    }

    printNamespace("mars");
}

void MultioEncodeOcean::setCommonMetadata() {
    codes_check_set_string(handle(), "expver", "xxxx");
    codes_check_set_string(handle(), "class", "rd");
    codes_check_set_string(handle(), "stream", "oper");
    codes_check_set_string(handle(), "type", "fc");
    CODES_CHECK(codes_set_long(handle(), "step", step_), nullptr);
    CODES_CHECK(codes_set_long(handle(), "level", level_), nullptr);
}

void MultioEncodeOcean::setDomainDimensions(const GribData& gd) {
    auto grid_def = readGrid(gd.gridType.substr(5), 0);
    codes_set_latlon_dimensions(handle(), grid_def);

    auto globalSize_ = grid_def[0] * grid_def[1];
    CODES_CHECK(codes_set_long(handle(), "numberOfDataPoints", globalSize_), nullptr);
    CODES_CHECK(codes_set_long(handle(), "numberOfValues", globalSize_), nullptr);

    CODES_CHECK(codes_set_long(handle(), "param", gd.param), nullptr);
}

std::vector<int> MultioEncodeOcean::readGrid(const std::string& grid_type, size_t client_id) {
    std::ostringstream oss;
    oss << grid_type << "_" << std::setfill('0') << std::setw(2) << client_id;

    auto grid = eckit::PathName{pathToNemoData_ + oss.str()};

    std::ifstream infile(std::string{grid.fullName()}.c_str());

    std::string gtype;
    infile >> gtype;
    if (gtype != grid_type) {
        throw eckit::SeriousBug("Wrong grid is being read");
    }

    std::vector<int> domain_dims;
    for (int next; infile >> next;) {
        domain_dims.push_back(next);
    }

    ASSERT(globalSize_ == (domain_dims[0] * domain_dims[1]));

    return domain_dims;
}

void MultioEncodeOcean::setFieldValues(const NemoKey& key) {
    auto field_path = eckit::PathName{std::string{pathToNemoData_ + key + "_" + std::to_string(step_) + "_reference"}};

    std::ifstream infile{std::string{field_path.fullName()}.c_str()};

    std::string str{std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>()};
    auto beg = reinterpret_cast<const double*>(str.data());
    std::vector<double> values{beg, beg + str.size() / sizeof(double)};

    CODES_CHECK(codes_set_double_array(handle(), "values", values.data(), values.size()), nullptr);
}

void MultioEncodeOcean::executePlan() {
    const char* buf = nullptr;
    size_t sz = 0;
    CODES_CHECK(codes_get_message(handle(), reinterpret_cast<const void**>(&buf), &sz), nullptr);

    auto multioConfig = test_configuration();
    std::vector<std::unique_ptr<Plan>> plans
        = multio::action::Plan::makePlans(multioConfig.parsedConfig().getSubConfigurations("plans"), multioConfig);
    Message msg{Message::Header{Message::Tag::Field, Peer{"", 0}, Peer{"", 0}}, eckit::Buffer{buf, sz}};
    eckit::Log::debug<multio::LibMultio>() << "Message size: " << msg.size() << std::endl;

    for (const auto& plan : plans) {
        plan->process(msg);
    }
}

void MultioEncodeOcean::printNamespace(const std::string& ns) {
    std::unique_ptr<codes_keys_iterator, codes_iterator_deleter> iter{codes_keys_iterator_new(handle(), 0, ns.c_str())};
    while (codes_keys_iterator_next(iter.get())) {
        auto keyname = codes_keys_iterator_get_name(iter.get());
        char keyval[1024];
        size_t keylen = sizeof(keyval);
        codes_get_string(handle(), keyname, keyval, &keylen);
        eckit::Log::debug<multio::LibMultio>() << "=== " << keyname << ":   " << keyval << std::endl;
    }
}


//----------------------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    MultioEncodeOcean tool(argc, argv);
    return tool.start();
}
