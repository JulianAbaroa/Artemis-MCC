export module Relations.Classifier.State;

export import :TagClassifications;

import Common.State.Type;
import Relations.Classifier.Type;
import std;

namespace
{
    using Relations::Classifier::Type::Classifieds;
}

export namespace Relations::Classifier::State
{
    class ClassifierStore : public Common::State::Type::Snapshot<Classifieds> {};
}