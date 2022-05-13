#pragma once

#include "common.h"
#include "ResourceIDs.h"

#include <vector>
#include <string>
#include <sokol_gfx.h>

#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "shaders/main.h"

struct aiNode;
struct aiScene;
struct aiMesh;

// structs
namespace Core
{
	namespace Sound
	{
		// Create non-copyable/movable versions of SoLoud::Wav and SoLoud::WavStream in order to store them in containers.

		struct SoundEffect final : public SoLoud::Wav
		{
			SoundEffect( SoundEffect const& ) = delete;
			SoundEffect& operator=( SoundEffect const& ) = delete;
			SoundEffect( SoLoud::Wav const& ) = delete;
			SoundEffect& operator=( SoLoud::Wav const& ) = delete;

			SoundEffect( SoundEffect&& _o )
			{
				*this = std::move( _o );
			}

			SoundEffect& operator=( SoundEffect&& _o )
			{
				// SoLoud::Wav
				mData = _o.mData;
				mSampleCount = _o.mSampleCount;

				// SoLoud::AudioSource
				mFlags = _o.mFlags;
				mBaseSamplerate = _o.mBaseSamplerate;
				mVolume = _o.mVolume;
				mChannels = _o.mChannels;
				mAudioSourceID = _o.mAudioSourceID;
				m3dMinDistance = _o.m3dMinDistance;
				m3dMaxDistance = _o.m3dMaxDistance;
				m3dAttenuationRolloff = _o.m3dAttenuationRolloff;
				m3dAttenuationModel = _o.m3dAttenuationModel;
				m3dDopplerFactor = _o.m3dDopplerFactor;
				// Filter pointer
				std::memcpy( mFilter, _o.mFilter, sizeof( mFilter ) );
				mSoloud = _o.mSoloud;
				mCollider = _o.mCollider;
				mAttenuator = _o.mAttenuator;
				mColliderData = _o.mColliderData;
				mLoopPoint = _o.mLoopPoint;

				// Assume that the only things we need to change are the destructor relevant stuff
				// i.e. allocated data and soloud reference for stopping the sound.
				_o.mData = nullptr;
				_o.mSoloud = nullptr;
			}

			SoundEffect() = default;
		};

		struct Music final : public SoLoud::WavStream
		{
			Music( Music const& ) = delete;
			Music& operator=( Music const& ) = delete;
			Music( SoLoud::WavStream const& ) = delete;
			Music& operator=( SoLoud::WavStream const& ) = delete;

			Music( Music&& _o )
			{
				*this = std::move( _o );
			}

			Music& operator=( Music&& _o )
			{
				// SoLoud::WavStream
				mFiletype = _o.mFiletype;
				mFilename = _o.mFilename;
				mMemFile = _o.mMemFile;
				mStreamFile = _o.mStreamFile;
				mSampleCount = _o.mSampleCount;

				// SoLoud::AudioSource
				mFlags = _o.mFlags;
				mBaseSamplerate = _o.mBaseSamplerate;
				mVolume = _o.mVolume;
				mChannels = _o.mChannels;
				mAudioSourceID = _o.mAudioSourceID;
				m3dMinDistance = _o.m3dMinDistance;
				m3dMaxDistance = _o.m3dMaxDistance;
				m3dAttenuationRolloff = _o.m3dAttenuationRolloff;
				m3dAttenuationModel = _o.m3dAttenuationModel;
				m3dDopplerFactor = _o.m3dDopplerFactor;
				// Filter pointer
				std::memcpy( mFilter, _o.mFilter, sizeof( mFilter ) );
				mSoloud = _o.mSoloud;
				mCollider = _o.mCollider;
				mAttenuator = _o.mAttenuator;
				mColliderData = _o.mColliderData;
				mLoopPoint = _o.mLoopPoint;

				// Assume that the only things we need to change are the destructor relevant stuff
				// i.e. allocated data and soloud reference for stopping the sound.
				_o.mFilename = nullptr;
				_o.mMemFile = nullptr;
				_o.mSoloud = nullptr;
			}

			Music() = default;
		};
	}

	namespace Resource
	{
		//-------------------------------------------------
		struct TextureData
		{
			enum class Type
			{
				Diffuse,
				Specular,
				Normal,
				Cubemap,
				General2D, // essentially, unclassified, but definitely 2D
			};

			Type m_type;
			int m_width;
			int m_height;
			std::string m_path;
		};

		//-------------------------------------------------
		struct VertexData
		{
			Vec3 position;
			Vec3 normal;
			Vec2 uv{ 0.0f, 0.0f };
			Vec3 tangent;
		};
		using IndexType = uint32;

		//-------------------------------------------------
		using MaterialData = main_material_t;

		struct MeshData
		{
			int m_indexCount{ 0 };
			MaterialData m_material;
			std::vector<TextureID> m_textures;
			sg_bindings m_bindings{};

			int NumToDraw() const { return m_indexCount; }
			void SetNumToDraw(int _indexCount) { m_indexCount = _indexCount; }
		};

		//-------------------------------------------------
		struct ModelData
		{
			// for now just stores meshes, no transform tree
			std::vector<MeshData> m_meshes;
			std::string m_path;

#if DEBUG_TOOLS
			std::string _traceName_vBufData;
			std::string _traceName_iBufData;
#endif
		};

		//-------------------------------------------------
		struct SpriteData
		{
			std::string m_path;
			TextureID m_texture;
			Vec2 m_dimensions;
			Vec2 m_dimensionsUV;
			Vec2 m_topLeftUV;
			bool m_useAlpha{ false };
		};

		//-------------------------------------------------
		struct SoundEffectData
		{
			std::string m_path;
			Sound::SoundEffect m_sound;
		};

		struct MusicData
		{
			std::string m_path;
			Sound::Music m_music;
		};
	}
}

// functions
namespace Core::Resource
{
	void Init();
	void SetupData();
	void Cleanup();

	TextureData const& GetTexture(TextureID _texture);
	ModelData const& GetModel(ModelID _model);
	SpriteData const& GetSprite(SpriteID _sprite);
	SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect);
	MusicData& GetMusic(MusicID _music);

	// make sure all load users check for success
	struct [[nodiscard]] ResourceLoadResult
	{
		bool const success{ false };
		ResourceLoadResult(bool _success) : success{_success} {}
		operator bool() const { return success; }
	};

	ResourceLoadResult Load2DTexture(std::string const& _path, TextureID& o_textureID, TextureData::Type _type, bool* o_semitransparent = nullptr);
	ResourceLoadResult LoadModel(std::string const& _path, ModelID& o_modelID);
	ResourceLoadResult LoadCubemap(std::string const& _folderPath, TextureID& o_cubemapID);
	ResourceLoadResult LoadSprite(std::string const& _path, SpriteID& o_spriteID);
	ResourceLoadResult LoadSoundEffect(std::string const& _path, SoundEffectID& o_soundEffectID);
	ResourceLoadResult LoadMusic(std::string const& _path, MusicID& o_musicID);
}