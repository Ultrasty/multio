
#ifndef multio_server_actions_Period_H
#define multio_server_actions_Period_H

#include "eckit/types/DateTime.h"

namespace multio {
namespace action {

class DatePeriod {
public:
    DatePeriod(const eckit::Date& startPoint, long duration);

    void reset(const eckit::Date& startPoint);
    bool isWithin(const eckit::Date& dt);

private:
    eckit::Date endPoint() const;
    void print(std::ostream& os) const;

    eckit::Date startPoint_;
    long duration_;

    friend std::ostream& operator<<(std::ostream& os, const DatePeriod& a);

};

class DateTimePeriod {
public:
    DateTimePeriod(const eckit::DateTime& startPoint, eckit::Second duration);

    void reset(const eckit::DateTime& current);
    bool isWithin(const eckit::DateTime& dt);

private:
    eckit::DateTime startPoint_;
    const eckit::Second duration_;

    eckit::DateTime endPoint() const;
    void print(std::ostream& os) const;

    friend std::ostream& operator<<(std::ostream& os, const DateTimePeriod& a);
};

}
}  // namespace multio

#endif // multio_server_actions_Period_H
