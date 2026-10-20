export module Relations.Classifier.State;

export import :TagClassifications;

import Common.State.Type;
import Relations.Classifier.Type;
import std;

export namespace Relations::Classifier::State
{
    using Relations::Classifier::Type::Classifieds;
    class ClassifierStore : public Common::State::Type::Snapshot<Classifieds> {};
}