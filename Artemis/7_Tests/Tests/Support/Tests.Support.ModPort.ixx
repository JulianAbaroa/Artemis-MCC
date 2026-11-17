export module Mod.Port.System;

export import :TickProducer;

// The primary interface of Mod.Port.System for the tests: only the partition of the TickProducerAdapter, whose sources are compiled in this
// project (RV.1). The other partitions of the port need the whole engine and are not part of the test executable.