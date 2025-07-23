#ifndef COCOA_INCLUDES_H
#define COCOA_INCLUDES_H

#ifdef __OBJC__

#import <MetricKit/MetricKit.h>
#ifdef IOS_ENABLED
#import <UIKit/UIKit.h>
#endif
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
using SentryException = ::SentryException;
using SentryStacktrace = ::SentryStacktrace;
using SentryFrame = ::SentryFrame;

} // namespace objc

#else // C++ context

// In C++ context, make objc::SentryEvent an alias to void
namespace objc {
using SentryEvent = void;
} // namespace objc

#endif

#endif // COCOA_INCLUDES_H
