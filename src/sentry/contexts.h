#ifndef CONTEXTS_H
#define CONTEXTS_H

#include "runtime_config.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace sentry::contexts {

bool should_delay_contexts();
void init_contexts();

Dictionary make_device_context(const Ref<RuntimeConfig> &p_runtime_config);

// Returns smaller device context dictionary that only includes values that are
// dynamic and need to be updated right before the event is sent.
Dictionary make_device_context_update();

Dictionary make_app_context();
Dictionary make_gpu_context();
Dictionary make_culture_context();
Dictionary make_display_context();
Dictionary make_godot_engine_context();
Dictionary make_environment_context();
Dictionary make_performance_context();

// Creates contexts that can only be generated right before an event, e.g. performance info.
HashMap<String, Dictionary> make_event_contexts();

} //namespace sentry::contexts

#endif // CONTEXTS_H
