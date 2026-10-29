export module Perception.Classifier.State;

export import :TagClassifications;

import Common.State.Type;
import Perception.Classifier.Type;
import std;

namespace
{
    using Perception::Classifier::Type::Classifieds;
}

export namespace Perception::Classifier::State
{
    class ClassifierStore : public Common::State::Type::Snapshot<Classifieds> {};
}