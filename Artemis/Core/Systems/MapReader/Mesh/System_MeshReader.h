// This geometry extraction logic is derived from Reclaimer
//   (https://github.com/Gravemind2401/Reclaimer) by Gravemind2401,
//   licensed under GPL-3.0. The mesh-access theory, buffer layout,
//   unstrip algorithm and instance transform handling were ported
//   from Reclaimer's C# implementation to C++.
// See MeshReader.md for the full Assembly <-> Reclaimer name mapping.

#pragma once

#include <vector>

struct SbspGeometry;

class State_MapReader;
class State_MapScnr;
class State_MapSbsp;
class State_MapLbsp;
class State_MapPlay;
class State_MapZone;
class System_MapReader;
class System_Logs;

struct System_MeshReader_Dependencies
{
    State_MapReader& State_MapReader;
    State_MapScnr& State_MapScnr;
    State_MapSbsp& State_MapSbsp;
    State_MapLbsp& State_MapLbsp;
    State_MapPlay& State_MapPlay;
    State_MapZone& State_MapZone;
    System_MapReader& System_MapReader;
    System_Logs& System_Logs;
};

class System_MeshReader
{
public:
    System_MeshReader(System_MeshReader_Dependencies dependencies) : 
        m_Deps(dependencies) {}
    ~System_MeshReader() = default;

    bool ReadSbspGeometries(std::vector<SbspGeometry>& outGeometries);

private:
    System_MeshReader_Dependencies m_Deps;
};