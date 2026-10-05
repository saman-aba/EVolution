#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_DOMAIN_POKER_EXPORTS)
#define EVOLUTION_DOMAIN_POKER_API __declspec(dllexport)
#else
#define EVOLUTION_DOMAIN_POKER_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_DOMAIN_POKER_API
#endif
