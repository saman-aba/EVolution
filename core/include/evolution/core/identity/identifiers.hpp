#pragma once

#include "evolution/core/identity/id.hpp"

namespace evolution::identity {

struct AlgorithmTag;
struct AnalysisTag;
struct ComponentTag;
struct ConfigurationTag;
struct CorrelationTag;
struct ContextTag;
struct EnvelopeTag;
struct EventTag;
struct MeasurementTag;
struct MetricTag;
struct ProcessorTag;
struct ProjectionTag;
struct ProvenanceTag;
struct RunTag;
struct SourceTag;
struct StateTag;
struct StreamTag;
struct TimeSeriesTag;
struct TraceTag;

using AlgorithmId = Id<AlgorithmTag>;
using AnalysisId = Id<AnalysisTag>;
using ComponentId = Id<ComponentTag>;
using ConfigurationId = Id<ConfigurationTag>;
using CorrelationId = Id<CorrelationTag>;
using ContextId = Id<ContextTag>;
using EnvelopeId = Id<EnvelopeTag>;
using EventId = Id<EventTag>;
using MeasurementId = Id<MeasurementTag>;
using MetricId = Id<MetricTag>;
using ProcessorId = Id<ProcessorTag>;
using ProjectionId = Id<ProjectionTag>;
using ProvenanceId = Id<ProvenanceTag>;
using RunId = Id<RunTag>;
using SourceId = Id<SourceTag>;
using StateId = Id<StateTag>;
using StreamId = Id<StreamTag>;
using TimeSeriesId = Id<TimeSeriesTag>;
using TraceId = Id<TraceTag>;

} // namespace evolution::identity

namespace evolution {

using identity::AlgorithmId;
using identity::AnalysisId;
using identity::ComponentId;
using identity::ConfigurationId;
using identity::ContextId;
using identity::CorrelationId;
using identity::EnvelopeId;
using identity::EventId;
using identity::MeasurementId;
using identity::MetricId;
using identity::ProcessorId;
using identity::ProjectionId;
using identity::ProvenanceId;
using identity::RunId;
using identity::SourceId;
using identity::StateId;
using identity::StreamId;
using identity::TimeSeriesId;
using identity::TraceId;

} // namespace evolution
