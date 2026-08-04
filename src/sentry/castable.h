#pragma once

#include <type_traits>

namespace sentry {

// Casts between related classes without the cost of dynamic_cast.
// Declare with SENTRY_CASTABLE in every participating class, naming Castable
// as the base in the root class.
class Castable {
public:
	struct TypeInfo {
		const TypeInfo *parent;
	};

	static constexpr TypeInfo type_info{ nullptr };

	// Returns true if p_from is a T, or derives from T. Null is safe and never matches.
	template <typename T>
	static bool is_class(const Castable *p_from) {
		static_assert(std::is_same_v<typename T::castable_self, T>,
				"T must declare SENTRY_CASTABLE");
		if (!p_from) {
			return false;
		}
		const TypeInfo *t = p_from->get_type_info();
		while (t) {
			if (t == &T::type_info) {
				return true;
			}
			t = t->parent;
		}
		return false;
	}

	// Returns p_from as T, or null if it isn't one. Null is safe.
	template <typename T>
	static T *cast_to(Castable *p_from) {
		return is_class<T>(p_from) ? static_cast<T *>(p_from) : nullptr;
	}

	// Returns p_from as T, or null if it isn't one. Null is safe.
	template <typename T>
	static const T *cast_to(const Castable *p_from) {
		return is_class<T>(p_from) ? static_cast<const T *>(p_from) : nullptr;
	}

	virtual ~Castable() = default;

protected:
	virtual const TypeInfo *get_type_info() const = 0;
};

} //namespace sentry

// Adds type info to a castable class. Add at the top of the class body.
#define SENTRY_CASTABLE(m_class, m_base)                                           \
public:                                                                            \
	using castable_self = m_class;                                                 \
	static constexpr ::sentry::Castable::TypeInfo type_info{ &m_base::type_info }; \
                                                                                   \
protected:                                                                         \
	virtual const ::sentry::Castable::TypeInfo *get_type_info() const override {   \
		static_assert(std::is_base_of_v<m_base, m_class>,                          \
				#m_base " must be a base of " #m_class ".");                       \
		return &type_info;                                                         \
	}                                                                              \
                                                                                   \
private:
