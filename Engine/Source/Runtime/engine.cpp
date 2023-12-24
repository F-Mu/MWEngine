#include "engine.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
namespace MW
{
	void MWEngine::run()
	{
	}

	void MWEngine::tick(float deltaTime)
	{
		engineGlobalContext.renderSystem->tick(deltaTime);
	}

	void MWEngine::initialize()
	{
		engineGlobalContext.initialize();
	}

	void MWEngine::clear()
	{
		engineGlobalContext.clear();
	}
}
