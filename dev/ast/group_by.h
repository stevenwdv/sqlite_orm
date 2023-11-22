#pragma once

#include <tuple>  //  std::tuple, std::make_tuple
#include <type_traits>  //  std::true_type, std::false_type
#include <utility>  //  std::forward, std::move

#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, class... Args>
        struct group_by_with_having {
            using args_type = std::tuple<Args...>;
            using expression_type = T;

            args_type args;
            expression_type expression;
        };

        /**
         *  GROUP BY pack holder.
         */
        template<class... Args>
        struct group_by_t {
            using args_type = std::tuple<Args...>;

            args_type args;

            template<class T>
            group_by_with_having<T, Args...> having(T expression) {
                return {std::move(this->args), std::move(expression)};
            }
        };

        template<class T>
        using is_group_by = polyfill::disjunction<polyfill::is_specialization_of<T, group_by_t>,
                                                  polyfill::is_specialization_of<T, group_by_with_having>>;
    }

    /**
     *  GROUP BY column.
     *  Example: storage.get_all<Employee>(group_by(&Employee::name))
     */
    template<class... Args>
    internal::group_by_t<Args...> group_by(Args... args) {
        return {{std::forward<Args>(args)...}};
    }
}
