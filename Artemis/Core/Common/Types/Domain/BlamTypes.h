#pragma once

#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

struct ConfigurationNode
{
	void* vtable;
	ConfigurationNode* next;

	union {
		wchar_t local[8];
		wchar_t* remote;
	} text;

	uint64_t length;
	uint64_t capacity;
	int32_t status;
	std::byte _pad[4];
};

struct EngineSpecificData
{
	std::byte _pad_start[755];

	std::byte CustomizationData[256];

	std::byte _pad_end[1753];
};

// TODO: Keep working on this struct.
struct GameConfiguration
{
	int32_t EngineID;
	std::byte _pad1[96];

	int32_t SessionType;
	std::byte _pad2[12];
	int32_t UnknownEngineCheck;
	std::byte _pad_session[4];

	int64_t SessionID;
	std::byte _pad3[36];

	uint8_t GameFlags;
	uint8_t GameFlagsSecondary;
	std::byte _pad4[9];
	char TeamIndex;

	std::byte _pad5[9317];
	EngineSpecificData EngineTable[10];
	std::byte _pad_post_table[25411];

	int32_t MCC_MapID;
	std::byte _pad6[1];
	uint8_t Difficulty;
	std::byte _pad7[182];
	int32_t CheckpointID;

	std::byte _pad_to_sync[21874];
	uint8_t GlobalSessionSyncFlag;
	std::byte _pad_pre_list[88521];

	void* ListSentinel;
	ConfigurationNode* ObjectiveListHead;
	const char* MapPathPtr;

	int32_t Engine_MapID;
	std::byte _pad9[4];

	int32_t LoadingState;
	std::byte _pad10[12];

	std::byte HostConfigInternal[160];
	void* pHostContext;
	void* pSessionContext;

	std::byte _pad_pre_net[112];

	uint8_t NetworkType;
	std::byte _pad11[3];

	uint64_t NetworkSessionID_Low;
	uint64_t NetworkSessionID_High;

	std::byte _pad_final[40];
};

//struct ObjectCreationParameters
//{
//	uint32_t DatumIndex;
//	uint64_t CreationFlags;
//	std::byte _pad1[4];
//
//	uint32_t UnkownHandle1;
//	std::byte _pad2[4];
//
//	uint8_t SpawnFlags1;
//	uint8_t SpawnFlags2;
//	std::byte _pad3[2];
//
//	float Position[3];
//	float Forward[3];
//	float Up[3];
//	float Velocity[3];
//	std::byte _pad4[12];
//
//	float _Scale;
//	uint16_t _TeamIndex;
//	uint16_t _PlayerSlotID;
//
//	uint32_t _UnknownFlags;
//	std::byte _pad5[4];
//
//	// Repeated TeamIndex, PlayerSlotID and UnknownFlags.
//	std::byte _pad6[12];
//
//	// Broken from here...
//
//	uint16_t _OneConstant2;
//	uint32_t PlayerOwnerHandle;
//	std::byte UnknownFlags1[6];
//	std::byte _pad6[2];
//
//	std::byte UnknownFlags2[10];
//	std::byte _pad7[2];
//
//	uint8_t UnknownFlag3;
//	std::byte _pad8[3];
//
//	float PrimaryColor[3];
//	float SecondaryColor[3];
//	std::byte _pad9[24];
//
//	uint8_t UnknownFlag4;
//	std::byte _pad10[3];
//
//	uint32_t UnknownHandle2;
//	uint8_t UnknownFlag5;
//	std::byte _pad11[3];
//
//	uint8_t UnknownFlag6;
//	std::byte _pad12[3];
//
//	uint8_t UnknownFlag7;
//	std::byte _pad13[3];
//
//	uint8_t UnknownFlag8;
//	std::byte _pad14[3];
//
//	uint8_t UnknownFlag9;
//	std::byte _pad15[3];
//
//	uint8_t UnknownFlag10;
//	std::byte _pad16[3];
//
//	uint8_t UnknownFlag11;
//	std::byte _pad17[3];
//
//	uint16_t UnknownFlag12;
//	std::byte _pad18[2];
//
//	uint16_t UnknownFlag13;
//	std::byte _pad19[2];
//
//	uint16_t UnknownFlag14;
//	std::byte _pad20[2];
//
//	uint8_t UnknownFlag15;
//	std::byte _pad21[3];
//
//	uint16_t UnknownFlag16;
//	std::byte _pad22[2];
//
//	uint16_t UnknownFlag17;
//	std::byte _pad23[2];
//
//	std::byte UnknownFlags3[72];
//	uint16_t UnknownFlag18;
//	std::byte _pad24[4];
//
//	uint16_t UnknownFlag19;
//	std::byte _pad25[4];
//
//	uint16_t UnknownFlag20;
//	std::byte _pad26[2];
//
//	std::byte UnknownFlags4[12];
//	std::byte _pad27[8];
//};

#pragma pack(pop)