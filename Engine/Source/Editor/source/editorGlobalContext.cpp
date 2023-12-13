#include "editorGlobalContext.h"
#include "Runtime/engine.h"
namespace MW {
	EditorGlobalContext editorGlobalContext;

	void EditorGlobalContext::initialize()
	{
		engine = std::make_shared<MWEngine>();
	}
	void EditorGlobalContext::clear()
	{
		engine->clear();
		engine.reset();
	}
}