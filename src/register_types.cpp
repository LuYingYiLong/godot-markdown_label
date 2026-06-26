#include "register_types.h"

#include "markdown_label.h"

#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_markdown_label_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_INTERNAL_CLASS(MarkdownLabelCanvas);
		GDREGISTER_CLASS(MarkdownLabel);
	}
}

void uninitialize_markdown_label_module(ModuleInitializationLevel p_level) {
}

extern "C" {
	GDExtensionBool GDE_EXPORT markdown_label_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_markdown_label_module);
		init_obj.register_terminator(uninitialize_markdown_label_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
