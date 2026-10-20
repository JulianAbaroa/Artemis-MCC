module;

export module Viewer.Fake.Widget;
import Viewer.Other.Type;

export namespace Viewer::Wrong::Widget
{
    // This function is a fake (see Ignore/Referencia/x.txt).
    // Esto es para el caso de prueba.
    using FakePtr = void(*)(int);
    using Viewer::Other::Type::Thing;

    class Widget
    {
    public:
        auto Run(int* pValue) -> int;

    private:
        static inline Widget* s_Instance{ nullptr };

    public:
        auto Other() -> int;
    };

    auto Limit(int a, int b) -> int
    {
        return std::max(a, b);
    }
}