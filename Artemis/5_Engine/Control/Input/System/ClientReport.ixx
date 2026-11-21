export module Engine.Control.Input.System:ClientReport;

import Control.Client.Type;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::UnitContext;

    using Engine::Control::Input::Type::ClientCounts;
    using Engine::Control::Input::Type::ClientDifferences;
    using Engine::Control::Input::Type::ShadowField;
    using Engine::Control::Input::Type::k_ClientStateFieldNames;
}

export namespace Engine::Control::Input::System
{
    // Decides which counters a compared call increments.
    // note: A call that read a fact of the unit the context does not know (UnknownRead) only goes to ClientPartial: its differences may
    // come from the missing fact, so they do not count as bugs of the port in the fields. Only fully known calls fill the fields.
    // param isZoomed: The client of the user had a zoom level at the entry of the call.
    auto CountClient(const ClientStep& step, const ClientDifferences& differences, const UnitContext& context, bool isZoomed) -> ClientCounts
    {
        ClientCounts counts{};
        if (step.Output.UnknownRead != 0)
        {
            counts.Add(ShadowField::ClientPartial, differences.IsEmpty());
            return counts;
        }

        counts.Add(ShadowField::ClientMove, !differences.IsMove);
        counts.Add(ShadowField::ClientLook, !differences.IsLook);
        counts.Add(ShadowField::ClientTriggers, !differences.IsTriggers);
        counts.Add(ShadowField::ClientButtons, differences.Buttons == 0);
        counts.Add(ShadowField::ClientEvents, differences.Events == 0);
        counts.Add(ShadowField::ClientLanes, differences.Lanes == 0);
        counts.Add(ShadowField::ClientState, differences.StateFields == 0);

        ShadowField contextField{ ShadowField::ClientOnFoot };
        if (!context.HasUnit) contextField = ShadowField::ClientNoUnit;
        else if (context.IsSeated) contextField = ShadowField::ClientSeated;
        counts.Add(contextField, differences.IsEmpty());
        if (isZoomed) counts.Add(ShadowField::ClientZoomed, differences.IsEmpty());
        return counts;
    }

    // return: The names of the state fields that differ, each after a space, or " none".
    auto FormatClientStateFields(const ClientDifferences& differences) -> std::string
    {
        std::string text{};
        for (std::size_t field{ 0 }; field < k_ClientStateFieldNames.size(); ++field)
        {
            if ((differences.StateFields & (1u << field)) != 0) std::format_to(std::back_inserter(text), " {}", k_ClientStateFieldNames[field]);
        }

        return text.empty() ? " none" : text;
    }

    // return: One line of the oracle file, without the line break: what an offline test needs to replay the call (the dt, the table and the
    // client block at the entry) and what differed. unknown is the mask of unknown facts the step read, zero for a fully known call.
    auto FormatClientLine(std::int32_t inputUser, float deltaTime, const ClientDifferences& differences, std::uint32_t unknown,
        std::span<const std::uint8_t> table, std::span<const std::uint8_t> blockBefore) -> std::string
    {
        auto hex = [](std::span<const std::uint8_t> bytes) {
            std::string text{};
            text.reserve(bytes.size() * 2);
            for (const std::uint8_t byte : bytes) std::format_to(std::back_inserter(text), "{:02X}", byte);
            return text;
        };

        return std::format("diff user={} dt={:08X} unknown={:X} move={} look={} trig={} tail={} buttons={:X} events={:X} lanes={:X} state={:X} table={} block={}",
            inputUser, std::bit_cast<std::uint32_t>(deltaTime), unknown, differences.IsMove, differences.IsLook, differences.IsTriggers,
            differences.IsTail, differences.Buttons, differences.Events, differences.Lanes, differences.StateFields, hex(table), hex(blockBefore));
    }
}