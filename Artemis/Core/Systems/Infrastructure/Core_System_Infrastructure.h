#pragma once

#include <memory>

class System_Dialog;
class System_Format;
class System_Input;
class System_Lifecycle;
class System_Render;
class System_Scanner;
class System_Thread;

class System_Preferences;
class System_Settings;

struct Core_System_Infrastructure
{
	Core_System_Infrastructure();
	~Core_System_Infrastructure();

	// Capture
	//std::unique_ptr<AudioSystem> Audio;
	//std::unique_ptr<DownloadSystem> Download;
	//std::unique_ptr<FFmpegSystem> FFmpeg;
	//std::unique_ptr<ProcessSystem> Process;
	//std::unique_ptr<PipeSystem> Pipe;
	//std::unique_ptr<SyncSystem> Sync;
	//std::unique_ptr<VideoSystem> Video;

	// Engine
	std::unique_ptr<System_Dialog> Dialog;
	std::unique_ptr<System_Format> Format;
	std::unique_ptr<System_Input> Input;
	std::unique_ptr<System_Lifecycle> Lifecycle;
	std::unique_ptr<System_Render> Render;
	std::unique_ptr<System_Scanner> Scanner;
	std::unique_ptr<System_Thread> Thread;

	// Persistence
	//std::unique_ptr<GallerySystem> Gallery;
	std::unique_ptr<System_Preferences> Preferences;
	//std::unique_ptr<ReplaySystem> Replay;
	std::unique_ptr<System_Settings> Settings;
};