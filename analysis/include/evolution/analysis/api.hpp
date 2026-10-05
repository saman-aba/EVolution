#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_ANALYSIS_EXPORTS)
#define EVOLUTION_ANALYSIS_API __declspec(dllexport)
#else
#define EVOLUTION_ANALYSIS_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_ANALYSIS_API
#endif
