export module Relations.Classifier.State;

import Common.State.System;
import Relations.Classifier.Type;
import std;

export namespace Relations::Classifier::State
{
    using Classifieds = std::vector<Relations::Classifier::Type::Classified>;
    using ClassifierStore = Common::State::System::Snapshot<Classifieds>;
}