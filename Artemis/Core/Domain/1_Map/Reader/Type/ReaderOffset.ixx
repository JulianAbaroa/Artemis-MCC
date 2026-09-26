export module Map.Reader.Type:Offset;

import std;

export namespace Map::Reader::Type::Offset
{
    // Size: 0xA000.
    namespace Header
    {
        constexpr std::size_t k_FileSize{ 0x8 };           // uint32.  
        constexpr std::size_t k_TagBufferOffset{ 0x10 };   // uint32.
        constexpr std::size_t k_VirtualSize{ 0x14 };       // uint32.

        constexpr std::size_t k_Type{ 0x18 };  // int16.

        constexpr std::size_t k_FileTableCount{ 0x20 };        // int32.
        constexpr std::size_t k_FileTableOffset{ 0x24 };       // int32.
        constexpr std::size_t k_FileTableSize{ 0x28 };         // int32.
        constexpr std::size_t k_FileIndexTableOffset{ 0x2C };  // int32.

        constexpr std::size_t k_StringTableCount{ 0x30 };              // int32.
        constexpr std::size_t k_StringTableOffset{ 0x34 };             // int32.
        constexpr std::size_t k_StringTableSize{ 0x38 };               // int32.
        constexpr std::size_t k_StringIndexTableOffset{ 0x3C };        // int32.
        constexpr std::size_t k_StringNamespaceTableCount{ 0x40 };     // int32.
        constexpr std::size_t k_StringNamespaceTableOffset{ 0x44 };    // int32.

        constexpr std::size_t k_CacheBuildDateHigh{ 0x50 };    // int32.
        constexpr std::size_t k_CacheBuildDateLow{ 0x54 };     // int32.

        constexpr std::size_t k_BuildString{ 0xA0 };   // ASCIIZ.

        constexpr std::size_t k_InternalName{ 0xC0 };  // ASCIIZ.
        constexpr std::size_t k_ScenarioName{ 0xE0 };  // ASCIIZ.

        constexpr std::size_t k_Checksum{ 0x360 }; // int32.

        constexpr std::size_t k_VirtualBaseAddress{ 0x2E0 };   // uint64.
        constexpr std::size_t k_IndexHeaderAddress{ 0x2E8 };   // uint64.
        constexpr std::size_t k_XDKVersion{ 0x2F0 };           // uint32.

        namespace Partitions
        {
            // Array data.
            constexpr std::size_t k_Base{ 0x300 };
            constexpr std::size_t k_Count{ 6 };
            constexpr std::size_t k_EntrySize{ 0x10 };

            constexpr std::size_t k_LoadAddress{ 0x0 };    // uint64.
            constexpr std::size_t k_Size{ 0x8 };           // uint64.

            constexpr std::size_t EntryOffset(std::size_t index)
            {
                return k_Base + (index * k_EntrySize);
            }
        }

        // Interop data.
        namespace OffsetMasks
        {
            // Array data.
            constexpr std::size_t k_Base{ 0x4CC };
            constexpr std::size_t k_Count{ 4 };
            constexpr std::size_t k_EntrySize{ 0x4 };

            constexpr std::size_t k_Mask{ 0x0 };   // uint32.

            constexpr std::size_t EntryOffset(std::size_t index)
            {
                return k_Base + (index * k_EntrySize);
            }
        }

        namespace Sections
        {
            // Array data.
            constexpr std::size_t k_Base{ 0x4DC };
            constexpr std::size_t k_Count{ 4 };
            constexpr std::size_t k_EntrySize{ 0x8 };

            constexpr std::size_t k_VirtualAddress{ 0x0 }; // uint32.
            constexpr std::size_t k_Size{ 0x4 };           // uint32.

            constexpr std::size_t EntryOffset(std::size_t index)
            {
                return k_Base + (index * k_EntrySize);
            }
        }
    }

    // Size: 0x4C.
    namespace IndexTableHeader
    {
        constexpr std::size_t k_NumberOfTagGroups{ 0x0 };      // int32.
        constexpr std::size_t k_TagGroupTableAddress{ 0x8 };   // uint64.
        constexpr std::size_t k_NumberOfTags{ 0x10 };          // int32.
        constexpr std::size_t k_TagTableAddress{ 0x18 };       // uint64.
        constexpr std::size_t k_NumberOfGlobalTags{ 0x20 };    // int32.
        constexpr std::size_t k_GlobalTagTableAddress{ 0x28 }; // uint64.
        constexpr std::size_t k_NumberOfTagInterops{ 0x30 };   // int32.
        constexpr std::size_t k_TagInteropTableAddress{ 0x38 };// uint64.
        constexpr std::size_t k_Magic{ 0x48 };                 // int32.
    }

