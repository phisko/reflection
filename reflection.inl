#include "reflection.hpp"

#include <string_view>

#include "meta/nameof.hpp"
#include "meta/for_each.hpp"
#include "meta/members.hpp"
#include "meta/traits/has_member.hpp"
#include "meta/traits/member_function_signature.hpp"

// Define a type_info for a templated type, like C in the example above
#define putils_reflection_info_template struct putils::reflection::type_info<refltype>

// Define a type_info for a given type
#define putils_reflection_info template<> putils_reflection_info_template 

// Place inside classes that need to have reflectible private fields
#define putils_reflection_friend(T) friend struct putils::reflection::type_info<T>;

// implementation detail
#define putils_impl_reflection_static_tuple(NAME, ...) static constexpr auto NAME = std::make_tuple(__VA_ARGS__);

// Lets you define a custom class name
#define putils_reflection_custom_class_name(className) static constexpr auto class_name = putils_nameof(className) + (std::string_view(putils_nameof(className)).rfind("::") != std::string_view::npos ? std::string_view(putils_nameof(className)).rfind("::") + 2 : 0);

// Uses refltype as class name
#define putils_reflection_class_name putils_reflection_custom_class_name(refltype)

#define putils_reflection_attributes(...) putils_impl_reflection_static_tuple(attributes, __VA_ARGS__)
#define putils_reflection_methods(...) putils_impl_reflection_static_tuple(methods, __VA_ARGS__)
#define putils_reflection_parents(...) putils_impl_reflection_static_tuple(parents, __VA_ARGS__)
#define putils_reflection_used_types(...) putils_impl_reflection_static_tuple(used_types, __VA_ARGS__)

#define putils_reflection_attribute(member, ...) putils::reflection::attribute_info{ .name = #member, .ptr = &refltype::member, .metadata = putils::make_table(__VA_ARGS__) }
#define putils_reflection_metadata(key, value) key, value
#define putils_reflection_attribute_private(member, ...) putils::reflection::attribute_info{ .name = #member + 1, .ptr = &refltype::member, .metadata = putils::make_table(__VA_ARGS__) }
#define putils_reflection_type(T, ...) putils::reflection::used_type_info{ .type = putils::meta::type<T>(), .metadata = putils::make_table(__VA_ARGS__) }

namespace putils::reflection {

#pragma region impl
	namespace detail {
		inline static const auto emptyTuple = std::tuple<>();
	}

#pragma region macros
#define putils_impl_reflection_member_detector(NAME) \
	namespace detail {\
		putils_member_detector(NAME);\
	}\
	template<typename T>\
	constexpr bool has_##NAME() noexcept {\
		return detail::has_member_##NAME<type_info<T>>();\
	}

#define putils_impl_reflection_member_detector_with_parents(NAME) \
	namespace detail {\
		putils_member_detector(NAME);\
	}\
	template<typename T>\
	constexpr bool has_##NAME() noexcept {\
		if constexpr (detail::has_member_##NAME<type_info<T>>())\
			return true;\
		return for_each_parent<T>([](const auto & parent) noexcept -> bool {\
			using Parent = putils_wrapped_type(parent.type);\
			if constexpr (detail::has_member_##NAME<type_info<Parent>>())  \
                return true;\
            return false;\
		});\
	}

#define putils_impl_reflection_member_get_single(NAME, defaultValue) \
	namespace detail{\
		template<typename T>\
		constexpr decltype(auto) get_single_##NAME() noexcept {\
			if constexpr (detail::has_member_##NAME<type_info<T>>())\
				return type_info<T>::NAME;\
			else\
                return defaultValue;\
		}\
	}

#define putils_impl_reflection_member_get_all(NAME) \
	namespace detail {\
		template<typename T, typename ... Ts>\
		constexpr auto get_all_##NAME() noexcept {\
			if constexpr (sizeof...(Ts) == 0)\
				return get_single_##NAME<T>();\
			else\
				return std::tuple_cat(get_single_##NAME<T>(), get_all_##NAME<Ts...>());\
		}\
\
		template<typename T, typename ... Parents, typename ... MetadataTables>\
		constexpr auto get_all_##NAME(const std::tuple<putils::reflection::used_type_info<Parents, MetadataTables>...> &) noexcept {\
			return get_all_##NAME<T, Parents...>();\
		}\
	}
