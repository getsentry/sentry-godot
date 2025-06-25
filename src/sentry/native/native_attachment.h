#ifndef NATIVE_ATTACHMENT_H
#define NATIVE_ATTACHMENT_H

#include "sentry_attachment.h"

#include <sentry.h>

// Attachment class that is used with the NativeSDK.
class NativeAttachment : public SentryAttachment {
	GDCLASS(NativeAttachment, SentryAttachment);

private:
	sentry_attachment_t *native_attachment = nullptr;

protected:
	static void _bind_methods() {}

public:
	sentry_attachment_t *get_native_attachment() const { return native_attachment; }
	void set_native_attachment(sentry_attachment_t *p_native_attachment) { native_attachment = p_native_attachment; }
};

#endif // NATIVE_ATTACHMENT_H
