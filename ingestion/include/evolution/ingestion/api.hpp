#pragma once

#if defined(_WIN32) && defined(EVOLUTION_BUILD_SHARED)
#if defined(EVOLUTION_INGESTION_EXPORTS)
#define EVOLUTION_INGESTION_API __declspec(dllexport)
#else
#define EVOLUTION_INGESTION_API __declspec(dllimport)
#endif
#else
#define EVOLUTION_INGESTION_API
#endif
