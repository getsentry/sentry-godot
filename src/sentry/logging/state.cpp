#include "state.h"

namespace sentry::logging {

thread_local bool in_message_logging = false;
thread_local bool in_error_logging = false;

} //namespace sentry::logging
