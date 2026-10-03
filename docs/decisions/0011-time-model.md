## Missing and Estimated Time

EVolution must distinguish between:

```text
Known time
Estimated time
Unknown time
```

These describe the **quality or certainty of temporal information**, not different kinds of clocks.

### Known Time

A timestamp is **known** when the source provides a temporal value that EVolution accepts as sufficiently authoritative for the relevant semantic purpose.

For example:

```text
event_time = 2026-10-03T12:30:15.123Z
time_status = KNOWN
```

"Known" does not necessarily mean perfect physical accuracy. It means that the timestamp is represented as an observed or explicitly supplied value rather than being inferred by EVolution.

The source may still have its own documented accuracy or resolution.

For example:

```text
source precision = 1 second
```

does not justify representing the value as though it were accurate to nanoseconds.

### Estimated Time

A timestamp is **estimated** when the temporal value is not directly known but has been inferred from available information.

Examples include:

```text
source provides only a date
source provides a time range
timestamp reconstructed from sequence information
timestamp inferred from neighboring observations
timestamp inferred from an external correlation
```

For example:

```text
event_time = 2026-10-03T12:30:00Z
time_status = ESTIMATED
```

The timestamp remains useful for temporal processing, but EVolution must not present it as though it were directly observed.

The estimation method should be recorded when the estimate can materially affect interpretation or reproducibility.

Conceptually:

```text
estimated_time
    +
estimation_method
    +
supporting_information
```

may be part of the object's provenance or temporal metadata.

### Unknown Time

A timestamp is **unknown** when no meaningful temporal value is available.

For example:

```text
event_time = absent
time_status = UNKNOWN
```

Unknown time must **not** be represented by a fabricated timestamp such as:

```text
1970-01-01
current_time()
0
```

unless the domain explicitly defines such a value as meaningful.

In particular:

```text
unknown event time
    ≠
ingestion time
```

If ingestion time is known, it may still be recorded separately:

```text
event_time    = UNKNOWN
ingestion_time = 2026-10-03T12:30:15Z
```

This preserves the distinction between when the event happened and when EVolution received it.

### Missing vs Unknown

"Missing" describes the **absence of a value in the input or representation**.

"Unknown" describes the **semantic state of the temporal information**.

For example:

```text
source contains no event timestamp
        ↓
event_time field is missing
        ↓
event_time semantic status = UNKNOWN
```

The distinction becomes useful when an object is transformed.

For example, an ingestion layer may receive:

```text
date = 2026-10-03
time = missing
```

and determine that the exact event time is unknown.

Therefore:

```text
missing
    → representation/input condition

unknown
    → semantic knowledge condition
```

They should not be treated as interchangeable concepts.

### Estimated vs Unknown

An estimated timestamp contains a temporal hypothesis or approximation.

An unknown timestamp contains no sufficiently supported temporal value.

Therefore:

```text
estimated:
    "we have evidence supporting approximately this time"

unknown:
    "we do not have sufficient information to assign a time"
```

An estimated timestamp may therefore participate in analytical processing where the processor explicitly supports estimated time.

Unknown time may prevent participation in operations requiring a temporal position.

### Time Status Must Not Alter the Timestamp Meaning

The status does not change the semantic type of the timestamp.

For example:

```text
event_time = T
status     = KNOWN
```

and:

```text
event_time = T
status     = ESTIMATED
```

both represent an `event_time`.

The status describes how the value is known.

Likewise:

```text
event_time = absent
status     = UNKNOWN
```

represents an event whose event time is unavailable.

This distinction prevents the system from creating separate timestamp concepts merely because their certainty differs.

### Propagation of Estimated Time

Derived objects must not silently upgrade an estimated timestamp to a known timestamp.

For example:

```text
Event
    event_time = T
    status = ESTIMATED
        ↓
Measurement
    time = T
```

must not imply:

```text
Measurement
    time_status = KNOWN
```

unless the measurement has an independent, authoritative temporal basis.

Derived temporal information should preserve or explicitly transform the uncertainty/knowledge status.

### Propagation of Unknown Time

A derived object does not necessarily become temporally unusable merely because one input lacks time.

The processing contract must determine whether the result can still have a meaningful temporal interpretation.

For example:

```text
Event A
    event_time = T1

Event B
    event_time = UNKNOWN
```

A measurement over Event A alone may still have:

```text
time = T1
```

while an aggregation requiring both events to be temporally positioned may have to:

* exclude the unknown event,
* produce a result with reduced temporal scope,
* produce an unknown temporal boundary,
* or reject the operation.

The choice is an explicit processing or analytical policy, not a Core assumption.

### Estimated Time Is Not Uncertainty Magnitude

`ESTIMATED` indicates how the timestamp was obtained.

It does not by itself specify how inaccurate the estimate may be.

For example:

```text
estimated
```

could mean:

```text
±1 millisecond
±1 second
±10 minutes
```

depending on the source and estimation method.

If temporal uncertainty is important, it should be represented separately.

Conceptually:

```text
Temporal Information
{
    timestamp
    status
    uncertainty?
    provenance?
}
```

where:

```text
status:
    KNOWN
    ESTIMATED
    UNKNOWN
```

and `uncertainty` describes the applicable temporal uncertainty when known.

The exact uncertainty representation is deferred.

### Ingestion Must Not Invent Event Time

An ingestion component must not silently use ingestion time as event time merely because event time is absent.

Incorrect:

```text
source:
    event_time = missing

ingestion:
    event_time = now()
```

Correct:

```text
source:
    event_time = missing

result:
    event_time = UNKNOWN
    ingestion_time = known
```

If a domain has a documented rule that allows event time to be inferred from other source information, the result may instead be:

```text
event_time = inferred value
status     = ESTIMATED
```

with the inference recorded as appropriate provenance.

### Analytical Eligibility

Whether estimated or unknown timestamps can participate in an operation depends on the operation's contract.

For example:

```text
Operation                         Estimated       Unknown
------------------------------------------------------------
Event storage                     Yes             Yes
Event inspection                  Yes             Yes
Sequence processing               Usually         Sometimes
Event-time sorting                Explicit        No
Time-window aggregation           Explicit        No
Duration calculation              With care       No
Replay by event time               Explicit        No
Non-temporal aggregation          Often           Often
```

This table is illustrative rather than a universal Core rule.

Each processor or analysis must define its temporal requirements.

### Temporal Status and Provenance

When a timestamp is estimated, provenance should be capable of answering:

```text
Why was this timestamp estimated?
Which inputs supported the estimate?
Which method produced it?
Which configuration/version was used?
```

This is particularly important when an estimated timestamp influences:

```text
ordering
window membership
aggregation
pattern detection
analysis
```

The provenance system remains responsible for recording derivation information; the time model only defines the temporal semantics.

## Revised Decision Summary

```text
Temporal knowledge states:       KNOWN / ESTIMATED / UNKNOWN
Missing value:                   Input/representation condition
Unknown time:                    No sufficiently supported temporal value
Estimated time:                  Inferred temporal value with supporting evidence
Known time:                      Accepted source/authoritative temporal value
Estimated ≠ unknown:             Yes
Estimated ≠ inaccurate by definition: Yes
Unknown ≠ ingestion time:        Yes
Ingestion may infer event time:  Only through explicit domain/source rules
Temporal uncertainty:            Separate concept
Estimated status propagation:    Must not silently become KNOWN
Unknown status propagation:      Operation-specific
```

## Invariant

> **EVolution must distinguish known, estimated, and unknown temporal information; missing temporal input must not be silently replaced with fabricated or ingestion time, and estimated time must never be presented as directly observed without preserving its estimated status.**
