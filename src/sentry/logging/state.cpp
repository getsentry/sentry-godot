#include "state.h"

namespace sentry::logging {

thread_local bool in_message_logging = false;
thread_local bool skip_logging_messages = false;

} //namespace sentry::logging
