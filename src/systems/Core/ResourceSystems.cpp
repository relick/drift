#include "ResourceSystems.h"

#include "components.h"
#include "SystemOrdering.h"

#include "managers/EntityManager.h"
#include "managers/ResourceManager.h"

#include <absl/container/inlined_vector.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>

#include <fstream>

namespace Core::Resource
{
	void FillFilesToLoad
	(
		std::vector<Core::Resource::Preload::FileToLoad>& o_files
	)
	{
		absl::InlinedVector<std::string, 16> resFileLists;
		absl::InlinedVector<std::string, 16> nextResFileLists;
#if DEBUG_TOOLS
		absl::flat_hash_set<std::string> seenResFiles;
		absl::flat_hash_set<std::string> seenOtherFiles;
#endif

		resFileLists.emplace_back("assets/preload.res");

		while (!resFileLists.empty())
		{
			for (auto const& resFilepath : resFileLists)
			{
				std::string_view const nextResView = resFilepath;
				std::string_view const extension = nextResView.substr(nextResView.find_last_of('.') + 1);
				std::string_view const directory = nextResView.substr(0, nextResView.find_last_of('/') + 1);

				if (extension == "res")
				{
#if DEBUG_TOOLS
					if (seenResFiles.contains(resFilepath))
					{
						kaError("Seen res file " + resFilepath + " multiple times. This will hardlock on final.");
						continue;
					}
					seenResFiles.insert(resFilepath);
#endif
					std::ifstream resFile(resFilepath);
					if (resFile.is_open())
					{
						std::string nextFilename;
						while (std::getline(resFile, nextFilename))
						{
							if (nextFilename.empty() || nextFilename[0] == '#')
							{
								continue;
							}
							nextResFileLists.emplace_back(std::string(directory) + nextFilename);
						}
					}
				}
				else
				{
#if DEBUG_TOOLS
					if (seenOtherFiles.contains(resFilepath))
					{
						kaError("Seen asset " + resFilepath + " multiple times. Won't break, but will load faster if fixed.");
						continue;
					}
					seenOtherFiles.insert(resFilepath);
#endif
					static const absl::flat_hash_map<std::string_view, Preload::FileType> fileTypes {
						{ "obj", Preload::FileType::Model },
						{ "jpg", Preload::FileType::Texture2D },
						{ "jpeg", Preload::FileType::Texture2D },
						{ "png", Preload::FileType::Texture2D },
						{ "cubemap", Preload::FileType::Cubemap },
						{ "spr", Preload::FileType::Sprite },
						{ "wav", Preload::FileType::SFX },
						{ "ogg", Preload::FileType::BGM },
						{ "mp3", Preload::FileType::BGM },
					};

					auto fileTypeI = fileTypes.find(extension);
					if (fileTypeI != fileTypes.end())
					{
						o_files.push_back({ resFilepath, fileTypeI->second, });
					}
					else
					{
						kaError("File " + resFilepath + " is missing a filetype specification!");
					}
				}
			}

			resFileLists = std::move(nextResFileLists);
			nextResFileLists.clear();
		}
	}

	void Setup()
	{
		Core::MakeSystem<Sys::FILE_LOADING>([](Core::EntityID::CoreType _entity, Core::MT_Only&, Core::Resource::Preload& _preload)
		{
			switch (_preload.m_preloadState)
			{
			case Preload::State::LoadingScreenDraw:
			{
				// skip a frame so the screen isn't completely white on the first file.
				_preload.m_preloadState = Preload::State::FillFilesList;
				return;
			}
			case Preload::State::FillFilesList:
			{
				FillFilesToLoad(_preload.m_filesToLoad);
				_preload.m_preloadState = Preload::State::Loading;
				break;
			}
			case Preload::State::Loading:
			{
				break;
			}
			}

			if (_preload.m_currentLoadingIndex == _preload.m_filesToLoad.size())
			{
				Core::RemoveComponent<Core::Resource::Preload>(_entity);
				return;
			}

			// load next file
			std::string const& nextFilePath = _preload.m_filesToLoad[_preload.m_currentLoadingIndex].m_filePath;

			bool success{ false };
			switch (_preload.m_filesToLoad[_preload.m_currentLoadingIndex].m_fileType)
			{
			case Preload::FileType::Model:
			{
				ModelID modelID;
				success = LoadModel(nextFilePath, modelID);
				break;
			}
			case Preload::FileType::Texture2D:
			{
				TextureID textureID;
				success = Load2DTexture(nextFilePath, textureID, TextureData::Type::General2D);
				break;
			}
			case Preload::FileType::Cubemap:
			{
				TextureID cubemapID;
				success = LoadCubemap(nextFilePath, cubemapID);
				break;
			}
			case Preload::FileType::Sprite:
			{
				SpriteID spriteID;
				success = LoadSprite(nextFilePath, spriteID);
				break;
			}
			case Preload::FileType::SFX:
			{
				SoundEffectID sfxID;
				success = LoadSoundEffect(nextFilePath, sfxID);
				break;
			}
			case Preload::FileType::BGM:
			{
				MusicID bgmID;
				success = LoadMusic(nextFilePath, bgmID);
				break;
			}
			}

			_preload.m_currentLoadingIndex++;
		});
	}
}