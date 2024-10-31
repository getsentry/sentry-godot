#ifndef CONTEXTS_H
#define CONTEXTS_H

#include "../runtime_config.h"

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace sentry::contexts {

Dictionary make_device_context(const Ref<RuntimeConfig> &p_runtime_config);
Dictionary make_app_context();
Dictionary make_gpu_context();
Dictionary make_culture_context();
Dictionary make_display_context();
Dictionary make_godot_engine_context();
Dictionary make_environment_context();
Dictionary make_performance_context();

} //namespace sentry::contexts

#endif // CONTEXTS_H
