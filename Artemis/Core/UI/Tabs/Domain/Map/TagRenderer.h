#pragma 

#include <string>
#include <vector>

enum class FieldType {
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64, Degree,
    Flags8, Flags16, Flags32, Flags64,
    Enum8, Enum16, Enum32,
    StringID, Vector3, Point3, Degree3, Vector4, Point2,
    Color, Color32, Color64,
    RangeF, RangeD, RangeDegree,
    TagBlock, TagRef, DataRef,
    String, Quaternion, Matrix, Unknown
};

struct FieldDefinition 
{
    const char* Name;
    FieldType Type;
    size_t Offset;
    const char* ExtraInfo;
    size_t EntrySize;
};

class TagRenderer 
{
public:
    void Render(const std::vector<FieldDefinition>& schema, void* baseAddr);

private:
    void DrawTagBlock(const char* label, const char* schemaName, void* ptr);
};