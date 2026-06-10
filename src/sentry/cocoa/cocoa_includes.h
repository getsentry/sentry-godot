#pragma once

#ifdef __OBJC__

#import <MetricKit/MetricKit.h>
#ifdef IOS_ENABLED
#import <UIKit/UIKit.h>
#endif
#import <SentryObjC/SentryObjC.h>

namespace objc {

// Type aliases for Cocoa SDK types to avoid naming conflicts
// with C++ extension types. The objc:: namespace distinguishes original
// Cocoa types from types defined elsewhere.

// TODO: Re-evaluate this approach since names are no longer clashing.

using SentryOptions = ::SentryObjCOptions;
using SentrySDK = ::SentryObjCSDK;
using SentryEvent = ::SentryObjCEvent;
using SentryUser = ::SentryObjCUser;
using SentryLevel = ::SentryObjCLevel;
using SentryBreadcrumb = ::SentryObjCBreadcrumb;
using SentryId = ::SentryObjCId;
using SentryScope = ::SentryObjCScope;
using SentryAttachment = ::SentryObjCAttachment;
using SentryAttachmentType = ::SentryObjCAttachmentType;
using SentryMessage = ::SentryObjCMessage;
using SentryException = ::SentryObjCException;
using SentryStacktrace = ::SentryObjCStacktrace;
using SentryFrame = ::SentryObjCFrame;
using SentryThread = ::SentryObjCThread;
using SentryFeedback = ::SentryObjCFeedback;
using SentryLog = ::SentryObjCLog;
using SentryLogLevel = ::SentryObjCLogLevel;
using SentrySpanId = ::SentryObjCSpanId;
using SentryAttribute = ::SentryObjCAttribute;
using SentryAttributeContent = ::SentryObjCAttributeContent;
using SentryUnit = ::SentryObjCUnit;
using SentryMetric = ::SentryObjCMetric;
using SentryMetricValue = ::SentryObjCMetricValue;
using SentryEnvelopeItem = ::SentryObjCEnvelopeItem;
using PrivateSentrySDKOnly = ::SentryObjCPrivateSDKOnly;

} // namespace objc

#else // C++ context

// In C++ context, make objc::SentryEvent an alias to void
namespace objc {

using SentryEvent = void;
using SentryBreadcrumb = void;
using SentryLog = void;
using SentryMetric = void;

} // namespace objc

#endif // __OBJC__
