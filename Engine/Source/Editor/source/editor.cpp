#include "editor.h"
#include "editorGlobalContext.h"
#include "runtime/function/global/engine_global_context.h"
#include "runtime/engine.h"
#include "runtime/function/render/window_system.h"
#include <chrono>
namespace MW {
	float calculateDeltaTime()
	{
		float deltaTime;
		{
			using namespace std::chrono;

			static steady_clock::time_point last = steady_clock::now();
			steady_clock::time_point now = steady_clock::now();
			duration<float> time_span = duration_cast<duration<float>>(now - last);
			deltaTime = time_span.count();
			last = now;
		}
		return deltaTime;
	}
	void MWEditor::run()
	{
		float deltaTime;
		while (!editorGlobalContext.windowSystem->shouldClose()) {
			glfwPollEvents();
			deltaTime = calculateDeltaTime();
			/*
			editorGlobalContext.m_scene_manager->tick(delta_time);
			g_editor_global_context.m_input_manager->tick(delta_time);*/
			engine->Tick(deltaTime);
		}
	}
	void MWEditor::initialize()
	{
		engine = std::make_shared<MWEngine>();
		engine->initialize();
		editorGlobalContext.initialize();
	}

	void MWEditor::clear()
	{
		editorGlobalContext.clear();
		engine->clear();
		engine.reset();
	}
}
