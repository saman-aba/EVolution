#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_SECURITY_EXPORTS)
#define EVOLUTION_SECURITY_API __declspec(dllexport)
#else
#define EVOLUTION_SECURITY_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_SECURITY_API
#endif
