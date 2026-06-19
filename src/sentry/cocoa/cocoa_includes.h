#pragma once

#ifdef __OBJC__

#import <MetricKit/MetricKit.h>
#ifdef IOS_ENABLED
#import <UIKit/UIKit.h>
#endif
#import <SentryObjC/SentryObjC.h>

#else // C++ context

// Allow headers to compile: In C++ context, the ObjC types are not available.
using SentryObjCEvent = void;
using SentryObjCBreadcrumb = void;
using SentryObjCLog = void;
using SentryObjCMetric = void;

#endif // __OBJC__
