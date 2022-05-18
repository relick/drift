#pragma once

#include "common.h"

#include "managers/ResourceIDs.h"
#include <sokol_gfx.h>

#include "Enums.h"

#include <array>
#include <optional>
#include <string>

namespace Core::Render
{

	// Help:
	// Bindings: mapping of data buffers and images to a shader's slots
	// Pipeline: shader + buffer layout description + shader parameters
	// Pass: set of render target images (depth and/or colour)

	// i.e.
	// WHAT data? bindings/passglue
	// HOW do we process that data? pipeline/renderer
	// WHERE do we output that processed data? pass

	//--------------------------------------------------------------------------------
	class Pass
	{
		sg_pass m_pass{};
		sg_pass_action m_defaultAction{};
		bool m_valid{ false };
		std::optional<Resource::TextureSampleID> m_depthTarget{};
		std::array<std::optional<Resource::TextureSampleID>, 4> m_colTargets{};

		void Destroy();

	public:
		struct DepthDetail
		{
			bool use_depth{ false };
			bool sampled{ false };
			bool linear_filter{ false };
		};

		Pass
		(
			int _w,
			int _h,
			DepthDetail _depthDetail,
			uint8 _numCol,
			std::string const& _debugName
		);

		~Pass();

		Pass(Pass const& _o) = delete;
		Pass& operator=(Pass const& _o) = delete;

		Pass( Pass&& _o ) noexcept;

		Pass& operator=( Pass&& _o ) noexcept;

		void SetPassAction( sg_pass_action const& _action );

		bool Begin();
		bool Begin( sg_pass_action const& _action );

		Resource::TextureSampleID GetColourImage( uint8 _i ) const;

		Resource::TextureSampleID GetDepthImage() const;
	};

	//--------------------------------------------------------------------------------
	class PassGlue
	{
		sg_bindings m_bindings{};
		bool m_valid{ false };
		int m_numToDraw{ 0 };

#if DEBUG_TOOLS
		std::array<bool, e_Pass_Count + 1> m_validPasses{ false };
		std::array<bool, e_Renderer_Count + 1> m_validRenderers{ false };
#endif
	public:
		PassGlue( sg_bindings const& _binds, int _numToDraw );

		bool Set( e_Pass _currentPass, e_Renderer _currentRenderer );

		int NumToDraw() const { return m_numToDraw; }

		void AddValidPass( e_Pass _pass );
		void AddValidRenderer( e_Renderer _renderer );
	};

	//--------------------------------------------------------------------------------
	class Renderer
	{
		sg_pipeline m_pipeline{};
		bool m_valid{ false };

#if DEBUG_TOOLS
		std::array<bool, e_Pass_Count + 1> m_validPasses{ false };
#endif

	public:
		Renderer( sg_pipeline const& _pipeline );

		bool Activate( e_Pass _currentPass );

		void AddValidPass( e_Pass _pass );
		void AllowGeneralBindings();
		constexpr bool CanUseGeneralBindings() const
		{
#if DEBUG_TOOLS
			return m_validPasses[ e_Pass_Count ];
#else
			return true;
#endif
		}
	};
}