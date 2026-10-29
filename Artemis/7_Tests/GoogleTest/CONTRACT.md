# Artemis.GoogleTest

Group `7_Tests` - static library, third party - GoogleTest and gMock built once for the test executable.

## Responsibility
Compiles `External/googletest` (GoogleTest with gMock, BSD-3-Clause) with the same toolset and runtime as the rest of the solution,
so `Artemis.Tests` links it. It has no code of its own.

## Does not
- Get linked into the DLL: only `Artemis.Tests` references it.

## Input and output
The static libraries of GoogleTest and gMock.

## Public interface
None: it exports no C++ module. `Artemis.Tests` includes the GoogleTest headers before its imports.

## Depends on
Nothing in the solution.

## State between ticks
Not applicable.

## Threads
Not applicable.

## Errors
Not applicable.

## Requirements
Same configuration (C++ standard, runtime library, `x64`) as `Artemis.Tests`.

## Tests
Not applicable.
