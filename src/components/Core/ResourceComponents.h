#pragma once

#include "common.h"

namespace Core::Resource
{
	struct Preload
	{
		enum class FileType
		{
			Model,
			Texture2D,
			Cubemap,
			Sprite,
			SFX,
			BGM,
		};

		struct FileToLoad
		{
			std::string m_filePath;
			FileType m_fileType;
		};

		enum class State
		{
			LoadingScreenDraw,
			FillFilesList,
			Loading,
		};

		State m_preloadState{ State::LoadingScreenDraw };
		std::optional<std::string> m_firstResFile;
		std::vector<FileToLoad> m_filesToLoad;
		usize m_currentLoadingIndex{ 0 };
	};
}