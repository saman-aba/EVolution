#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_DOMAIN_EXPORTS)
#define EVOLUTION_DOMAIN_API __declspec(dllexport)
#else
#define EVOLUTION_DOMAIN_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_DOMAIN_API
#endif
