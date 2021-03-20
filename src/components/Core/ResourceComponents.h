#pragma once

#include "common.h"

namespace Core::Resource
{
	struct Preload
	{
		bool m_preloadStarted{ false };
		std::vector<std::string> m_filesToLoad;
		usize m_currentLoadingIndex{ 0 };
	};
}