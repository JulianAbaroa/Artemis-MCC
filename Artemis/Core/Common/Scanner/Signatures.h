#pragma once

#include "Core/Types/Infrastructure/SignatureTypes.h"

namespace Signatures
{
	// Functions: Lifecycle.
	const Signature GameEngineStart = {
		"GameEngineStart",
		"48 89 5C 24 08 57 48 83 EC 30 49 8B F8 48 8D 0D ? ? ? ? 48 8B DA 41 B8 ? ? ? ? 48 8B D7 E8"
	};

	const Signature EngineInitialize = {
		"EngineInitialize",
		"48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 B7 FE FF FF FF",
	};

	const Signature DestroySubsystems = {
		"DestroySubsystems",
		"40 56 57 41 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 68 48 89 6C 24 70 33 F6",
	};

	// Functions: Map.
	const Signature BlamOpenMap = {
		"BlamOpenMap",
		"48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 80 FD FF FF 48 81 EC 80 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 02 00 00 33 FF"
	};

	// Functions: Objects & Players.
	const Signature TelemetryIdModifier = {
		"TelemetryIdModifier",
		"8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 48 8B 04 C8",
	};

	const Signature CreateObject = {
		"CreateObject",
		"40 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 8B 41 ?? 49 83 CC"
	};

	const Signature ReleaseObject = {
		"ReleaseObjectByHandle",
		"48 89 5C 24 08 57 48 83 EC 20 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 8B D9"
	};

	const Signature CreatePlayer = {
		"CreatePlayer",
		"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 48 8B FA 65 48 8B 04 25 58 00 00 00 8B D1 41 8A E8 4A 8B 34 C8 B8 18 00 00 00 48 8B 0C 30 E8 ?? ?? ?? ?? 8B D8 83 F8 FF 74"
	};

	// Functions: High-level Data.
	const Signature BuildGameEvent = {
		"BuildGameEvent",
		"48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
	};

	// Gametype...

	// Functions: Input Injection.
	const Signature GetButtonState = {
		"GetButtonState",
		"?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F BF D1 83 EA 69 74 ?? 83 EA 01 74 ?? 83 EA 01 74 ?? 83 FA 01 74"
	};

	// MouseState...

	// Maybe we need them, maybe not.
	//const Signature BlamOpenFile = {
	//	"BlamOpenFile",
	//	"48 89 5C 24 10 55 56 57 48 81 EC 50 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 40 03 00 00",
	//};

	// const Signature SpawnSceneryObject = {
	// 	"SpawnSceneryObject",
	// 	"48 89 5C 24 10 48 89 74 24 18 55 57 41 54 41 56 41 57 48 8D AC 24 30 FF FF FF 48 81 EC D0 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 C8 00 00 00 48 8B 05 ?? ?? ?? ?? 48 8D 35 ?? ?? ?? ?? 41 83 CF FF 4C 63 F1 41 8B DF 44 8B 80 8C 00 00 00"
	// };
}