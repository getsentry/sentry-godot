#ifndef COCOA_INCLUDES_H
#define COCOA_INCLUDES_H

#ifdef __OBJC__

#import <MetricKit/MetricKit.h>
#ifdef IOS_ENABLED
#import <UIKit/UIKit.h>
#endif
#import <Sentry/Sentry-Swift.h>

namespace objc {

// Type aliases for Cocoa SDK types to avoid naming conflicts
// with C++ extension types. The objc:: namespace distinguishes original
// Cocoa types from types defined elsewhere.

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
using SentryThread = ::SentryThread;

} // namespace objc

#else // C++ context

// In C++ context, make objc::SentryEvent an alias to void
namespace objc {
using SentryEvent = void;
using SentryBreadcrumb = void;
} // namespace objc

#endif // __OBJC__

#endif // COCOA_INCLUDES_H
