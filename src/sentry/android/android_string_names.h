#pragma once

#include "sentry/util/module_instance.h"

#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

namespace sentry::android::detail {
/**
 * Stores StringName constants for Android SDK implementation.
 * Improves performance by avoiding repeated StringName allocations and hash calculations.
 * Add new names only for repeatedly used strings.
 */
struct AndroidStringNameData {
	// API methods.
	const StringName init{ "init" };
	const StringName close{ "close" };
	const StringName isEnabled{ "isEnabled" };
	const StringName setContext{ "setContext" };
	const StringName removeContext{ "removeContext" };
	const StringName setTag{ "setTag" };
	const StringName removeTag{ "removeTag" };
	const StringName setUser{ "setUser" };
	const StringName removeUser{ "removeUser" };
	const StringName addBreadcrumb{ "addBreadcrumb" };
	const StringName log{ "log" };
	const StringName getLastEventId{ "getLastEventId" };
	const StringName captureError{ "captureError" };
	const StringName createEvent{ "createEvent" };
	const StringName releaseEvent{ "releaseEvent" };
	const StringName captureEvent{ "captureEvent" };
	const StringName captureFeedback{ "captureFeedback" };
	const StringName addFileAttachment{ "addFileAttachment" };
	const StringName addBytesAttachment{ "addBytesAttachment" };
	const StringName clearAttachments{ "clearAttachments" };
	const StringName setTrace{ "setTrace" };

	// Event methods.
	const StringName eventGetId{ "eventGetId" };
	const StringName eventSetMessage{ "eventSetMessage" };
	const StringName eventGetMessage{ "eventGetMessage" };
	const StringName eventSetTimestamp{ "eventSetTimestamp" };
	const StringName eventGetTimestamp{ "eventGetTimestamp" };
	const StringName eventGetPlatform{ "eventGetPlatform" };
	const StringName eventSetLevel{ "eventSetLevel" };
	const StringName eventGetLevel{ "eventGetLevel" };
	const StringName eventSetLogger{ "eventSetLogger" };
	const StringName eventGetLogger{ "eventGetLogger" };
	const StringName eventSetRelease{ "eventSetRelease" };
	const StringName eventGetRelease{ "eventGetRelease" };
	const StringName eventSetDist{ "eventSetDist" };
	const StringName eventGetDist{ "eventGetDist" };
	const StringName eventSetEnvironment{ "eventSetEnvironment" };
	const StringName eventGetEnvironment{ "eventGetEnvironment" };
	const StringName eventSetTag{ "eventSetTag" };
	const StringName eventRemoveTag{ "eventRemoveTag" };
	const StringName eventGetTag{ "eventGetTag" };
	const StringName eventSetUser{ "eventSetUser" };
	const StringName eventRemoveUser{ "eventRemoveUser" };
	const StringName eventSetFingerprint{ "eventSetFingerprint" };
	const StringName eventSetContext{ "eventSetContext" };
	const StringName eventMergeContext{ "eventMergeContext" };
	const StringName eventIsCrash{ "eventIsCrash" };
	const StringName eventToJson{ "eventToJson" };

	// Exceptions.
	const StringName eventAddException{ "eventAddException" };
	const StringName eventGetExceptionCount{ "eventGetExceptionCount" };
	const StringName eventSetExceptionValue{ "eventSetExceptionValue" };
	const StringName eventGetExceptionValue{ "eventGetExceptionValue" };
	const StringName eventAddThreadStackTrace{ "eventAddThreadStackTrace" };

	// Breadcrumbs.
	const StringName createBreadcrumb{ "createBreadcrumb" };
	const StringName releaseBreadcrumb{ "releaseBreadcrumb" };
	const StringName breadcrumbSetMessage{ "breadcrumbSetMessage" };
	const StringName breadcrumbGetMessage{ "breadcrumbGetMessage" };
	const StringName breadcrumbSetType{ "breadcrumbSetType" };
	const StringName breadcrumbGetType{ "breadcrumbGetType" };
	const StringName breadcrumbSetCategory{ "breadcrumbSetCategory" };
	const StringName breadcrumbGetCategory{ "breadcrumbGetCategory" };
	const StringName breadcrumbSetLevel{ "breadcrumbSetLevel" };
	const StringName breadcrumbGetLevel{ "breadcrumbGetLevel" };
	const StringName breadcrumbSetData{ "breadcrumbSetData" };
	const StringName breadcrumbGetTimestamp{ "breadcrumbGetTimestamp" };

