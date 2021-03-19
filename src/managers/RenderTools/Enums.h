#pragma once

namespace Core::Render
{
	enum e_Pass
	{
		Pass_DirectionalLight,
		Pass_MainTarget,
		Pass_RenderToScreen,

		e_Pass_Count,
		e_DefaultPass = Pass_RenderToScreen,
	};
	enum e_PassGlue
	{
		PassGlue_MainTarget_To_Screen,

		e_PassGlue_Count,
	};
	enum e_Renderer
	{
		Renderer_Main,
		Renderer_TargetToScreen,
		Renderer_DepthOnly,
		Renderer_Skybox,
		Renderer_Sprites,

		e_Renderer_Count,
	};
}