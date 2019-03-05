
#ifndef multio_sandbox_PlanConfiguration_H
#define multio_sandbox_PlanConfiguration_H

#include <string>

namespace multio {
namespace sandbox {

std::string plan_configurations() {
    return R"json(
        {
           "transport" : "mpi",
           "domain" : "world",
           "plans" : [
              {
                 "name" : "ocean",
                 "actions" : {
                    "root" : {
                       "type" : "Print",
                       "stream" : "error",
                       "next" : {
                          "type" : "AppendToFile",
                          "path" : "messages.txt"
                       }
                    }
                 }
              },
              {
                 "name" : "atmosphere",
                 "actions" : {
                    "root" : {
                       "type" : "Select",
                       "categories" : [ "prognostic", "diagnostic" ],
                       "next" : {
                          "type" : "Aggregation",
                          "mapping" : "scattered",
                          "next" : {
                             "type" : "Encode",
                             "format" : "grib",
                             "next" : {
                                "type" : "Sink",
                                "datasink" : "file"
                             }
                          }
                       }
                    }
                 }
              }
           ]
        }
    )json";
}

}  // namespace sandbox
}  // namespace multio

#endif
