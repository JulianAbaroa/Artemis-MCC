export module Viewer.Selection.System;

import Viewer.Selection.State;
import Viewer.Camera.Type;
import Viewer.Map.Type;
import std;

export namespace Viewer::Selection::System
{
	class SelectionService
	{
	private:
		using SelectionStore = Viewer::Selection::State::SelectionStore;
		using Ray = Viewer::Camera::Type::Ray;
		using ObjectBounds = Viewer::Map::Type::ObjectBounds;

	public:
		explicit SelectionService(SelectionStore& selectionStore) :
			m_SelectionStore(selectionStore) {
		}
		~SelectionService() = default;

		auto Pick(const Ray& ray, std::span<const ObjectBounds> bounds) -> void;

	private:
		SelectionStore& m_SelectionStore;

		static auto Intersect(const Ray& ray, const ObjectBounds& bounds) -> float;
	};
}