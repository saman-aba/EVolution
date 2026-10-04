#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_CORE_EXPORTS)
#define EVOLUTION_CORE_API __declspec(dllexport)
#else
#define EVOLUTION_CORE_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_CORE_API
#endif

#define EVOLUTION_STABLE_API
#define EVOLUTION_EXPERIMENTAL_API

namespace evolution {

enum class ApiStability {
    Experimental,
    Stable,
    Deprecated,
};

inline constexpr ApiStability core_api_stability = ApiStability::Experimental;

} // namespace evolution
