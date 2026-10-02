export module Resolved.Definitions.State;

import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::State
{
    class DefinitionsStore
    {
    private:
        // Damage
        using ResolvedJpt = Resolved::Definitions::Type::Jpt::Jpt;

        // Level
        using ResolvedSbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
        using ResolvedScnr = Resolved::Definitions::Type::Scnr::Scnr;

        // Model
        using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
        using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using ResolvedMode = Resolved::Definitions::Type::Mode::Mode;
        using ResolvedPhmo = Resolved::Definitions::Type::Phmo::Phmo;

        // Object
        using ResolvedBipd = Resolved::Definitions::Type::Bipd::Bipd;
        using ResolvedBloc = Resolved::Definitions::Type::Bloc::Bloc;
        using ResolvedCtrl = Resolved::Definitions::Type::Ctrl::Ctrl;
        using ResolvedEqip = Resolved::Definitions::Type::Eqip::Eqip;
        using ResolvedMach = Resolved::Definitions::Type::Mach::Mach;
        using ResolvedProj = Resolved::Definitions::Type::Proj::Proj;
        using ResolvedScen = Resolved::Definitions::Type::Scen::Scen;
        using ResolvedVehi = Resolved::Definitions::Type::Vehi::Vehi;
        using ResolvedWeap = Resolved::Definitions::Type::Weap::Weap;

    public:
        DefinitionsStore() = default;
        ~DefinitionsStore() = default;

        // --- Damage ---

        // Jpt
        auto HasResolvedJpt(const std::string& tagName) const -> bool;
        auto GetResolvedJpt(const std::string& tagName) const -> const ResolvedJpt*;
        auto AddResolvedJpt(const std::string& tagName, ResolvedJpt data) -> void;
        auto GetAllResolvedJpts() const -> const std::unordered_map<std::string, ResolvedJpt>&;

        // --- Level ---

        // Sbsp
        auto HasResolvedSbsp(const std::string& tagName) const -> bool;
        auto GetResolvedSbsp(const std::string& tagName) const -> const ResolvedSbsp*;
        auto AddResolvedSbsp(const std::string& tagName, ResolvedSbsp data) -> void;
        auto GetAllResolvedSbsps() const -> const std::unordered_map<std::string, ResolvedSbsp>&;

        // Scnr
        auto HasResolvedScnr(const std::string& tagName) const -> bool;
        auto GetResolvedScnr(const std::string& tagName) const -> const ResolvedScnr*;
        auto AddResolvedScnr(const std::string& tagName, ResolvedScnr data) -> void;
        auto GetAllResolvedScnrs() const -> const std::unordered_map<std::string, ResolvedScnr>&;

        // --- Model ---

        // Coll
        auto HasResolvedColl(const std::string& tagName) const -> bool;
        auto GetResolvedColl(const std::string& tagName) const -> const ResolvedColl*;
        auto AddResolvedColl(const std::string& tagName, ResolvedColl data) -> void;
        auto GetAllResolvedColls() const -> const std::unordered_map<std::string, ResolvedColl>&;

        // Hlmt
        auto HasResolvedHlmt(const std::string& tagName) const -> bool;
        auto GetResolvedHlmt(const std::string& tagName) const -> const ResolvedHlmt*;
        auto AddResolvedHlmt(const std::string& tagName, ResolvedHlmt data) -> void;
        auto GetAllResolvedHlmts() const -> const std::unordered_map<std::string, ResolvedHlmt>&;

        // Mode
        auto HasResolvedMode(const std::string& tagName) const -> bool;
        auto GetResolvedMode(const std::string& tagName) const -> const ResolvedMode*;
        auto AddResolvedMode(const std::string& tagName, ResolvedMode data) -> void;
        auto GetAllResolvedModes() const -> const std::unordered_map<std::string, ResolvedMode>&;

        // Phmo
        auto HasResolvedPhmo(const std::string& tagName) const -> bool;
        auto GetResolvedPhmo(const std::string& tagName) const -> const ResolvedPhmo*;
        auto AddResolvedPhmo(const std::string& tagName, ResolvedPhmo data) -> void;
        auto GetAllResolvedPhmos() const -> const std::unordered_map<std::string, ResolvedPhmo>&;

        // --- Object ---

        // Bipd
        auto HasResolvedBipd(const std::string& tagName) const -> bool;
        auto GetResolvedBipd(const std::string& tagName) const -> const ResolvedBipd*;
        auto AddResolvedBipd(const std::string& tagName, ResolvedBipd data) -> void;
        auto GetAllResolvedBipds() const -> const std::unordered_map<std::string, ResolvedBipd>&;

        // Bloc
        auto HasResolvedBloc(const std::string& tagName) const -> bool;
        auto GetResolvedBloc(const std::string& tagName) const -> const ResolvedBloc*;
        auto AddResolvedBloc(const std::string& tagName, ResolvedBloc data) -> void;
        auto GetAllResolvedBlocs() const -> const std::unordered_map<std::string, ResolvedBloc>&;

        // Ctrl
        auto HasResolvedCtrl(const std::string& tagName) const -> bool;
        auto GetResolvedCtrl(const std::string& tagName) const -> const ResolvedCtrl*;
        auto AddResolvedCtrl(const std::string& tagName, ResolvedCtrl data) -> void;
        auto GetAllResolvedCtrls() const -> const std::unordered_map<std::string, ResolvedCtrl>&;

        // Eqip
        auto HasResolvedEqip(const std::string& tagName) const -> bool;
        auto GetResolvedEqip(const std::string& tagName) const -> const ResolvedEqip*;
        auto AddResolvedEqip(const std::string& tagName, ResolvedEqip data) -> void;
        auto GetAllResolvedEqips() const -> const std::unordered_map<std::string, ResolvedEqip>&;

        // Mach
        auto HasResolvedMach(const std::string& tagName) const -> bool;
        auto GetResolvedMach(const std::string& tagName) const -> const ResolvedMach*;
        auto AddResolvedMach(const std::string& tagName, ResolvedMach data) -> void;
        auto GetAllResolvedMachs() const -> const std::unordered_map<std::string, ResolvedMach>&;

        // Proj
        auto HasResolvedProj(const std::string& tagName) const -> bool;
        auto GetResolvedProj(const std::string& tagName) const -> const ResolvedProj*;
        auto AddResolvedProj(const std::string& tagName, ResolvedProj data) -> void;
        auto GetAllResolvedProjs() const -> const std::unordered_map<std::string, ResolvedProj>&;

        // Scen
        auto HasResolvedScen(const std::string& tagName) const -> bool;
        auto GetResolvedScen(const std::string& tagName) const -> const ResolvedScen*;
        auto AddResolvedScen(const std::string& tagName, ResolvedScen data) -> void;
        auto GetAllResolvedScens() const -> const std::unordered_map<std::string, ResolvedScen>&;

        // Vehi
        auto HasResolvedVehi(const std::string& tagName) const -> bool;
        auto GetResolvedVehi(const std::string& tagName) const -> const ResolvedVehi*;
        auto AddResolvedVehi(const std::string& tagName, ResolvedVehi data) -> void;
        auto GetAllResolvedVehis() const -> const std::unordered_map<std::string, ResolvedVehi>&;

        // Weap
        auto HasResolvedWeap(const std::string& tagName) const -> bool;
        auto GetResolvedWeap(const std::string& tagName) const -> const ResolvedWeap*;
        auto AddResolvedWeap(const std::string& tagName, ResolvedWeap data) -> void;
        auto GetAllResolvedWeaps() const -> const std::unordered_map<std::string, ResolvedWeap>&;

        auto IsFrozen() const -> bool
        {
            return m_Frozen.load(std::memory_order_acquire);
        }

        auto Freeze() -> void
        {
            m_Frozen.store(true, std::memory_order_release);
        }

        auto Cleanup() -> void;

    private:
        // Damage
        std::unordered_map<std::string, ResolvedJpt> m_ResolvedJpts{};

        // Level
        std::unordered_map<std::string, ResolvedSbsp> m_ResolvedSbsps{};
        std::unordered_map<std::string, ResolvedScnr> m_ResolvedScnrs{};

        // Model
        std::unordered_map<std::string, ResolvedColl> m_ResolvedColls{};
        std::unordered_map<std::string, ResolvedHlmt> m_ResolvedHlmts{};
        std::unordered_map<std::string, ResolvedMode> m_ResolvedModes{};
        std::unordered_map<std::string, ResolvedPhmo> m_ResolvedPhmos{};

        // Object
        std::unordered_map<std::string, ResolvedBipd> m_ResolvedBipds{};
        std::unordered_map<std::string, ResolvedBloc> m_ResolvedBlocs{};
        std::unordered_map<std::string, ResolvedCtrl> m_ResolvedCtrls{};
        std::unordered_map<std::string, ResolvedEqip> m_ResolvedEqips{};
        std::unordered_map<std::string, ResolvedMach> m_ResolvedMachs{};
        std::unordered_map<std::string, ResolvedProj> m_ResolvedProjs{};
        std::unordered_map<std::string, ResolvedScen> m_ResolvedScens{};
        std::unordered_map<std::string, ResolvedVehi> m_ResolvedVehis{};
        std::unordered_map<std::string, ResolvedWeap> m_ResolvedWeaps{};

        std::atomic<bool> m_Frozen{ false };
    };
}