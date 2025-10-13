#include "state.h"

namespace sentry::logging {

thread_local bool in_message_logging = false;

} //namespace sentry::logging
