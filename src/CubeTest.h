#pragma once

#include <ecs/ecs.h>
#include "components.h"

static const char* vs_src =
"#version 330\n"
"uniform mat4 mvp;\n"
"in vec4 position;\n"
"in vec4 color0;\n"
"out vec4 color;\n"
"void main() {\n"
"  gl_Position = mvp * position;\n"
"  color = color0;\n"
"}\n";
static const char* fs_src =
"#version 330\n"
"in vec4 color;\n"
"out vec4 frag_color;\n"
"void main() {\n"
"  frag_color = color;\n"
"}\n";

typedef struct
{
	hmm_mat4 mvp;
} vs_params_t;

struct
{
	sg_pipeline pip;
	sg_bindings bind;
} CubeTestState;

namespace Core
{
	namespace Render
	{
		struct CubeTest
		{
			float rx{ 0.0f };
			float ry{ 0.0f };
		};
	}
}

void setup_cube()
{

	/* cube vertex buffer */
	float vertices[] = {
		-1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
		 1.0, -1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
		 1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,
		-1.0,  1.0, -1.0,   1.0, 0.0, 0.0, 1.0,

		-1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
		 1.0, -1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
		 1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,
		-1.0,  1.0,  1.0,   0.0, 1.0, 0.0, 1.0,

		-1.0, -1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
		-1.0,  1.0, -1.0,   0.0, 0.0, 1.0, 1.0,
		-1.0,  1.0,  1.0,   0.0, 0.0, 1.0, 1.0,
		-1.0, -1.0,  1.0,   0.0, 0.0, 1.0, 1.0,

		1.0, -1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
		1.0,  1.0, -1.0,    1.0, 0.5, 0.0, 1.0,
		1.0,  1.0,  1.0,    1.0, 0.5, 0.0, 1.0,
		1.0, -1.0,  1.0,    1.0, 0.5, 0.0, 1.0,

		-1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,
		-1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
		 1.0, -1.0,  1.0,   0.0, 0.5, 1.0, 1.0,
		 1.0, -1.0, -1.0,   0.0, 0.5, 1.0, 1.0,

		-1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0,
		-1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
		 1.0,  1.0,  1.0,   1.0, 0.0, 0.5, 1.0,
		 1.0,  1.0, -1.0,   1.0, 0.0, 0.5, 1.0
	};

	sg_buffer_desc cubeverts{};
	cubeverts.size = sizeof(vertices);
	cubeverts.content = vertices;
	cubeverts.label = "cube-vertices";

	sg_buffer vbuf = sg_make_buffer(&cubeverts);

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
	posdesc.name = "position";
	posdesc.sem_name = "POS";
	sg_shader_attr_desc coldesc{};
	coldesc.name = "color0";
	coldesc.sem_name = "COLOR";
	shaderdesc.attrs[0] = posdesc;
	shaderdesc.attrs[1] = coldesc;
	sg_shader_uniform_block_desc unifblockdesc{};
	unifblockdesc.size = sizeof(vs_params_t);

	sg_shader_uniform_desc unifdesc{};
	unifdesc.name = "mvp";
	unifdesc.type = SG_UNIFORMTYPE_MAT4;
	unifblockdesc.uniforms[0] = unifdesc;

	shaderdesc.vs.uniform_blocks[0] = unifblockdesc;
	shaderdesc.vs.source = vs_src;
	shaderdesc.fs.source = fs_src;
	shaderdesc.label = "cube-shader";

	sg_shader shd = sg_make_shader(&shaderdesc);

	/* create pipeline object */
	sg_layout_desc layoutdesc{};
	/* test to provide buffer stride, but no attr offsets */
	layoutdesc.buffers[0].stride = 28;
	layoutdesc.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
	layoutdesc.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;

	sg_pipeline_desc pipelinedesc{};
	pipelinedesc.layout = layoutdesc;
	pipelinedesc.shader = shd;
	pipelinedesc.index_type = SG_INDEXTYPE_UINT16;
	pipelinedesc.depth_stencil = {
		.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
		.depth_write_enabled = true,
	};
	pipelinedesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
	pipelinedesc.label = "cube-pipeline";

	CubeTestState.pip = sg_make_pipeline(&pipelinedesc);

	/* setup resource bindings */
	sg_bindings binds{};
	binds.vertex_buffers[0] = vbuf;
	binds.index_buffer = ibuf;

	CubeTestState.bind = binds;

	ecs::entity_id cube = Core::CreateEntity();
	Core::AddComponent(cube, Core::Transform(fQuat::getIdentity(), LoadVec3(0.0f, 1.0f, 0.0f)));
	Core::AddComponent(cube, Core::Render::CubeTest{});

	ecs::entity_id cube2 = Core::CreateEntity();
	Core::AddComponent(cube2, Core::Transform(fQuat::getIdentity(), LoadVec3(0.0f, -1.0f, 0.0f)));
	Core::AddComponent(cube2, Core::Render::CubeTest{});

	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Render::CubeTest& _cubeTest, Core::Transform& _t)
	{
		_cubeTest.rx += 1.0f * _fd.dt;
		_cubeTest.ry += 2.0f * _fd.dt;
		_t.T().getBasis().setEulerYPR(_cubeTest.ry, 0.0f, _cubeTest.rx);
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS>, ecs::opts::not_parallel>([](Core::Render::FrameData& _fd, Core::Render::CubeTest& _cubeTest, Core::Transform& _t)
	{
		vs_params_t vs_params;
		hmm_mat4 proj = HMM_Perspective(60.0f, _fd.fW / _fd.fH, 0.01f, 10.0f);
		fVec3 const& pos = _t.T().getOrigin();
		fQuat quat;
		_t.T().getBasis().getRotation(quat);

		hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 1.5f, 6.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
		hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);

		hmm_vec3 position = HMM_Vec3(pos.x, pos.y, pos.z);
		hmm_mat4 translation = HMM_Translate(position);
		hmm_quaternion quaternion = HMM_Quaternion(quat.x, quat.y, quat.z, quat.w);
		hmm_mat4 rotation = HMM_QuaternionToMat4(quaternion);

		vs_params.mvp = HMM_MultiplyMat4(view_proj, HMM_MultiplyMat4(translation, rotation));

		sg_apply_pipeline(CubeTestState.pip);
		sg_apply_bindings(&CubeTestState.bind);
		sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
		sg_draw(0, 36, 1);
	});
}