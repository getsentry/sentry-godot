#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/engine_lifecycle/engine_lifecycle.h"
#include "sentry/util/callable_cpp.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/script.hpp>

using namespace godot;
using sentry::engine_lifecycle::attach_predelete_script;

namespace {

class PredeleteObserver {
	LIFETIME_OBSERVABLE();

public:
	uint64_t object_id = 0;
	int call_count = 0;
	bool object_resolved_during_callback = false;

	void on_predelete() {
		++call_count;
		object_resolved_during_callback = ObjectDB::get_instance(object_id) != nullptr;
	}
};

} // namespace

TEST_SUITE("Pre-delete script") {
	TEST_CASE("Attaches an in-memory script and invokes the callback on pre-delete") {
		PredeleteObserver observer;
		Object *object = memnew(Object);
		observer.object_id = object->get_instance_id();
		CHECK(attach_predelete_script(object, callable_cpp(&observer, &PredeleteObserver::on_predelete)) == OK);
		Ref<Script> script = object->get_script();
		CHECK(script.is_valid());
		if (script.is_valid()) {
			CHECK(script->get_path().is_empty());
			CHECK(script->instance_has(object));
		}
		object->notification(Object::NOTIFICATION_POSTINITIALIZE);
		CHECK(observer.call_count == 0);
		memdelete(object);
		CHECK(observer.call_count == 1);
		CHECK(observer.object_resolved_during_callback);
		CHECK(ObjectDB::get_instance(observer.object_id) == nullptr);
	}

	TEST_CASE("Refuses to overwrite a pre-existing script and its instance state") {
		Ref<Script> existing_script = ClassDB::instantiate("GDScript");
		REQUIRED_CHECK(existing_script.is_valid());
		existing_script->set_source_code(R"(@tool
extends Object
var user_value: int = 0
)");
		REQUIRED_CHECK(existing_script->reload() == OK);

		PredeleteObserver observer;
		Object *object = memnew(Object);
		object->set_script(existing_script);
		object->set("user_value", 42);
		CHECK(existing_script->instance_has(object));

		CHECK(attach_predelete_script(object, callable_cpp(&observer, &PredeleteObserver::on_predelete)) == ERR_ALREADY_IN_USE);
		Ref<Script> retained_script = object->get_script();
		CHECK(retained_script == existing_script);
		CHECK(int(object->get("user_value")) == 42);
		memdelete(object);
		CHECK(observer.call_count == 0);
	}
}

#endif // TESTS_ENABLED
