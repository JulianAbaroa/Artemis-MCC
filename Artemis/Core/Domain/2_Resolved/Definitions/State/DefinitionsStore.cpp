module;

#include <cassert>

module Resolved.Definitions.State;

namespace Resolved::Definitions::State
{
    // --- Damage ---

    // Jpt
    auto DefinitionsStore::HasResolvedJpt(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedJpts.find(tagName) != m_ResolvedJpts.end();
    }

    auto DefinitionsStore::GetResolvedJpt(const std::string& tagName) const -> const ResolvedJpt*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedJpts.find(tagName);
        if (it == m_ResolvedJpts.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedJpts() const -> const std::unordered_map<std::string, ResolvedJpt>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedJpts;
    }

    auto DefinitionsStore::AddResolvedJpt(const std::string& tagName, ResolvedJpt data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedJpts.emplace(tagName, std::move(data));
    }

    // --- Level ---

    // Sbsp
    auto DefinitionsStore::HasResolvedSbsp(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedSbsps.find(tagName) != m_ResolvedSbsps.end();
    }

    auto DefinitionsStore::GetResolvedSbsp(const std::string& tagName) const -> const ResolvedSbsp*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedSbsps.find(tagName);
        if (it == m_ResolvedSbsps.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedSbsps() const -> const std::unordered_map<std::string, ResolvedSbsp>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedSbsps;
    }

    auto DefinitionsStore::AddResolvedSbsp(const std::string& tagName, ResolvedSbsp data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedSbsps.emplace(tagName, std::move(data));
    }

    // Scnr
    auto DefinitionsStore::HasResolvedScnr(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedScnrs.find(tagName) != m_ResolvedScnrs.end();
    }

    auto DefinitionsStore::GetResolvedScnr(const std::string& tagName) const -> const ResolvedScnr*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedScnrs.find(tagName);
        if (it == m_ResolvedScnrs.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedScnrs() const -> const std::unordered_map<std::string, ResolvedScnr>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedScnrs;
    }

    auto DefinitionsStore::AddResolvedScnr(const std::string& tagName, ResolvedScnr data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedScnrs.emplace(tagName, std::move(data));
    }

    // --- Model ---

    // Coll
    auto DefinitionsStore::HasResolvedColl(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedColls.find(tagName) != m_ResolvedColls.end();
    }

    auto DefinitionsStore::GetResolvedColl(const std::string& tagName) const -> const ResolvedColl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedColls.find(tagName);
        if (it == m_ResolvedColls.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedColls() const -> const std::unordered_map<std::string, ResolvedColl>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedColls;
    }

    auto DefinitionsStore::AddResolvedColl(const std::string& tagName, ResolvedColl data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedColls.emplace(tagName, std::move(data));
    }

    // Hlmt
    auto DefinitionsStore::HasResolvedHlmt(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedHlmts.find(tagName) != m_ResolvedHlmts.end();
    }

    auto DefinitionsStore::GetResolvedHlmt(const std::string& tagName) const -> const ResolvedHlmt*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedHlmts.find(tagName);
        if (it == m_ResolvedHlmts.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedHlmts() const -> const std::unordered_map<std::string, ResolvedHlmt>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedHlmts;
    }

    auto DefinitionsStore::AddResolvedHlmt(const std::string& tagName, ResolvedHlmt data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedHlmts.emplace(tagName, std::move(data));
    }

    // Mode
    auto DefinitionsStore::HasResolvedMode(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedModes.find(tagName) != m_ResolvedModes.end();
    }

    auto DefinitionsStore::GetResolvedMode(const std::string& tagName) const -> const ResolvedMode*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedModes.find(tagName);
        if (it == m_ResolvedModes.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedModes() const -> const std::unordered_map<std::string, ResolvedMode>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedModes;
    }

    auto DefinitionsStore::AddResolvedMode(const std::string& tagName, ResolvedMode data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedModes.emplace(tagName, std::move(data));
    }

    // --- Object ---

    // Bipd
    auto DefinitionsStore::HasResolvedBipd(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedBipds.find(tagName) != m_ResolvedBipds.end();
    }

    auto DefinitionsStore::GetResolvedBipd(const std::string& tagName) const -> const ResolvedBipd*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedBipds.find(tagName);
        if (it == m_ResolvedBipds.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedBipds() const -> const std::unordered_map<std::string, ResolvedBipd>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedBipds;
    }

    auto DefinitionsStore::AddResolvedBipd(const std::string& tagName, ResolvedBipd data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedBipds.emplace(tagName, std::move(data));
    }

    // Bloc
    auto DefinitionsStore::HasResolvedBloc(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedBlocs.find(tagName) != m_ResolvedBlocs.end();
    }

    auto DefinitionsStore::GetResolvedBloc(const std::string& tagName) const -> const ResolvedBloc*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedBlocs.find(tagName);
        if (it == m_ResolvedBlocs.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedBlocs() const -> const std::unordered_map<std::string, ResolvedBloc>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedBlocs;
    }

    auto DefinitionsStore::AddResolvedBloc(const std::string& tagName, ResolvedBloc data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedBlocs.emplace(tagName, std::move(data));
    }

    // Ctrl
    auto DefinitionsStore::HasResolvedCtrl(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedCtrls.find(tagName) != m_ResolvedCtrls.end();
    }

