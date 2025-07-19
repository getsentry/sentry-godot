#ifndef COCOA_INCLUDES_H
#define COCOA_INCLUDES_H

#import <MetricKit/MetricKit.h>
#import <Sentry/Sentry-Swift.h>

namespace objc {

using SentryOptions = ::SentryOptions;
using SentrySDK = ::SentrySDK;
using SentryEvent = ::SentryEvent;
using SentryUser = ::SentryUser;
using SentryLevel = ::SentryLevel;
using SentryBreadcrumb = ::SentryBreadcrumb;
using SentryId = ::SentryId;
using SentryScope = ::SentryScope;
using SentryAttachment = ::SentryAttachment;
using SentryMessage = ::SentryMessage;

} // namespace objc

#endif // COCOA_INCLUDES_H