#pragma endregion

#pragma region parents
	putils_impl_reflection_member_detector(parents);
	putils_impl_reflection_member_get_single(parents, detail::emptyTuple);
	namespace detail {
		template<typename T>
		constexpr auto get_all_parents() noexcept;

		template<typename ... Ts, typename ... MetadataTables>
		constexpr auto get_all_parents(const std::tuple<used_type_info<Ts, MetadataTables>...> &) noexcept {
			return std::tuple_cat(get_all_parents<Ts>()...);
		}

		template<typename T>
		constexpr auto get_all_parents() noexcept {
			if constexpr (has_parents<T>())
				return std::tuple_cat(get_single_parents<T>(), get_all_parents(get_single_parents<T>()));
			else
				return get_single_parents<T>();
		}
	}
#pragma endregion

#pragma region class_name
	putils_impl_reflection_member_detector(class_name);
	putils_impl_reflection_member_get_single(class_name, typeid(T).name());
#pragma endregion

#define putils_impl_reflection_member(NAME) \
	putils_impl_reflection_member_detector_with_parents(NAME) \
	putils_impl_reflection_member_get_single(NAME, detail::emptyTuple) \
	putils_impl_reflection_member_get_all(NAME)

	putils_impl_reflection_member(attributes);
	putils_impl_reflection_member(methods);
	putils_impl_reflection_member(used_types);

	namespace detail {
		template<typename T>
		struct type_info_with_parents {
			static constexpr auto class_name = has_member_class_name<type_info<T>>() ? get_single_class_name<T>() : (const char *)nullptr;
			static constexpr auto parents = get_all_parents<T>();
			static constexpr auto attributes = get_all_attributes<T>(parents);
			static constexpr auto methods = get_all_methods<T>(parents);
			static constexpr auto used_types = get_all_used_types<T>(parents);
		};
	}

#pragma endregion

	template<typename T>
	constexpr auto get_class_name() noexcept {
		if constexpr (detail::type_info_with_parents<T>::class_name != nullptr)
			return detail::type_info_with_parents<T>::class_name;
		else
			return typeid(T).name();
	}

#define putils_impl_reflection_member_getter_and_for_each(NAME) \
	template<typename T>\
	constexpr const auto & get_##NAME##s() noexcept {\
		return detail::type_info_with_parents<T>::NAME##s;\
	}\
\
	template<typename T, typename Func>\
	constexpr auto for_each_##NAME(Func && func) noexcept {\
		return tuple_for_each(get_##NAME##s<T>(), func);\
	}

	// get_attributes/methods<T>():
	//		returns a tuple<pair<const char *, MemberPointer>> of all the attributes/methods in T and its parents
	// get_parents/used_types<T>():
	//		returns a tuple<putils::meta::type<T>> of all the parents/used types of T and its parents

	// for_each_attribute/method<T>(functor):
	//		calls `functor(name, memberPointer)` for each attribute/method in T and its parents
	// for_each_parent/used_type<T>(functor):
	//		calls `functor(putils::meta::type<Type>)` for each parent/used type of T and its parents

	putils_impl_reflection_member_getter_and_for_each(attribute);
	putils_impl_reflection_member_getter_and_for_each(method);
	putils_impl_reflection_member_getter_and_for_each(parent);
	putils_impl_reflection_member_getter_and_for_each(used_type);

#pragma region attributes
	template<typename T, typename Func>
	constexpr auto for_each_attribute(T && obj, Func && func) noexcept {
		return for_each_attribute<std::decay_t<T>>([&](const auto & attrInfo) noexcept {
			return func(object_attribute_info{
				.name = attrInfo.name,
				.member = obj.*attrInfo.ptr,
				.metadata = attrInfo.metadata
			});
		});
	}

	template<typename Attribute, typename T>
	constexpr std::optional<Attribute T::*> get_attribute(std::string_view name) noexcept {
		return for_each_attribute<T>([&](const auto & attr) noexcept -> std::optional<Attribute T::*> {
			if constexpr (std::is_same<putils::MemberType<putils_typeof(attr.ptr)>, Attribute>()) {
				if (name == attr.name)
					return (Attribute T::*)attr.ptr;
			}
            return std::nullopt;
		});
	}

	template<typename Attribute, typename T>
	constexpr auto get_attribute(T && obj, std::string_view name) noexcept {
		const auto member = get_attribute<Attribute, std::decay_t<T>>(name);

        using ReturnType = decltype(&(obj.*(*member)));
		if (!member)
			return ReturnType(nullptr);
		return &(obj.*(*member));
	}