    auto DefinitionsStore::GetResolvedCtrl(const std::string& tagName) const -> const ResolvedCtrl*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedCtrls.find(tagName);
        if (it == m_ResolvedCtrls.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedCtrls() const -> const std::unordered_map<std::string, ResolvedCtrl>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedCtrls;
    }

    auto DefinitionsStore::AddResolvedCtrl(const std::string& tagName, ResolvedCtrl data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedCtrls.emplace(tagName, std::move(data));
    }

    // Eqip
    auto DefinitionsStore::HasResolvedEqip(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedEqips.find(tagName) != m_ResolvedEqips.end();
    }

    auto DefinitionsStore::GetResolvedEqip(const std::string& tagName) const -> const ResolvedEqip*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedEqips.find(tagName);
        if (it == m_ResolvedEqips.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedEqips() const -> const std::unordered_map<std::string, ResolvedEqip>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedEqips;
    }

    auto DefinitionsStore::AddResolvedEqip(const std::string& tagName, ResolvedEqip data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedEqips.emplace(tagName, std::move(data));
    }

    // Mach
    auto DefinitionsStore::HasResolvedMach(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedMachs.find(tagName) != m_ResolvedMachs.end();
    }

    auto DefinitionsStore::GetResolvedMach(const std::string& tagName) const -> const ResolvedMach*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedMachs.find(tagName);
        if (it == m_ResolvedMachs.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedMachs() const -> const std::unordered_map<std::string, ResolvedMach>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedMachs;
    }

    auto DefinitionsStore::AddResolvedMach(const std::string& tagName, ResolvedMach data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedMachs.emplace(tagName, std::move(data));
    }

    // Proj
    auto DefinitionsStore::HasResolvedProj(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedProjs.find(tagName) != m_ResolvedProjs.end();
    }

    auto DefinitionsStore::GetResolvedProj(const std::string& tagName) const -> const ResolvedProj*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedProjs.find(tagName);
        if (it == m_ResolvedProjs.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedProjs() const -> const std::unordered_map<std::string, ResolvedProj>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedProjs;
    }

    auto DefinitionsStore::AddResolvedProj(const std::string& tagName, ResolvedProj data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedProjs.emplace(tagName, std::move(data));
    }

    // Scen
    auto DefinitionsStore::HasResolvedScen(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedScens.find(tagName) != m_ResolvedScens.end();
    }

    auto DefinitionsStore::GetResolvedScen(const std::string& tagName) const -> const ResolvedScen*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedScens.find(tagName);
        if (it == m_ResolvedScens.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedScens() const -> const std::unordered_map<std::string, ResolvedScen>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedScens;
    }

    auto DefinitionsStore::AddResolvedScen(const std::string& tagName, ResolvedScen data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedScens.emplace(tagName, std::move(data));
    }

    // Vehi
    auto DefinitionsStore::HasResolvedVehi(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedVehis.find(tagName) != m_ResolvedVehis.end();
    }

    auto DefinitionsStore::GetResolvedVehi(const std::string& tagName) const -> const ResolvedVehi*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedVehis.find(tagName);
        if (it == m_ResolvedVehis.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedVehis() const -> const std::unordered_map<std::string, ResolvedVehi>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedVehis;
    }

    auto DefinitionsStore::AddResolvedVehi(const std::string& tagName, ResolvedVehi data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedVehis.emplace(tagName, std::move(data));
    }

    // Weap
    auto DefinitionsStore::HasResolvedWeap(const std::string& tagName) const -> bool
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedWeaps.find(tagName) != m_ResolvedWeaps.end();
    }

    auto DefinitionsStore::GetResolvedWeap(const std::string& tagName) const -> const ResolvedWeap*
    {
        assert(m_Frozen.load(std::memory_order_acquire));

        auto it = m_ResolvedWeaps.find(tagName);
        if (it == m_ResolvedWeaps.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    auto DefinitionsStore::GetAllResolvedWeaps() const -> const std::unordered_map<std::string, ResolvedWeap>&
    {
        assert(m_Frozen.load(std::memory_order_acquire));
        return m_ResolvedWeaps;
    }

    auto DefinitionsStore::AddResolvedWeap(const std::string& tagName, ResolvedWeap data) -> void
    {
        assert(!m_Frozen.load(std::memory_order_relaxed));
        m_ResolvedWeaps.emplace(tagName, std::move(data));
    }

    auto DefinitionsStore::Cleanup() -> void
    {
        m_Frozen.store(false, std::memory_order_relaxed);

        // Damage
        m_ResolvedJpts.clear();

        // Level
        m_ResolvedSbsps.clear();
        m_ResolvedScnrs.clear();

        // Model
        m_ResolvedColls.clear();
        m_ResolvedHlmts.clear();
        m_ResolvedModes.clear();

        // Object
        m_ResolvedBipds.clear();
        m_ResolvedBlocs.clear();
        m_ResolvedCtrls.clear();
        m_ResolvedEqips.clear();
        m_ResolvedMachs.clear();
        m_ResolvedProjs.clear();
        m_ResolvedScens.clear();
        m_ResolvedVehis.clear();
        m_ResolvedWeaps.clear();
    }
}