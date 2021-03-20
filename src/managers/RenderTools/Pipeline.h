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

	class Pass
	{
		sg_pass m_pass{};
		sg_pass_action m_defaultAction{};
		bool m_valid{ false };
		std::optional<Resource::TextureSampleID> m_depthTarget{};
		std::array<std::optional<Resource::TextureSampleID>, 4> m_colTargets{};

		void Destroy()
		{
			sg_destroy_pass(m_pass);
			m_pass = sg_pass{};
			m_valid = false;
		}

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
			int _numCol,
			std::string const& _debugName
		)
		{
			sg_pass_desc passDesc{};
#if DEBUG_TOOLS
			std::string const passDebugStr = _debugName + "-pass";
			passDesc.label = passDebugStr.c_str();
#endif

			if (_depthDetail.use_depth)
			{
				sg_image_desc depthTargetDesc{
					.type = SG_IMAGETYPE_2D,
					.render_target = true,
					.width = _w,
					.height = _h,
					.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
					.wrap_u = SG_WRAP_CLAMP_TO_BORDER,
					.wrap_v = SG_WRAP_CLAMP_TO_BORDER,
					.border_color = SG_BORDERCOLOR_OPAQUE_WHITE,
					.will_be_sampled = _depthDetail.sampled,
				};
				if (_depthDetail.linear_filter)
				{
					depthTargetDesc.min_filter = SG_FILTER_LINEAR;
					depthTargetDesc.mag_filter = SG_FILTER_LINEAR;
				}
#if DEBUG_TOOLS
				std::string const depthDebugStr = _debugName + "-depth";
				depthTargetDesc.label = depthDebugStr.c_str();
#endif
				m_depthTarget = sg_make_image(depthTargetDesc);

				passDesc.depth_stencil_attachment = { .image = m_depthTarget->GetValue(), .mip_level = 0 };
			}

			for (int i = 0; i < _numCol && i < 4; ++i)
			{
				sg_image_desc colTargetDesc{
					.type = SG_IMAGETYPE_2D,
					.render_target = true,
					.width = _w,
					.height = _h,
				};
#if DEBUG_TOOLS
				std::string const colDebugStr = _debugName + "-colour[" + std::to_string(i) + "]";
				colTargetDesc.label = colDebugStr.c_str();
#endif
				m_colTargets[i] = sg_make_image(colTargetDesc);

				passDesc.color_attachments[i] = { .image = m_colTargets[i]->GetValue(), .mip_level = 0 };
			}

			m_valid = _depthDetail.use_depth || _numCol > 0;
			if (m_valid)
			{
				m_pass = sg_make_pass(passDesc);
			}
		}

		~Pass()
		{
			Destroy();
		}

		Pass(Pass const& _o) = delete;
		Pass& operator=(Pass const& _o) = delete;

		Pass(Pass&& _o) noexcept
			: m_pass{ _o.m_pass }
			, m_defaultAction{ _o.m_defaultAction }
			, m_valid{ _o.m_valid }
			, m_depthTarget{ _o.m_depthTarget }
			, m_colTargets{ _o.m_colTargets }
		{
			_o.m_pass = sg_pass{};
			_o.m_defaultAction = sg_pass_action{};
			_o.m_valid = false;
			_o.m_depthTarget = Resource::TextureSampleID{};
			_o.m_colTargets = { Resource::TextureSampleID{} };
		}

		Pass& operator=(Pass&& _o) noexcept
		{
			Destroy();

			m_pass = _o.m_pass;
			m_defaultAction = _o.m_defaultAction;
			m_valid = _o.m_valid;
			m_depthTarget = _o.m_depthTarget;
			m_colTargets = _o.m_colTargets;

			_o.m_pass = sg_pass{};
			_o.m_defaultAction = sg_pass_action{};
			_o.m_valid = false;
			_o.m_depthTarget = Resource::TextureSampleID{};
			_o.m_colTargets = { Resource::TextureSampleID{} };
			return *this;
		}

		////
		void SetPassAction(sg_pass_action const& _action)
		{
			m_defaultAction = _action;
		}

		bool Begin()
		{
			return Begin(m_defaultAction);
		}
		bool Begin(sg_pass_action const& _action)
		{
			if (m_valid)
			{
				sg_begin_pass(m_pass, _action);
				return true;
			}
			return false;
		}

		Resource::TextureSampleID GetColourImage(uint8 _i) const
		{
			kaAssert(_i < 4);
			kaAssert(m_colTargets[_i].has_value());
			return *m_colTargets[_i];
		}

		Resource::TextureSampleID GetDepthImage() const
		{
			kaAssert(m_depthTarget.has_value());
			return *m_depthTarget;
		}
	};

	class PassGlue
	{
		sg_bindings m_bindings{};
		bool m_valid{ false };
		uint32 m_numToDraw{ 0 };

#if DEBUG_TOOLS
		std::array<bool, e_Pass_Count + 1> m_validPasses{ false };
		std::array<bool, e_Renderer_Count + 1> m_validRenderers{ false };
#endif
	public:
		PassGlue(sg_bindings const& _binds, uint32 _numToDraw)
			: m_bindings{ _binds }
			, m_valid{ true }
			, m_numToDraw{ _numToDraw }
		{}

		bool Set(e_Pass _currentPass, e_Renderer _currentRenderer)
		{
#if DEBUG_TOOLS
			kaAssert(m_validPasses[_currentPass]);
			kaAssert(m_validRenderers[_currentRenderer]);
			if (!m_validPasses[_currentPass] || !m_validRenderers[_currentRenderer])
			{
				return false;
			}
#endif
			if (m_valid)
			{
				sg_apply_bindings(m_bindings);
				return true;
			}
			return false;
		}

		uint32 NumToDraw() const { return m_numToDraw; }

		void AddValidPass(e_Pass _pass)
		{
#if DEBUG_TOOLS
			m_validPasses[_pass] = true;
#endif
		}
		void AddValidRenderer(e_Renderer _renderer)
		{
#if DEBUG_TOOLS
			m_validRenderers[_renderer] = true;
#endif
		}
	};

	class Renderer
	{
		sg_pipeline m_pipeline{};
		bool m_valid{ false };

#if DEBUG_TOOLS
		std::array<bool, e_Pass_Count + 1> m_validPasses{ false };
#endif

	public:
		Renderer(sg_pipeline const& _pipeline)
			: m_pipeline{ _pipeline }
			, m_valid{ true }
		{}

		bool Activate(e_Pass _currentPass)
		{
#if DEBUG_TOOLS
			kaAssert(m_validPasses[_currentPass]);
			if (!m_validPasses[_currentPass])
			{
				return false;
			}
#endif
			if (m_valid)
			{
				sg_apply_pipeline(m_pipeline);
				return true;
			}
			return false;
		}

		void AddValidPass(e_Pass _pass)
		{
#if DEBUG_TOOLS
			m_validPasses[_pass] = true;
#endif
		}
		void AllowGeneralBindings()
		{
#if DEBUG_TOOLS
			m_validPasses[e_Pass_Count] = true;
#endif
		}
		constexpr bool CanUseGeneralBindings() const
		{
#if DEBUG_TOOLS
			return m_validPasses[e_Pass_Count];
#else
			return true;
#endif
		}
	};
}