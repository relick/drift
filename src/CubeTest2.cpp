#include "CubeTest2.h"

#include <fstream>
#include <sstream>

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <ecs/ecs.h>
#include "components.h"
#include "systems.h"
#include "managers/EntityManager.h"

#define HANDMADE_MATH_CPP_MODE
#include "HandmadeMath.h"

struct PhongVSParams
{
	hmm_mat4 model;
	hmm_mat4 normal;
	hmm_mat4 view;
	hmm_mat4 projection;
};

struct UnlitVSParams
{
	hmm_mat4 model;
	hmm_mat4 view;
	hmm_mat4 projection;
};

struct PhongFSParams
{
	hmm_vec3 objectColor;
	hmm_vec3 lightColor;
	hmm_vec3 lightPos;
};

struct
{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pipeline lightCubePip;
	sg_bindings lightCubeBind;
} CubeTestState;

namespace Core
{
	namespace Render
	{
		struct CubeTest
		{
			bool isLightCube;
			float rx{ 0.0f };
			float ry{ 0.0f };
		};
	}
}

struct
{
	hmm_mat4 proj{ HMM_Perspective(60.0f, 640.0f / 480.0f, 0.01f, 10.0f) };
	hmm_mat4 view{ HMM_LookAt(HMM_Vec3(1.4f, 1.5f, 4.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f)) };
} camera_state;

