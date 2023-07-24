#pragma once

#include <memory>
#include <variant>

#include "TypeTraits.h"

//-----------------------------------------------------------------------------

// Overload pattern for visiting std::variant using std::visit, see
// https://en.cppreference.com/w/cpp/utility/variant/visit

// The struct Overloaded can have arbitrary many base classes (Ts ...). It publicly inherits from each class and brings
// the call operator (Ts::operator...) of each base class into its scope. The base classes need an overloaded call
// operator (Ts::operator()).


// Overload pattern
template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Explicit deduction guide for the overload pattern above (not needed as of C++20)
template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

//-----------------------------------------------------------------------------

namespace multio::util {

template <typename T, typename V>
struct GetVariantIndex;

template <typename T, typename... Ts>
struct GetVariantIndex<T, std::variant<Ts...>>
    : std::integral_constant<size_t, std::variant<TypeTag<Ts>...>{TypeTag<T>{}}.index()> {};

//-----------------------------------------------------------------------------

}  // namespace multio::util


//-----------------------------------------------------------------------------