	// Scopes.
	const StringName createScope{ "createScope" };
	const StringName releaseScope{ "releaseScope" };
	const StringName cloneScope{ "cloneScope" };
	const StringName scopeSetContext{ "scopeSetContext" };
	const StringName scopeSetTag{ "scopeSetTag" };
	const StringName scopeSetUser{ "scopeSetUser" };
	const StringName scopeRemoveUser{ "scopeRemoveUser" };
	const StringName scopeSetLevel{ "scopeSetLevel" };
	const StringName scopeSetFingerprint{ "scopeSetFingerprint" };
	const StringName scopeSetAttributeBool{ "scopeSetAttributeBool" };
	const StringName scopeSetAttributeLong{ "scopeSetAttributeLong" };
	const StringName scopeSetAttributeDouble{ "scopeSetAttributeDouble" };
	const StringName scopeSetAttributeString{ "scopeSetAttributeString" };
	const StringName scopeAddBreadcrumb{ "scopeAddBreadcrumb" };
	const StringName scopeAddFileAttachment{ "scopeAddFileAttachment" };
	const StringName scopeAddBytesAttachment{ "scopeAddBytesAttachment" };
	const StringName scopeClear{ "scopeClear" };
	const StringName scopeSetSpan{ "scopeSetSpan" };

	// Spans.
	const StringName startSpan{ "startSpan" };
	const StringName releaseSpan{ "releaseSpan" };
	const StringName spanStartChild{ "spanStartChild" };
	const StringName spanSetAttributeBool{ "spanSetAttributeBool" };
	const StringName spanSetAttributeLong{ "spanSetAttributeLong" };
	const StringName spanSetAttributeDouble{ "spanSetAttributeDouble" };
	const StringName spanSetAttributeString{ "spanSetAttributeString" };
	const StringName spanSetStatus{ "spanSetStatus" };
	const StringName spanEnd{ "spanEnd" };
	const StringName spanGetTraceHeaders{ "spanGetTraceHeaders" };

	// Logs.
	const StringName releaseLog{ "releaseLog" };
	const StringName logSetLevel{ "logSetLevel" };
	const StringName logGetLevel{ "logGetLevel" };
	const StringName logSetBody{ "logSetBody" };
	const StringName logGetBody{ "logGetBody" };
	const StringName logGetAttribute{ "logGetAttribute" };
	const StringName logSetAttribute{ "logSetAttribute" };
	const StringName logAddAttributes{ "logAddAttributes" };
	const StringName logRemoveAttribute{ "logRemoveAttribute" };

	// Metrics.
	const StringName metricsAddCount{ "metricsAddCount" };
	const StringName metricsAddGauge{ "metricsAddGauge" };
	const StringName metricsAddDistribution{ "metricsAddDistribution" };
	const StringName metricGetName{ "metricGetName" };
	const StringName metricSetName{ "metricSetName" };
	const StringName metricGetType{ "metricGetType" };
	const StringName metricSetType{ "metricSetType" };
	const StringName metricGetValue{ "metricGetValue" };
	const StringName metricSetValue{ "metricSetValue" };
	const StringName metricGetUnit{ "metricGetUnit" };
	const StringName metricSetUnit{ "metricSetUnit" };
	const StringName metricGetAttribute{ "metricGetAttribute" };
	const StringName metricSetAttribute{ "metricSetAttribute" };
	const StringName metricAddAttributes{ "metricAddAttributes" };
	const StringName metricRemoveAttribute{ "metricRemoveAttribute" };
	const StringName releaseMetric{ "releaseMetric" };

	// Attributes.
	const StringName setAttributeBool{ "setAttributeBool" };
	const StringName setAttributeLong{ "setAttributeLong" };
	const StringName setAttributeDouble{ "setAttributeDouble" };
	const StringName setAttributeString{ "setAttributeString" };
	const StringName removeAttribute{ "removeAttribute" };
};

} //namespace sentry::android::detail

namespace sentry::android {

using AndroidStringNames = util::ModuleInstance<detail::AndroidStringNameData>;

} //namespace sentry::android

#define ANDROID_SN(m_arg) sentry::android::AndroidStringNames::get().m_arg
