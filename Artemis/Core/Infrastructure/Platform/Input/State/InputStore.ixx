export module Platform.Input.State;

import Platform.Input.Type;
import std;

export namespace Platform::Input::State
{
	class InputStore
	{
	private:
		using Request = Platform::Input::Type::Request;
		using Context = Platform::Input::Type::Context;
		using Action = Platform::Input::Type::Action;

	public:
		InputStore() = default;
		~InputStore() = default;

		auto GetNextRequest() const -> Request;
		auto IsProcessing() const -> bool;

		auto SetNextRequest(Context context, Action action) -> void;
		auto SetProcessing(bool processing) -> void;

		auto EnqueueRequest(const Request& request, bool uniqueRequest = false) -> void;
		auto DequeueRequest(Request& outRequest) -> bool;

		auto Cleanup() -> void;

	private:
		std::atomic<bool> m_IsProcessing{ false };

		std::queue<Request> m_Queue{};
		Request m_NextRequest{ Context::Unknown, Action::Unknown };
		mutable std::mutex m_Mutex{};
	};
}