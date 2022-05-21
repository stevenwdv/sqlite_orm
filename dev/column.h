#pragma once

#include <tuple>  //  std::tuple
#include <string>  //  std::string
#include <memory>  //  std::unique_ptr
#include <type_traits>  //  std::false_type, std::is_same, std::enable_if

#include "functional/cxx_universal.h"
#include "functional/cxx_polyfill.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "type_traits.h"
#include "member_traits/member_traits.h"
#include "type_is_nullable.h"
#include "constraints.h"

namespace sqlite_orm {

    namespace internal {

        struct basic_column {

            /**
             *  Column name. Specified during construction in `make_column()`.
             */
            const std::string name;
        };

        struct empty_setter {};

        template<class G, class S>
        struct column_field {
            using member_pointer_t = G;
            using setter_type = S;
            using object_type = member_object_type_t<G>;
            using field_type = member_field_type_t<G>;

            /**
             *  Member pointer used to read a field value.
             *  If it is a object member pointer it is also used to write a field value.
             */
            const member_pointer_t member_pointer;

            /**
             *  Setter member function to write a field value
             */
            SQLITE_ORM_NOUNIQUEADDRESS
            const setter_type setter;

            /**
             *  Simplified interface for `NOT NULL` constraint
             */
            constexpr bool is_not_null() const {
                return !type_is_nullable<field_type>::value;
            }
        };

        template<class... Op>
        struct column_constraints {
            using constraints_type = std::tuple<Op...>;

            SQLITE_ORM_NOUNIQUEADDRESS
            const constraints_type constraints;

            /**
             *  Checks whether contraints are of trait `Trait`
             */
            template<template<class...> class Trait>
            constexpr bool is() const {
                return tuple_has<Trait, constraints_type>::value;
            }

            constexpr bool is_generated() const {
#if SQLITE_VERSION_NUMBER >= 3031000
                return is<is_generated_always>();
#else
                return false;
#endif
            }

            /**
             *  Simplified interface for `DEFAULT` constraint
             *  @return string representation of default value if it exists otherwise nullptr
             */
            std::unique_ptr<std::string> default_value() const;
        };

        /**
         *  This class stores information about a single column.
         *  column_t is a pair of [column_name:member_pointer] mapped to a storage.
         *  
         *  G is a member object pointer or member function pointer
         *  S is a member function pointer or `empty_setter`
         *  Op... is a constraints pack, e.g. primary_key_t, autoincrement_t etc
         */
        template<class G, class S, class... Op>
        struct column_t : basic_column, column_field<G, S>, column_constraints<Op...> {
#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            column_t(std::string name, G memberPointer, S setter, std::tuple<Op...> op) :
                basic_column{move(name)}, column_field<G, S>{memberPointer, setter}, column_constraints<Op...>{
                                                                                         move(op)} {}
#endif
            // Simplified interface for cast to base class
            constexpr const column_constraints<Op...>& as_column_constraints() const {
                return *this;
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_v = polyfill::is_specialization_of_v<T, column_t>;

        template<class T>
        using is_column = polyfill::bool_constant<is_column_v<T>>;

        template<class T>
        using column_field_type_t = polyfill::detected_or_t<void, field_type_t, T>;

        template<class T>
        using column_constraints_type_t = polyfill::detected_or_t<std::tuple<>, constraints_type_t, T>;

        template<class Elements, template<class...> class TraitFn>
        using col_index_sequence_with = filter_tuple_sequence_t<Elements,
                                                                check_if_tuple_has<TraitFn>::template fn,
                                                                column_constraints_type_t,
                                                                filter_tuple_sequence_t<Elements, is_column>>;

        template<class Elements, template<class...> class TraitFn>
        using col_index_sequence_excluding = filter_tuple_sequence_t<Elements,
                                                                     check_if_tuple_has_not<TraitFn>::template fn,
                                                                     column_constraints_type_t,
                                                                     filter_tuple_sequence_t<Elements, is_column>>;
    }

    /**
     *  Column builder function. You should use it to create columns instead of constructor
     */
    template<class M, class... Op, internal::satisfies<std::is_member_object_pointer, M> = true>
    internal::column_t<M, internal::empty_setter, Op...> make_column(std::string name, M m, Op... constraints) {
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {move(name), m, {}, std::make_tuple(constraints...)});
    }

    /**
     *  Column builder function with setter and getter. You should use it to create columns instead of constructor
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, S setter, G getter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {move(name), getter, setter, std::make_tuple(constraints...)});
    }

    /**
     *  Column builder function with getter and setter (reverse order). You should use it to create columns instead of
     * constructor
     */
    template<class G,
             class S,
             class... Op,
             internal::satisfies<internal::is_getter, G> = true,
             internal::satisfies<internal::is_setter, S> = true>
    internal::column_t<G, S, Op...> make_column(std::string name, G getter, S setter, Op... constraints) {
        static_assert(std::is_same<internal::setter_field_type_t<S>, internal::getter_field_type_t<G>>::value,
                      "Getter and setter must get and set same data type");
        static_assert(polyfill::conjunction_v<internal::is_constraint<Op>...>, "Incorrect constraints pack");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {move(name), getter, setter, std::make_tuple(constraints...)});
    }
}
