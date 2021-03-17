#include "CubeTest.h"

#include <fstream>
#include <sstream>

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <ecs/ecs.h>
#include "managers/EntityManager.h"
#include "components.h"
#include "systems.h"

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
	fMat4 proj{ glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 10.0f) };
	fMat4 view{ glm::lookAt(fVec3(1.4f, 1.5f, 4.0f), fVec3(0.0f, 0.0f, 0.0f), fVec3(0.0f, 1.0f, 0.0f)) };
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
	cubeverts.data = SG_RANGE(vertices);
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
	cubeindices.data = SG_RANGE(indices);
	cubeindices.label = "cube-indices";

	sg_buffer ibuf = sg_make_buffer(&cubeindices);

	/* create shader */
	sg_shader shd = sg_make_shader(phong_sg_shader_desc(sg_query_backend()));
	sg_shader unlit_shd = sg_make_shader(unlit_sg_shader_desc(sg_query_backend()));


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
	pipelinedesc.depth = {
		.compare = SG_COMPAREFUNC_LESS_EQUAL,
		.write_enabled = true,
	};
	pipelinedesc.cull_mode = SG_CULLMODE_BACK;
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
	unlitPipelineDesc.depth = {
		.compare = SG_COMPAREFUNC_LESS_EQUAL,
		.write_enabled = true,
	};
	unlitPipelineDesc.cull_mode = SG_CULLMODE_BACK;
	unlitPipelineDesc.label = "unlit-pipeline";

	CubeTestState.lightCubePip = sg_make_pipeline(&unlitPipelineDesc);

	/* setup resource bindings */
	sg_bindings binds{};
	binds.vertex_buffers[0] = vbuf;
	binds.index_buffer = ibuf;

	CubeTestState.bind = binds;

	Core::EntityID ground = Core::CreateEntity();
	fTrans const groundTrans{ fQuatIdentity(), fVec3(0.0f, -2.0f, 0.0f) };
	Core::AddComponent(ground, Core::Transform(groundTrans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/groundcube.obj";
		Core::AddComponent(ground, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = fVec3(50.0f, 1.0f, 50.0f);
		rbDesc.m_mass = 0.0f;
		rbDesc.m_startTransform = groundTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(ground, rbDesc);
	}

	Core::EntityID wall = Core::CreateEntity();
	fTrans const wallTrans{ fMat3(glm::yawPitchRoll(0.0f, glm::radians(90.0f), 0.0f)), fVec3(0.0f, 0.0f, 2.0f) };
	Core::AddComponent(wall, Core::Transform(wallTrans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/groundcube.obj";
		Core::AddComponent(wall, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = fVec3(50.0f, 1.0f, 50.0f);
		rbDesc.m_mass = 0.0f;
		rbDesc.m_startTransform = wallTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(wall, rbDesc);
	}

	Core::EntityID cube = Core::CreateEntity();
	fTrans const cubeTrans{ fQuatIdentity(), fVec3(0.0f, 0.0f, 0.0f) };
	Core::AddComponent(cube, Core::Transform(cubeTrans));
	Core::AddComponent(cube, Core::Render::CubeTest{false, 0.0f, 0.0f});
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = fVec3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 1000.0f;
		rbDesc.m_isKinematic = true;
		rbDesc.m_startTransform = cubeTrans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube, rbDesc);
	}
	//Core::AddComponent(cube, Core::Sound::BGMDesc{ .m_filePath = "assets/encrypted/bgm/rom.mp3", .m_initVolume = 1.0f, });
	//Core::AddComponent(cube, Core::Sound::FadeChangeBGM{ .m_timeToFade = 5.0f, .m_targetVolume = 1.0f, .m_nextBGMFilePath = "assets/encrypted/bgm/ztd.mp3", });

	Core::EntityID cube2 = Core::CreateEntity();
	fTrans const cube2Trans{ fQuatIdentity(), fVec3(-0.5f, 1.5f, 0.0f) };
	Core::AddComponent(cube2, Core::Transform(cube2Trans));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/cube/bluecube.obj";
		Core::AddComponent(cube2, modelDesc);
	}
	{
		Core::Physics::RigidBodyDesc rbDesc{};
		rbDesc.m_shapeType = Core::Physics::ShapeType::Box;
		rbDesc.m_boxHalfDimensions = fVec3(0.5f, 0.5f, 0.5f);
		rbDesc.m_mass = 160.0f;
		rbDesc.m_startTransform = cube2Trans;
		rbDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(cube2, rbDesc);
	}

	/*Core::EntityID backpack = Core::CreateEntity();
	Core::AddComponent(backpack, Core::Transform(fQuatIdentity(), fVec3(0.0f, 1.0f, -2.0f)));
	{
		Core::Render::ModelDesc modelDesc{};
		modelDesc.m_filePath = "assets/models/backpack/backpack.obj";
		Core::AddComponent(backpack, modelDesc);
		Core::AddComponent(backpack, Core::Render::CubeTest{ true, 0.0f, 0.0f });
	}*/

	Core::EntityID lightCube = Core::CreateEntity();
	Core::AddComponent(lightCube, Core::Transform(RotationFromForward(fVec3(-1.0f, -1.0f, 1.0f)), fVec3(1.2f, 1.0f, 2.0f)));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 1.0f;
		lightComponent.m_type = Core::Render::Light::Type::Directional;
		Core::AddComponent(lightCube, lightComponent);
	}
	Core::AddComponent(lightCube, Core::Render::SkyboxDesc{ .m_cubemapPath = "assets/encrypted/skybox/starrysky/starrysky.cubemap" });
	Core::EntityID lightCube2 = Core::CreateEntity();
	Core::AddComponent(lightCube2, Core::Render::CubeTest{ true, 0.0f, 0.0f });

	Core::AddComponent(lightCube2, Core::Transform(RotationFromForward(fVec3(0.0f, 0.0f, -1.0f)), fVec3(1.2f, 1.0f, 2.0f)));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 5.0f;
		//lightComponent.m_direction = fVec3(0.0f, 0.0f, -1.0f);
		lightComponent.m_attenuation = fVec3(1.0f, 0.07f, 0.18f);
		lightComponent.m_type = Core::Render::Light::Type::Spotlight;
		Core::AddComponent(lightCube2, lightComponent);
	}
	Core::EntityID lightCube3 = Core::CreateEntity();
	Core::AddComponent(lightCube3, Core::Transform(lightCube2));
	{
		Core::Render::Light lightComponent{};
		lightComponent.m_colour = fVec3(1.0f, 1.0f, 1.0f);
		lightComponent.m_intensity = 0.1f;
		lightComponent.m_type = Core::Render::Light::Type::Ambient;
		Core::AddComponent(lightCube3, lightComponent);
	}

	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Render::CubeTest& _cubeTest, Core::Transform& _t)
	{
		if (_cubeTest.isLightCube)
		{
			_cubeTest.rx += _fd.dt;
			_cubeTest.ry = sin(_cubeTest.rx);
			_t.T().m_origin.x = 0.0f;
			_t.T().m_basis = glm::yawPitchRoll(_cubeTest.rx, 0.0f, 0.0f);
		}
		else
		{
			_cubeTest.rx -= 1.0f * _fd.dt;
			_cubeTest.ry -= 2.0f * _fd.dt;
			_t.T().m_basis = glm::yawPitchRoll(0.0f, _cubeTest.rx, _cubeTest.ry);
		}
	});

	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Render::Light& _light, Core::Transform& _t)
	{
		static float n = 0.0f;
		n += _fd.dt * 0.5f;
		if (_light.m_type == Core::Render::Light::Type::Directional)
		{
			_t.T().m_basis = RotationFromForward(fVec3(cos(n), sin(n), 1.0f));
		}
	});
}