void setup_cube2()
{
	/* cube vertex buffer */
	float vertices[] = {
		-1.0, -1.0, -1.0,  0.0f,  0.0f, -1.0f,
		 1.0, -1.0, -1.0,  0.0f,  0.0f, -1.0f,
		 1.0,  1.0, -1.0,  0.0f,  0.0f, -1.0f,
		-1.0,  1.0, -1.0,  0.0f,  0.0f, -1.0f,

		-1.0, -1.0,  1.0,  0.0f,  0.0f, 1.0f,
		 1.0, -1.0,  1.0,  0.0f,  0.0f, 1.0f,
		 1.0,  1.0,  1.0,  0.0f,  0.0f, 1.0f,
		-1.0,  1.0,  1.0,  0.0f,  0.0f, 1.0f,

		-1.0, -1.0, -1.0, -1.0f,  0.0f,  0.0f,
		-1.0,  1.0, -1.0, -1.0f,  0.0f,  0.0f,
		-1.0,  1.0,  1.0, -1.0f,  0.0f,  0.0f,
		-1.0, -1.0,  1.0, -1.0f,  0.0f,  0.0f,

		1.0, -1.0, -1.0,  1.0f,  0.0f,  0.0f,
		1.0,  1.0, -1.0,  1.0f,  0.0f,  0.0f,
		1.0,  1.0,  1.0,  1.0f,  0.0f,  0.0f,
		1.0, -1.0,  1.0,  1.0f,  0.0f,  0.0f,

		-1.0, -1.0, -1.0,  0.0f, -1.0f,  0.0f,
		-1.0, -1.0,  1.0,  0.0f, -1.0f,  0.0f,
		 1.0, -1.0,  1.0,  0.0f, -1.0f,  0.0f,
		 1.0, -1.0, -1.0,  0.0f, -1.0f,  0.0f,

		-1.0,  1.0, -1.0,  0.0f,  1.0f,  0.0f,
		-1.0,  1.0,  1.0,  0.0f,  1.0f,  0.0f,
		 1.0,  1.0,  1.0,  0.0f,  1.0f,  0.0f,
		 1.0,  1.0, -1.0,  0.0f,  1.0f,  0.0f,
	};

	sg_buffer_desc cubeverts{};
	cubeverts.size = sizeof(vertices);
	cubeverts.content = vertices;
	cubeverts.label = "cube-vertices";

	sg_buffer vbuf = sg_make_buffer(&cubeverts);
	sg_buffer lightCubevbuf = sg_make_buffer(&cubeverts);

	/* create an index buffer for the cube */
	uint16_t indices[] = {
		0, 1, 2,  0, 2, 3,
		6, 5, 4,  7, 6, 4,
		8, 9, 10,  8, 10, 11,
		14, 13, 12,  15, 14, 12,
		16, 17, 18,  16, 18, 19,
		22, 21, 20,  23, 22, 20
	};

	sg_buffer_desc cubeindices{};
	cubeindices.type = SG_BUFFERTYPE_INDEXBUFFER;
	cubeindices.size = sizeof(indices);
	cubeindices.content = indices;
	cubeindices.label = "cube-indices";

	sg_buffer ibuf = sg_make_buffer(&cubeindices);

	/* create shader */
	sg_shader_desc shaderdesc{};
	sg_shader_attr_desc posdesc{};
	posdesc.name = "aPos";
	posdesc.sem_name = "POSITION";
	shaderdesc.attrs[0] = posdesc;
	sg_shader_attr_desc normaldesc{};
	normaldesc.name = "aNormal";
	normaldesc.sem_name = "NORMAL";
	shaderdesc.attrs[1] = normaldesc;
	sg_shader_uniform_block_desc vsunifblockdesc{};
	vsunifblockdesc.size = sizeof(PhongVSParams);

	vsunifblockdesc.uniforms[0].name = "model";
	vsunifblockdesc.uniforms[0].type = SG_UNIFORMTYPE_MAT4;
	vsunifblockdesc.uniforms[1].name = "normal";
	vsunifblockdesc.uniforms[1].type = SG_UNIFORMTYPE_MAT4;
	vsunifblockdesc.uniforms[2].name = "view";
	vsunifblockdesc.uniforms[2].type = SG_UNIFORMTYPE_MAT4;
	vsunifblockdesc.uniforms[3].name = "projection";
	vsunifblockdesc.uniforms[3].type = SG_UNIFORMTYPE_MAT4;

	std::string vs_src;
	{
		std::ifstream file{ "src/shaders/phong.vert" };
		vs_src = static_cast<std::ostringstream&&>(std::ostringstream{} << file.rdbuf()).str();
	}

	sg_shader_uniform_block_desc fsunifblockdesc{};
	fsunifblockdesc.size = sizeof(PhongFSParams);

	fsunifblockdesc.uniforms[0].name = "objectColor";
	fsunifblockdesc.uniforms[0].type = SG_UNIFORMTYPE_FLOAT3;
	fsunifblockdesc.uniforms[1].name = "lightColor";
	fsunifblockdesc.uniforms[1].type = SG_UNIFORMTYPE_FLOAT3;
	fsunifblockdesc.uniforms[2].name = "lightPos";
	fsunifblockdesc.uniforms[2].type = SG_UNIFORMTYPE_FLOAT3;

	std::string fs_src;
	{
		std::ifstream file{ "src/shaders/phong.frag" };
		fs_src = static_cast<std::ostringstream&&>(std::ostringstream{} << file.rdbuf()).str();
	}

	shaderdesc.vs.uniform_blocks[0] = vsunifblockdesc;
	shaderdesc.vs.source = vs_src.c_str();
	shaderdesc.fs.uniform_blocks[0] = fsunifblockdesc;
	shaderdesc.fs.source = fs_src.c_str();
	shaderdesc.label = "phong-shader";

	sg_shader shd = sg_make_shader(&shaderdesc);


	std::string unlit_vs_src;
	{
		std::ifstream file{ "src/shaders/unlit.vert" };
		unlit_vs_src = static_cast<std::ostringstream&&>(std::ostringstream{} << file.rdbuf()).str();
	}

	std::string unlit_fs_src;
	{
		std::ifstream file{ "src/shaders/unlit.frag" };
		unlit_fs_src = static_cast<std::ostringstream&&>(std::ostringstream{} << file.rdbuf()).str();
	}

	sg_shader_uniform_block_desc unlitVsUnifBlockDesc{};
	unlitVsUnifBlockDesc.size = sizeof(UnlitVSParams);
	unlitVsUnifBlockDesc.uniforms[0].name = "model";
	unlitVsUnifBlockDesc.uniforms[0].type = SG_UNIFORMTYPE_MAT4;
	unlitVsUnifBlockDesc.uniforms[1].name = "view";
	unlitVsUnifBlockDesc.uniforms[1].type = SG_UNIFORMTYPE_MAT4;
	unlitVsUnifBlockDesc.uniforms[2].name = "projection";
	unlitVsUnifBlockDesc.uniforms[2].type = SG_UNIFORMTYPE_MAT4;

	sg_shader_desc unlitShaderDesc{};
	unlitShaderDesc.attrs[0] = posdesc;
	unlitShaderDesc.vs.uniform_blocks[0] = unlitVsUnifBlockDesc;
	unlitShaderDesc.vs.source = unlit_vs_src.c_str();
	unlitShaderDesc.fs.source = unlit_fs_src.c_str();
	unlitShaderDesc.label = "unlit-shader";

	sg_shader unlit_shd = sg_make_shader(&unlitShaderDesc);


	/* create pipeline object */
	sg_layout_desc layoutdesc{};
	/* test to provide buffer stride, but no attr offsets */
	layoutdesc.buffers[0].stride = 6 * sizeof(float);
	layoutdesc.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
	layoutdesc.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;

	sg_pipeline_desc pipelinedesc{};
	pipelinedesc.layout = layoutdesc;
	pipelinedesc.shader = shd;
	pipelinedesc.index_type = SG_INDEXTYPE_UINT16;
	pipelinedesc.depth_stencil = {
		.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
		.depth_write_enabled = true,
	};
	pipelinedesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
	pipelinedesc.label = "phong-pipeline";

	CubeTestState.pip = sg_make_pipeline(&pipelinedesc);

	sg_pipeline_desc unlitPipelineDesc{};
	sg_layout_desc unlitlayoutdesc{};
	/* test to provide buffer stride, but no attr offsets */
	unlitlayoutdesc.buffers[0].stride = 6 * sizeof(float);
	unlitlayoutdesc.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
	unlitPipelineDesc.layout = unlitlayoutdesc;
	unlitPipelineDesc.shader = unlit_shd;
	unlitPipelineDesc.index_type = SG_INDEXTYPE_UINT16;
	unlitPipelineDesc.depth_stencil = {
		.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
		.depth_write_enabled = true,
	};
	unlitPipelineDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
	unlitPipelineDesc.label = "unlit-pipeline";

	CubeTestState.lightCubePip = sg_make_pipeline(&unlitPipelineDesc);

	/* setup resource bindings */
	sg_bindings binds{};
	binds.vertex_buffers[0] = vbuf;
	binds.index_buffer = ibuf;

	sg_bindings lightCubeBinds{};
	lightCubeBinds.vertex_buffers[0] = lightCubevbuf;
	lightCubeBinds.index_buffer = ibuf;

	CubeTestState.bind = binds;
	CubeTestState.lightCubeBind = lightCubeBinds;

	Core::EntityID cube = Core::CreateEntity();
	ecs::add_component(cube.GetValue(), Core::Transform(fQuat::getIdentity(), LoadVec3(0.0f, 0.0f, 0.0f)));
	ecs::add_component(cube.GetValue(), Core::Render::CubeTest{false, 0.0f, 0.0f});

	Core::EntityID cube2 = Core::CreateEntity();
	ecs::add_component(cube2.GetValue(), Core::Transform(fQuat::getIdentity(), LoadVec3(-1.0f, 1.0f, 0.0f), cube));
	ecs::add_component(cube2.GetValue(), Core::Render::CubeTest{false, 0.0f, 0.0f});

	Core::EntityID lightCube = Core::CreateEntity();
	ecs::add_component(lightCube.GetValue(), Core::Transform(fQuat::getIdentity(), LoadVec3(1.2f, 1.0f, 2.0f)));
	ecs::add_component(lightCube.GetValue(), Core::Render::CubeTest{ true, 0.0f, 0.0f });

	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Render::CubeTest& _cubeTest, Core::Transform& _t)
	{
		_cubeTest.rx += 1.0f * _fd.dt;
		_cubeTest.ry += 2.0f * _fd.dt;
		_t.T().getBasis().setEulerYPR(_cubeTest.ry, 0.0f, _cubeTest.rx);
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _t, Core::Render::DefaultPass_Tag)
	{
		camera_state.proj = HMM_Perspective(60.0f, _rfd.fW / _rfd.fH, 0.01f, 10.0f);

		fVec3 const& pos = _t.T().getOrigin();
		hmm_vec3 position = HMM_Vec3(pos.x, pos.y, pos.z);
		fQuat quat;
		_t.T().getBasis().getRotation(quat);
		hmm_quaternion quaternion = HMM_Quaternion(quat.x, quat.y, quat.z, quat.w);
		hmm_mat4 rotation = HMM_QuaternionToMat4(quaternion);

		camera_state.view = HMM_InverseNoScale(HMM_Translate(position) * rotation);
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::FrameData const& _fd, Core::Render::CubeTest const& _cubeTest, Core::Transform const& _t)
	{
		hmm_vec3 lightPos = HMM_Vec3(1.2f, 1.0f, 2.0f);

		PhongFSParams fs_params;
		fs_params.objectColor = HMM_Vec3(1.0f, 0.5f, 0.31f);
		fs_params.lightColor = HMM_Vec3(1.0f, 1.0f, 1.0f);
		fs_params.lightPos = (camera_state.view * HMM_Vec4v(lightPos, 1.0f)).XYZ;

		if (!_cubeTest.isLightCube)
		{
			PhongVSParams vs_params;
			fTrans const cubeTrans = _t.CalculateWorldTransform();
			fVec3 const& pos = cubeTrans.getOrigin();
			fQuat quat;
			cubeTrans.getBasis().getRotation(quat);


			hmm_vec3 position = HMM_Vec3(pos.x, pos.y, pos.z);
			hmm_mat4 translation = HMM_Translate(position);
			hmm_quaternion quaternion = HMM_Quaternion(quat.x, quat.y, quat.z, quat.w);
			hmm_mat4 rotation = HMM_QuaternionToMat4(quaternion);

			vs_params.model = translation * rotation;
			vs_params.model = vs_params.model * HMM_Scale(HMM_Vec3(0.5f, 0.5f, 0.5f));
			vs_params.view = camera_state.view;
			vs_params.normal = HMM_Transpose(HMM_Inverse(vs_params.view * vs_params.model));
			vs_params.projection = camera_state.proj;

			sg_apply_pipeline(CubeTestState.pip);
			sg_apply_bindings(&CubeTestState.bind);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
			sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fs_params, sizeof(fs_params));
			sg_draw(0, 36, 1);
		}
		else
		{
			hmm_mat4 lightCubeModel = HMM_Translate(lightPos);
			lightCubeModel = lightCubeModel * HMM_Scale(HMM_Vec3(0.2f, 0.2f, 0.2f));

			UnlitVSParams vs_params;
			vs_params.model = lightCubeModel;
			vs_params.view = camera_state.view;
			vs_params.projection = camera_state.proj;

			sg_apply_pipeline(CubeTestState.lightCubePip);
			sg_apply_bindings(&CubeTestState.lightCubeBind);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
			sg_draw(0, 36, 1);
		}
	});
}