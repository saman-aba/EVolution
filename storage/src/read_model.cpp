#include "evolution/storage/read_model.hpp"

#include <utility>

namespace evolution::storage {

ReadModelDescriptor::ReadModelDescriptor(ReadModelDescriptorData data) : data_(std::move(data)) {}

auto ReadModelDescriptor::create(ReadModelDescriptorData data) -> Result<ReadModelDescriptor> {
    if (data.name.empty() || data.version.empty()) {
        return Result<ReadModelDescriptor>::failure(
            Error(ErrorCode::create("storage.invalid_read_model"), ErrorCategory::InvalidArgument,
                  "read model name and version cannot be empty"));
    }
    if (data.authority == ReadModelAuthority::Derived) {
        if (!data.projection_component || !data.projection_version ||
            data.projection_version->empty() || !data.configuration_id || !data.provenance_id ||
            data.authoritative_sources.empty()) {
            return Result<ReadModelDescriptor>::failure(
                Error(ErrorCode::create("storage.incomplete_derived_read_model"),
                      ErrorCategory::InvalidArgument,
                      "derived read models require sources, projection version, configuration, and "
                      "provenance"));
        }
    }
    if (data.authority == ReadModelAuthority::Authoritative && data.cache) {
        return Result<ReadModelDescriptor>::failure(
            Error(ErrorCode::create("storage.authoritative_cache_conflict"),
                  ErrorCategory::InvalidArgument, "a cache cannot be declared authoritative"));
    }
    if (data.rebuildable && data.authoritative_sources.empty()) {
        return Result<ReadModelDescriptor>::failure(Error(
            ErrorCode::create("storage.missing_rebuild_sources"), ErrorCategory::InvalidArgument,
            "rebuildable read models require retained authoritative sources"));
    }
    return Result<ReadModelDescriptor>::success(ReadModelDescriptor(std::move(data)));
}

auto ReadModelDescriptor::data() const noexcept -> const ReadModelDescriptorData & {
    return data_;
}

} // namespace evolution::storage