#pragma endregion

#pragma region methods
	template<typename T, typename Func>
	constexpr auto for_each_method(T && obj, Func && func) noexcept {
		return for_each_method<std::decay_t<T>>([&](const auto & attrInfo) noexcept {
			return func(object_method_info{
				.name = attrInfo.name,
				.method = [&](auto && ... args) { return (obj.*attrInfo.ptr)(FWD(args)...); },
				.metadata = attrInfo.metadata
			});
		});
	}

    namespace detail {
        template<typename Signature, typename T>
        constexpr auto get_method(std::string_view name) noexcept {
            return for_each_method<T>([&](const auto & attr) noexcept -> std::optional<Signature> {
                if constexpr (std::is_same<Signature, putils_typeof(attr.ptr)>()) {
                    if (name == attr.name)
                        return attr.ptr;
                }
                return std::nullopt;
            });
        }

        template<typename Signature>
        struct get_method_helper;

        template<typename Ret, typename T, typename ... Args>
        struct get_method_helper<Ret (T::*)(Args...)> {
            static constexpr auto get(std::string_view name) noexcept {
                return detail::get_method<Ret (T::*)(Args...), T>(name);
            }
        };

        template<typename Ret, typename T, typename ... Args>
        struct get_method_helper<Ret (T::*)(Args...) const> {
            static constexpr auto get(std::string_view name) noexcept {
                return detail::get_method<Ret (T::*)(Args...) const, T>(name);
            }
        };

        template<typename Ret, typename T, typename ... Args>
        struct get_method_helper<Ret (T::*)(Args...) noexcept> {
            static constexpr auto get(std::string_view name) noexcept {
                return detail::get_method<Ret (T::*)(Args...) noexcept, T>(name);
            }
        };

        template<typename Ret, typename T, typename ... Args>
        struct get_method_helper<Ret (T::*)(Args...) const noexcept> {
            static constexpr auto get(std::string_view name) noexcept {
                return detail::get_method<Ret (T::*)(Args...) const noexcept, T>(name);
            }
        };
    };

    template<typename Signature>
    constexpr auto get_method(std::string_view name) noexcept {
        return detail::get_method_helper<Signature>::get(name);
    }

	template<typename Signature, typename T>
    constexpr auto get_method(std::string_view name) noexcept {
        return for_each_method<T>([&](const auto &attr) noexcept -> std::optional<Signature T::*> {
            using currentSignature = putils::member_function_signature<putils_typeof(attr.ptr)>;
            if constexpr (std::is_same<Signature, currentSignature>()) {
                if (name == attr.name)
                    return (Signature T::*)attr.ptr;
            }
            return std::nullopt;
        });
    }

	template<typename Signature, typename T>
	constexpr auto get_method(T && obj, std::string_view name) noexcept {
        const auto callMethod = [&](const auto method) {
            const auto ret = [&obj, method](auto && ... args) noexcept {
                const auto ptr = *method;
                auto & decayed = (std::decay_t<T> &)obj; // method might have lost its qualifier
                return (decayed.*ptr)(FWD(args)...);
            };

            using ReturnType = std::optional<decltype(ret)>;
            if (!method)
                return ReturnType(std::nullopt);
            return ReturnType(ret);
        };

        if constexpr (std::is_member_function_pointer_v<Signature> ) {
            const auto method = get_method<Signature>(name);
            return callMethod(method);
        }
        else {
            const auto method = get_method<Signature, std::decay_t<T>>(name);
            return callMethod(method);
        }
	}
#pragma endregion

#pragma region metadata
	template<typename ... Metadata, typename Key>
	constexpr bool has_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept {
		return has_key(metadata, FWD(key));
	}

	template<typename Ret, typename ... Metadata, typename Key>
	constexpr const Ret & get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept {
		return get_value<Ret>(metadata, FWD(key));
	}

	template<typename Ret, typename ... Metadata, typename Key>
	constexpr const Ret * try_get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept {
		return try_get_value<Ret>(metadata, FWD(key));
	}
#pragma endregion
}
