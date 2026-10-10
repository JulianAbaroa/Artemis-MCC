export module Relations.ObjectGraph.Type;

import std;

export namespace Relations::ObjectGraph::Type
{
	struct ObjectNode
	{
		std::uint32_t Handle;
		std::uint32_t ParentHandle;
		std::vector<std::uint32_t> ChildrenHandles;
	};
}