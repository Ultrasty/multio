
#include "multio/message/MetadataMatcher.h"

#include "eckit/config/LocalConfiguration.h"
#include "eckit/value/Value.h"  // Remove once config visitor is implemented

#include <sstream>

#include <sstream>

using eckit::LocalConfiguration;

namespace multio::message::match {

//--------------------------------------------------------------------------------------------------

MatchKeys::MatchKeys(const LocalConfiguration& cfg, Predicate p) {
    std::map<typename MetadataTypes::KeyType, std::unordered_set<MetadataValue>> matcher;

    for (const auto& k : cfg.keys()) {
        // TODO Use config visitor once added to eckit
        eckit::LocalConfiguration cfgK;
        cfg.get(k, cfgK);
        if (cfgK.get().isList()) {
            auto v = cfg.getSubConfigurations(k);
            std::unordered_set<MetadataValue> s;
            unsigned int i = 0;
            for (auto& vi : v) {
                auto optMetadataValue = tryToMetadataValue(vi.get());
                if (!optMetadataValue) {
                    std::ostringstream oss;
                    oss << "Matcher for key \"" << k << "\"[" << i
                        << "] can not be  represented by an internal metadata value: " << vi.get();
                    throw MetadataException(oss.str());
                }
                s.emplace(std::move(*optMetadataValue));
                ++i;
            }
            matcher.emplace(k, std::move(s));
        }
        else {
            auto optMetadataValue = tryToMetadataValue(cfgK.get());
            if (!optMetadataValue) {
                std::ostringstream oss;
                oss << "Matcher for key \"" << k
                    << "\" can not be represented by an internal metadata value: " << cfgK.get();
                throw MetadataException(oss.str());
            }
            matcher.emplace(k, std::unordered_set<MetadataValue>{std::move(*optMetadataValue)});
        }
    }

    // Now copy to vector that will get iteratied in future
    matcher_.reserve(matcher.size());
    for (auto&& kv : std::move(matcher)) {
        matcher_.push_back(std::move(kv));
    }

    predicate_ = p;
}


bool MatchKeys::matches(const Metadata& md) const {
    bool res = true;

    for (const auto& kv : matcher_) {
        auto searchKey = md.find(kv.first);
        if (searchKey == md.end()) {
            res = false;
            break;
        }
        if (kv.second.find(searchKey->second) == kv.second.end()) {
            res = false;
            break;
        }
    }

    if (predicate_ == Predicate::Negate) {
        return !res;
    }
    else {
        return res;
    }
}

namespace {

Predicate negatePredicate(Predicate p) {
    switch (p) {
        case Predicate::None:
            return Predicate::Negate;
        default:
            return Predicate::None;
    }
}

}  // namespace

void MatchKeys::negate() {
    predicate_ = negatePredicate(predicate_);
}

void MatchKeys::applyPredicate(Predicate p) {
    if (p == Predicate::Negate) {
        this->negate();
    }
}


void MatchKeys::print(std::ostream& os) const {
    if (predicate_ == Predicate::Negate) {
        os << "!(";
    }
    os << "{";
    bool isFirst = true;
    for (const auto& kv : matcher_) {
        if (!isFirst) {
            os << " ,";
        }
        else {
            isFirst = false;
        }
        os << kv.first.value() << " => {";

        bool isFirst2 = true;
        for (const auto& v : kv.second) {
            if (!isFirst2) {
                os << " ,";
            }
            else {
                isFirst2 = false;
            }
            os << v;
        }
        os << "}";
    }
    os << "}";
    if (predicate_ == Predicate::Negate) {
        os << ")";
    }
}

//--------------------------------------------------------------------------------------------------


namespace {

Predicate invert(Predicate p) {
    switch (p) {
        case Predicate::None:
            return Predicate::Negate;
        default:
            return Predicate::None;
    }
}

std::variant<MatchKeys, MatchReduce> constructMatchIgnore(const LocalConfiguration& cfg, const std::string& key,
                                                          Predicate p) {
    eckit::LocalConfiguration cfgK;
    cfg.get(key, cfgK);

    if (cfgK.get().isMap()) {
        return MatchKeys{cfgK, p};
    }
    else if (cfgK.get().isList()) {
        auto v = cfg.getSubConfigurations(key);

        if (v.size() == 1) {
            return MatchKeys{v[0], p};
        }
        else {
            MatchReduce res(Reduce::Or, p);
            std::size_t i = 0;
            for (auto& vi : v) {

                if (!vi.get().isMap()) {
                    std::ostringstream oss;
                    oss << "MetadataMatcher: The block for \"" << key
                        << "\" is expected to be a map or list of maps. This one is a list, but the " << i
                        << " element is no map: " << cfg;
                    throw MetadataException(oss.str(), Here());
                }
                res.extend(MatchKeys{vi});
                ++i;
            }
            return res;
        }
    }
    else {
        std::ostringstream oss;
        oss << "MetadataMatcher: The block for \"" << key
            << "\" is expected to be a map or list of maps. This one is something else: " << cfg;
        throw MetadataException(oss.str(), Here());
    }
}

}  // namespace

MatchReduce MatchReduce::construct(const LocalConfiguration& cfg, Predicate p) {
    if (!cfg.get().isMap()) {
        std::ostringstream oss;
        oss << "MetadataMatcher: Expected a map: " << cfg;
        throw MetadataException(oss.str(), Here());
    }

    bool hasAny = cfg.has("any");
    bool hasAll = cfg.has("all");
    bool hasMatch = cfg.has("match");
    bool hasIgnore = cfg.has("ignore");
    bool hasMatchOrIgnore = hasMatch || hasIgnore;
    bool hasNot = cfg.has("not");

    int checkKeySum = ((int)hasAny + (int)hasAll + (int)hasNot + (int)hasMatchOrIgnore);

    if (checkKeySum > 1) {
        std::ostringstream oss;
        oss << "MetadataMatcher: can only have either \"any\", \"all\", \"not\" or directly a \"match\"/\"ignore\" "
               "configuration but not their combination: "
            << cfg;
        throw MetadataException(oss.str(), Here());
    }
    if (checkKeySum == 0) {
        std::ostringstream oss;
        oss << "MetadataMatcher: Require  a \"any\", \"all\", \"not\", \"match\" or \"ignore\" configuration block: "
            << cfg;
        throw MetadataException(oss.str(), Here());
    }

    if (hasNot) {
        return construct(cfg.getSubConfiguration("not"), invert(p));
    }
    else if (hasMatchOrIgnore) {
        if (hasMatch && hasIgnore) {
            MatchReduce res{Reduce::And, p};
            std::visit([&](auto&& v) { res.extend(std::move(v)); },
                       constructMatchIgnore(cfg, "match", Predicate::None));
            std::visit([&](auto&& v) { res.extend(std::move(v)); },
                       constructMatchIgnore(cfg, "ignore", Predicate::Negate));
            return res;
        }
        else {
            return std::visit(
                eckit::Overloaded{[&](MatchKeys&& mk) -> MatchReduce {
                                      MatchReduce res{Reduce::Or, Predicate::None};
                                      res.extend(std::move(mk));
                                      return res;
                                  },
                                  [&](MatchReduce&& mr) -> MatchReduce { return mr; }},
                constructMatchIgnore(cfg, hasMatch ? "match" : "ignore", hasMatch ? p : negatePredicate(p)));
        }
    }
    else {
        // all or any
        MatchReduce res{hasAll ? Reduce::And : Reduce::Or, p};
        std::string key = hasAll ? "all" : "any";

        eckit::LocalConfiguration cfgK;
        cfg.get(key, cfgK);
        if (!cfgK.get().isList()) {
            std::ostringstream oss;
            oss << "MetadataMatcher: The block for \"" << key << "\" is expected to be a list of maps: " << cfg;
            throw MetadataException(oss.str(), Here());
        }

        auto v = cfg.getSubConfigurations(key);
        std::size_t i = 0;
        for (auto& vi : v) {
            if (!vi.get().isMap()) {
                std::ostringstream oss;
                oss << "MetadataMatcher: The block for \"" << key
                    << "\" is expected to be a list of maps. This one is a list, but the " << i
                    << " element is not a map: " << cfg;
                throw MetadataException(oss.str(), Here());
            }
            res.extend(construct(vi, Predicate::None));
            ++i;
        }

        return res;
    }
}

MatchReduce::MatchReduce(Reduce r, Predicate p) : reduce_{r}, predicate_{p} {}

MatchReduce::MatchReduce(const eckit::LocalConfiguration& cfg, Predicate p) :
    MatchReduce{MatchReduce::construct(cfg, p)} {}

namespace {

bool matchesVariant(typename MatchReduce::Elem const& matcher, const Metadata& md) {
    return std::visit(eckit::Overloaded{[&](const MatchKeys& mk) { return mk.matches(md); },
                                        [&](const std::shared_ptr<MatchReduce>& mr) { return mr->matches(md); }},
                      matcher);
}

}  // namespace

bool MatchReduce::matches(const Metadata& md) const {
    bool res;
    if (reduce_ == Reduce::Or) {
        // Any: Reduce or
        res = false;
        for (const auto& matcher : matchers_) {
            if (matchesVariant(matcher, md)) {
                res = true;
                break;
            }
        }
    }
    else {
        // All: Reduce and
        res = true;
        for (const auto& matcher : matchers_) {
            if (!matchesVariant(matcher, md)) {
                res = false;
                break;
            }
        }
    }

    if (predicate_ == Predicate::Negate) {
        return !res;
    }
    else {
        return res;
    }
}

bool MatchReduce::isEmpty() const {
    return matchers_.empty();
}

void MatchReduce::extend(const MatchKeys& mk) {
    matchers_.push_back(mk);
}

void MatchReduce::extend(MatchKeys&& mk) {
    matchers_.push_back(std::move(mk));
}

void MatchReduce::extend(const MatchReduce& mr) {
    matchers_.push_back(std::make_shared<MatchReduce>(mr));
}

void MatchReduce::extend(MatchReduce&& mr) {
    matchers_.push_back(std::make_shared<MatchReduce>(std::move(mr)));
}

void MatchReduce::extend(typename MatchReduce::Elem const& e) {
    matchers_.push_back(e);
}

void MatchReduce::extend(typename MatchReduce::Elem&& e) {
    matchers_.push_back(std::move(e));
}


void MatchReduce::print(std::ostream& os) const {
    os << "MatchReduce(" << (reduce_ == Reduce::And ? "&&" : "||") << ", "
       << (predicate_ == Predicate::None ? "+" : "-") << ", [";
    bool first = true;
    for (const auto& elem : matchers_) {
        if (!first) {
            os << ", ";
        }
        first = false;

        std::visit(eckit::Overloaded{[&](const MatchKeys& mk) { os << mk; },
                                     [&](const std::shared_ptr<MatchReduce>& mr) { os << *mr.get(); }},
                   elem);
    }
    os << "])";
}

void MatchReduce::negate() {
    predicate_ = negatePredicate(predicate_);
}

void MatchReduce::applyPredicate(Predicate p) {
    if (p == Predicate::Negate) {
        this->negate();
    }
}

//--------------------------------------------------------------------------------------------------

}  // namespace multio::message::match
