#ifndef SCREENSHOT_H
#define SCREENSHOT_H

namespace godot {

class PackedByteArray;

}

namespace sentry::util {

godot::PackedByteArray take_screenshot();

}

#endif // SCREENSHOT_H
