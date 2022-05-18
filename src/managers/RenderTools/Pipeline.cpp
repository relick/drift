#include "Pipeline.h"

namespace Core::Render
{

//--------------------------------------------------------------------------------
void Pass::Destroy()
{
	sg_destroy_pass(m_pass);
	m_pass = sg_pass{};
	m_valid = false;
}

//--------------------------------------------------------------------------------
Pass::Pass
(
	int _w,
	int _h,
	DepthDetail _depthDetail,
	uint8 _numCol,
	std::string const& _debugName
)
{
	sg_pass_desc passDesc{};
#if DEBUG_TOOLS
	std::string const passDebugStr = _debugName + "-pass";
	passDesc.label = passDebugStr.c_str();
#endif

	if ( _depthDetail.use_depth )
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
		if ( _depthDetail.linear_filter )
		{
			depthTargetDesc.min_filter = SG_FILTER_LINEAR;
			depthTargetDesc.mag_filter = SG_FILTER_LINEAR;
		}
#if DEBUG_TOOLS
		std::string const depthDebugStr = _debugName + "-depth";
		depthTargetDesc.label = depthDebugStr.c_str();
#endif
		m_depthTarget = sg_make_image( depthTargetDesc );

		passDesc.depth_stencil_attachment = { .image = m_depthTarget->GetSokolID(), .mip_level = 0 };
	}

	kaAssert( _numCol <= 4, "More than 4 colour buffers requested, instantiating only 4." );
	for ( uint8 i = 0; i < _numCol && i < 4; ++i )
	{
		sg_image_desc colTargetDesc{
			.type = SG_IMAGETYPE_2D,
			.render_target = true,
			.width = _w,
			.height = _h,
		};
#if DEBUG_TOOLS
		std::string const colDebugStr = _debugName + "-colour[" + std::to_string( i ) + "]";
		colTargetDesc.label = colDebugStr.c_str();
#endif
		m_colTargets[ i ] = sg_make_image( colTargetDesc );

		passDesc.color_attachments[ i ] = { .image = m_colTargets[ i ]->GetSokolID(), .mip_level = 0 };
	}

	m_valid = _depthDetail.use_depth || _numCol > 0;
	if ( m_valid )
	{
		m_pass = sg_make_pass( passDesc );
	}
}

//--------------------------------------------------------------------------------
Pass::~Pass()
{
	Destroy();
}

//--------------------------------------------------------------------------------
Pass::Pass(Pass&& _o) noexcept
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

//--------------------------------------------------------------------------------
Pass& Pass::operator=(Pass&& _o) noexcept
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

//--------------------------------------------------------------------------------
void Pass::SetPassAction(sg_pass_action const& _action)
{
	m_defaultAction = _action;
}

//--------------------------------------------------------------------------------
bool Pass::Begin()
{
	return Begin(m_defaultAction);
}

//--------------------------------------------------------------------------------
bool Pass::Begin(sg_pass_action const& _action)
{
	if (m_valid)
	{
		sg_begin_pass(m_pass, _action);
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------
Resource::TextureSampleID Pass::GetColourImage(uint8 _i) const
{
	kaAssert(_i < 4);
	kaAssert(m_colTargets[_i].has_value());
	return *m_colTargets[_i];
}

//--------------------------------------------------------------------------------
Resource::TextureSampleID Pass::GetDepthImage() const
{
	kaAssert( m_depthTarget.has_value() );
	return *m_depthTarget;
}

//--------------------------------------------------------------------------------
PassGlue::PassGlue(sg_bindings const& _binds, int _numToDraw)
	: m_bindings{ _binds }
	, m_valid{ true }
	, m_numToDraw{ _numToDraw }
{}

//--------------------------------------------------------------------------------
bool PassGlue::Set(e_Pass _currentPass, e_Renderer _currentRenderer)
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

//--------------------------------------------------------------------------------
void PassGlue::AddValidPass(e_Pass _pass)
{
#if DEBUG_TOOLS
	m_validPasses[_pass] = true;
#endif
}

//--------------------------------------------------------------------------------
void PassGlue::AddValidRenderer(e_Renderer _renderer)
{
#if DEBUG_TOOLS
	m_validRenderers[_renderer] = true;
#endif
}

//--------------------------------------------------------------------------------
Renderer::Renderer(sg_pipeline const& _pipeline)
	: m_pipeline{ _pipeline }
	, m_valid{ true }
{}

//--------------------------------------------------------------------------------
bool Renderer::Activate(e_Pass _currentPass)
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

//--------------------------------------------------------------------------------
void Renderer::AddValidPass(e_Pass _pass)
{
#if DEBUG_TOOLS
	m_validPasses[_pass] = true;
#endif
}

//--------------------------------------------------------------------------------
void Renderer::AllowGeneralBindings()
{
#if DEBUG_TOOLS
	m_validPasses[e_Pass_Count] = true;
#endif
}

}