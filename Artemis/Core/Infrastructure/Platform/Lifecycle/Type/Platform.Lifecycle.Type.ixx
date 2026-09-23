export module Platform.Lifecycle.Type;

export namespace Platform::Lifecycle::Type
{
	enum class Status
	{
		Waiting,
		Initialized,
		Running,
		TearingDown,
		Destroyed,
	};
}