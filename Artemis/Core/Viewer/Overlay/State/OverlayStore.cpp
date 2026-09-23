module Viewer.Overlay.State;

namespace Viewer::Overlay::State
{
	auto OverlayStore::GetMode() const -> Mode
	{
		return static_cast<Mode>(m_Mode.load(std::memory_order_relaxed));
	}

	auto OverlayStore::NextMode() -> void
	{
		const auto count = static_cast<std::uint8_t>(Mode::Count);
		const auto current = m_Mode.load(std::memory_order_relaxed);

		m_Mode.store(static_cast<std::uint8_t>((current + 1) % count), std::memory_order_relaxed);
		m_Page.store(0, std::memory_order_relaxed);
	}

	auto OverlayStore::PreviousMode() -> void
	{
		const auto count = static_cast<std::uint8_t>(Mode::Count);
		const auto current = m_Mode.load(std::memory_order_relaxed);

		m_Mode.store(static_cast<std::uint8_t>((current + count - 1) % count), std::memory_order_relaxed);
		m_Page.store(0, std::memory_order_relaxed);
	}

	auto OverlayStore::ResetMode() -> void
	{
		m_Mode.store(static_cast<std::uint8_t>(Mode::Default), std::memory_order_relaxed);
		m_Page.store(0, std::memory_order_relaxed);
	}

	auto OverlayStore::GetPage() const -> int
	{
		return m_Page.load(std::memory_order_relaxed);
	}

	auto OverlayStore::NextPage() -> void
	{
		m_Page.fetch_add(1, std::memory_order_relaxed);
	}

	auto OverlayStore::PreviousPage() -> void
	{
		const int page = m_Page.load(std::memory_order_relaxed);
		if (page > 0) m_Page.store(page - 1, std::memory_order_relaxed);
	}

	auto OverlayStore::ResetPage() -> void
	{
		m_Page.store(0, std::memory_order_relaxed);
	}

	auto OverlayStore::ClampPage(int maxPage) -> void
	{
		const int page = m_Page.load(std::memory_order_relaxed);

		m_Page.store(std::clamp(page, 0, (std::max)(maxPage, 0)), std::memory_order_relaxed);
	}
}