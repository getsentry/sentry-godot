#ifndef DISABLED_ATTACHMENT_H
#define DISABLED_ATTACHMENT_H

#include "sentry_attachment.h"

// Attachment class that does nothing (for unsupported platforms or disabled SDK).
class DisabledAttachment : public SentryAttachment {
	GDCLASS(DisabledAttachment, SentryAttachment);

protected:
	static void _bind_methods() {}

public:
	virtual ~DisabledAttachment() override = default;
};

#endif // DISABLED_ATTACHMENT_H
