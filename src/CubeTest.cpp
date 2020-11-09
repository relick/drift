#include "CubeTest.h"

#include <fstream>
#include <sstream>

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <ecs/ecs.h>
#include "managers/EntityManager.h"
#include "components.h"
#include "systems.h"

#include "HandmadeMath.h"

#include "shaders/phong.h"
#include "shaders/unlit.h"

struct
{
	sg_pipeline pip;
	sg_bindings bind;
	sg_pipeline lightCubePip;
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

void setup_cube()
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
	sg_shader shd = sg_make_shader(phong_sg_shader_desc());
	sg_shader unlit_shd = sg_make_shader(unlit_sg_shader_desc());


	/* create pipeline object */
	sg_layout_desc layoutdesc{};
	/* test to provide buffer stride, but no attr offsets */
	layoutdesc.buffers[0].stride = 6 * sizeof(float);
	layoutdesc.attrs[ATTR_phong_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
	layoutdesc.attrs[ATTR_phong_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3;

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
	unlitlayoutdesc.attrs[ATTR_unlit_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
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

	CubeTestState.bind = binds;

	Core::EntityID ground = Core::CreateEntity();
	Core::AddComponent(ground, Core::Transform(fQuat::getIdentity(), fVec3(0.0f, -2.0f, 0.0f)));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/groundcube.obj";
		Core::AddComponent(ground, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxDimensions = btVector3(50.0f, 1.0f, 50.0f);
		rbDesc.m_mass = 0.0f;
		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, -2, 0));
		rbDesc.m_startTransform = groundTransform;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(ground, rbDesc);
	}


	Core::EntityID cube = Core::CreateEntity();
	Core::AddComponent(cube, Core::Transform(fQuat::getIdentity(), fVec3(0.0f, 0.0f, 0.0f)));
	Core::AddComponent(cube, Core::Render::CubeTest{false, 0.0f, 0.0f});
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxDimensions = btVector3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 1.0f;
		rbDesc.m_isKinematic = true;
		btTransform cubeTransform;
		cubeTransform.setIdentity();
		cubeTransform.setOrigin(btVector3(0, 0, 0));
		rbDesc.m_startTransform = cubeTransform;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube, rbDesc);
	}

	Core::EntityID cube2 = Core::CreateEntity();
	Core::AddComponent(cube2, Core::Transform(fQuat::getIdentity(), fVec3(-0.5f, 1.5f, 0.0f)));
	Core::AddComponent(cube2, Core::Render::CubeTest{false, 0.0f, 0.0f});
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube2, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxDimensions = btVector3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 1.0f;
		btTransform cubeTransform;
		cubeTransform.setIdentity();
		cubeTransform.setOrigin(btVector3(-0.5f, 1.5f, 0.0f));
		rbDesc.m_startTransform = cubeTransform;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube2, rbDesc);
	}

	Core::EntityID backpack = Core::CreateEntity();
	Core::AddComponent(backpack, Core::Transform(fQuat::getIdentity(), fVec3(-0.5f, 1.5f, 0.0f)));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/backpack/backpack.obj";
		Core::AddComponent(backpack, modelDesc);
	}

	Core::EntityID lightCube = Core::CreateEntity();
	Core::AddComponent(lightCube, Core::Transform(fQuat::getIdentity(), fVec3(1.2f, 1.0f, 2.0f)));
	Core::AddComponent(lightCube, Core::Render::CubeTest{ true, 0.0f, 0.0f });
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3Data(1.0f, 0.0f, 0.0f);
		lightComponent.m_intensity = 1.0f;
		lightComponent.m_direction = fVec3Data(-1.0f, -1.0f, 0.0f);
		lightComponent.m_type = Core::Render::Light::Type::Directional;
		Core::AddComponent(lightCube, lightComponent);
	}
	Core::EntityID lightCube2 = Core::CreateEntity();
	Core::AddComponent(lightCube2, Core::Transform(lightCube));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3Data(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 5.0f;
		lightComponent.m_direction = fVec3Data(0.0f, 0.0f, -1.0f);
		lightComponent.m_attenuation = fVec3Data(1.0f, 0.07f, 0.18f);
		lightComponent.m_type = Core::Render::Light::Type::Spotlight;
		Core::AddComponent(lightCube2, lightComponent);
	}
	Core::EntityID lightCube3 = Core::CreateEntity();
	Core::AddComponent(lightCube3, Core::Transform(lightCube2));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3Data(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 0.3f;
		lightComponent.m_type = Core::Render::Light::Type::Ambient;
		Core::AddComponent(lightCube3, lightComponent);
	}

	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Render::CubeTest& _cubeTest, Core::Transform& _t)
	{
		if (_cubeTest.isLightCube)
		{
			_cubeTest.rx += _fd.dt;
			_cubeTest.ry = sin(_cubeTest.rx);
			_t.T().getOrigin().setX(_cubeTest.ry);
		}
		else
		{
			_cubeTest.rx += 1.0f * _fd.dt;
			_cubeTest.ry += 2.0f * _fd.dt;
			_t.T().getBasis().setEulerYPR(_cubeTest.ry, 0.0f, _cubeTest.rx);
		}
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _t, Core::Render::DefaultPass_Tag)
	{
		camera_state.proj = HMM_Perspective(60.0f, _rfd.fW / _rfd.fH, 0.01f, 1000.0f);

		hmm_mat4 const cameraMat = HMM_Mat4FromfTrans(_t.T());
		camera_state.view = HMM_InverseNoScale(cameraMat);
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_PASSES>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::FrameData const& _fd, Core::Render::CubeTest const& _cubeTest, Core::Transform const& _t)
	{
		fVec3 const& pos = _t.T().getOrigin();
		hmm_vec3 const lightPos = HMM_Vec3(pos.x(), pos.y(), pos.z());

		if (_cubeTest.isLightCube)
		{
			hmm_mat4 lightCubeModel = HMM_Translate(lightPos);
			lightCubeModel = lightCubeModel * HMM_Scale(HMM_Vec3(0.2f, 0.2f, 0.2f));

			unlit_vs_params_t vs_params;
			vs_params.view_model = camera_state.view * lightCubeModel;
			vs_params.projection = camera_state.proj;

			sg_apply_pipeline(CubeTestState.lightCubePip);
			sg_apply_bindings(&CubeTestState.bind);
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_unlit_vs_params, &vs_params, sizeof(vs_params));
			sg_draw(0, 36, 1);
		}
	});
}