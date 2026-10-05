#include "evolution/ingestion/ingestion.hpp"

#include <utility>

namespace evolution::ingestion {

RawRecord::RawRecord(RawRecordData data) : data_(std::move(data)) {}

auto RawRecord::create(RawRecordData data) -> Result<RawRecord> {
    if (data.media_type.empty() || data.bytes.empty()) {
        return Result<RawRecord>::failure(
            Error(ErrorCode::create("ingestion.invalid_raw_record"), ErrorCategory::InvalidArgument,
                  "raw record requires media type and non-empty bytes"));
    }
    if (data.source_version && data.source_version->empty()) {
        return Result<RawRecord>::failure(
            Error(ErrorCode::create("ingestion.invalid_source_version"),
                  ErrorCategory::InvalidArgument, "source version cannot be empty"));
    }
    return Result<RawRecord>::success(RawRecord(std::move(data)));
}

auto RawRecord::data() const noexcept -> const RawRecordData & {
    return data_;
}

} // namespace evolution::ingestion
