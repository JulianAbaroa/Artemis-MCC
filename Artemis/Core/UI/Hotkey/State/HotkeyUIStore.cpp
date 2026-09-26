module UI.Hotkey.State;

namespace UI::Hotkey::State
{
	auto HotkeyUIStore::GetBindings() const -> std::span<const Binding>
	{
		return m_Bindings;
	}

	auto HotkeyUIStore::Find(std::uint32_t virtualKey, bool isShiftDown) const -> const Binding*
	{
		for (const Binding& binding : m_Bindings)
		{
			if (binding.Scope != Scope::UI) continue;
			if (binding.VirtualKey != virtualKey) continue;
			if (binding.RequiresShift && !isShiftDown) continue;

			return &binding;
		}

		return nullptr;
	}
}