    // Size: 0x10.
    namespace TagGroupTableEntry
    {
        constexpr std::size_t k_Magic{ 0x0 };              // int32.
        constexpr std::size_t k_ParentMagic{ 0x4 };        // int32.
        constexpr std::size_t k_GrandParentMagic{ 0x8 };   // int32.
        constexpr std::size_t k_StringID{ 0xC };           // int32.
    }

    // Size: 0x8.
    namespace TagTableEntry
    {
        constexpr std::size_t k_TagGroupIndex{ 0x0 };  // int16.
        constexpr std::size_t k_DatumIndexSalt{ 0x2 }; // uint16.
        constexpr std::size_t k_MemoryAddress{ 0x4 };  // uint32.
    }

    // Size: 0x8.
    namespace GlobalTagTableEntry
    {
        constexpr std::size_t k_TagGroupMagic{ 0x0 };  // int32.
        constexpr std::size_t k_DatumIndex{ 0x4 };     // uint32.
    }

    // Size: 0x8.
    namespace TagInteropTableEntry
    {
        constexpr std::size_t k_Pointer{ 0x0 };    // uint32.
        constexpr std::size_t k_Type{ 0x4 };       // int32.
    }

    namespace MapGlobalsMeta
    {
        namespace Languages
        {
            // Array data.
            constexpr std::size_t k_Base{ 0x18 };
            constexpr std::size_t k_EntrySize{ 0x50 };
            constexpr std::size_t k_Count{ 12 };

            constexpr std::size_t k_StringCount{ 0x0 };            // uint32.
            constexpr std::size_t k_LocaleTableSize{ 0x4 };        // uint32.
            constexpr std::size_t k_LocaleIndexTableOffset{ 0x8 }; // uint32.
            constexpr std::size_t k_LocaleDataIndexOffset{ 0xC };  // uint32.
            
            constexpr std::size_t k_IndexTableHash{ 0x10 };
            constexpr std::size_t k_IndexTableHashSize{ 0x14 };

            constexpr std::size_t k_StringDataHash{ 0x24 };
            constexpr std::size_t k_StringDataHashSize{ 0x14 };

            constexpr std::size_t EntryOffset(std::size_t index)
            {
                return k_Base + (index * k_EntrySize);
            }
        }
    }

    // Size: 0x8.
    namespace LocaleIndexTableElement
    {
        constexpr std::size_t k_StringID{ 0x0 };   // uint32.
        constexpr std::size_t k_Offset{ 0x4 };     // uint32.
    }

    // Size: 0xC.
    namespace TagBlock
    {
        constexpr std::size_t k_EntryCount{ 0x0 }; // int32.
        constexpr std::size_t k_Pointer{ 0x4 };    // uint32.
    }

    // Size: 0x10.
    namespace TagReference
    {
        constexpr std::size_t k_TagGroupMagic{ 0x0 };  // int32.
        constexpr std::size_t k_DatumIndex{ 0xC };     // uint32.
    }

    // Size: 0x14.
    namespace DataReference
    {
        constexpr std::size_t k_Size{ 0x0 };       // int32.
        constexpr std::size_t k_Pointer{ 0xC };    // uint32.
    }

    // Multilingual Unicode String List.
    namespace Unic
    {
        namespace LanguageRanges
        {
            // Array data.
            constexpr std::size_t k_Base{ 0x2C };
            constexpr std::size_t k_EntrySize{ 0x4 };
            constexpr std::size_t k_Count{ 12 };

            constexpr std::size_t k_RangeStartIndex{ 0x0 };    // int16.
            constexpr std::size_t k_RangeSize{ 0x2 };          // int16.

            constexpr std::size_t EntryOffset(std::size_t index)
            {
                return k_Base + (index * k_EntrySize);
            }
        }
    }

    // Scenario Structure BSP.
    namespace SBSP
    {
        constexpr std::size_t k_NumberOfInstancedGeometry{ 0x258 };        // int32.
        constexpr std::size_t k_InstancedGeometryTableAddress{ 0x25C };    // uint32.
    }

    // Size: 0x4.
    namespace SBSPInstancedGeometry
    {
        constexpr std::size_t k_NameStringID{ 0x0 };   // int32.
    }
}