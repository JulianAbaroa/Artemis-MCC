module;

export module DevTools.Viewer.Fake.Widget;
import DevTools.Viewer.Other.Type;

export namespace DevTools::Viewer::Wrong::Widget
{
    // This function is a fake (see Ignore/Referencia/x.txt).
    // Esto es para el caso de prueba.
    using FakePtr = void(*)(int);
    using DevTools::Viewer::Other::Type::Thing;

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