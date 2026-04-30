#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "editor/csproj_patcher.h"

#include <string_view>

using editor::CsprojPatcher;
using sentry::tests::as_view;

namespace {

struct CsprojFixture {
	static constexpr std::string_view IMPORT_PATH = "Sentry.Godot.props";

	static constexpr std::string_view PROJECT_WITHOUT_IMPORT =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"  </PropertyGroup>\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_PATCHED =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"  </PropertyGroup>\n"
			"  <Import Project=\"Sentry.Godot.props\" Condition=\"Exists('Sentry.Godot.props')\" />\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_WITH_IMPORT =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"  <Import Project=\"Sentry.Godot.props\" />\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"  </PropertyGroup>\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_WITHOUT_IMPORT_CRLF =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\r\n"
			"  <PropertyGroup>\r\n"
			"    <TargetFramework>net8.0</TargetFramework>\r\n"
			"  </PropertyGroup>\r\n"
			"</Project>\r\n";

	static constexpr std::string_view PROJECT_PATCHED_CRLF =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\r\n"
			"  <PropertyGroup>\r\n"
			"    <TargetFramework>net8.0</TargetFramework>\r\n"
			"  </PropertyGroup>\r\n"
			"  <Import Project=\"Sentry.Godot.props\" Condition=\"Exists('Sentry.Godot.props')\" />\r\n"
			"</Project>\r\n";

	static constexpr std::string_view PROJECT_WITHOUT_IMPORT_TABS =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"\t<PropertyGroup>\n"
			"\t\t<TargetFramework>net8.0</TargetFramework>\n"
			"\t</PropertyGroup>\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_PATCHED_TABS =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"\t<PropertyGroup>\n"
			"\t\t<TargetFramework>net8.0</TargetFramework>\n"
			"\t</PropertyGroup>\n"
			"\t<Import Project=\"Sentry.Godot.props\" Condition=\"Exists('Sentry.Godot.props')\" />\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_WITH_COMMENTED_IMPORT =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"  </PropertyGroup>\n"
			"  <!-- <Import Project=\"Sentry.Godot.props\" /> -->\n"
			"</Project>\n";

	static constexpr std::string_view PROJECT_PATCHED_FROM_COMMENTED =
			"<Project Sdk=\"Godot.NET.Sdk/4.5.0\">\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"  </PropertyGroup>\n"
			"  <!-- <Import Project=\"Sentry.Godot.props\" /> -->\n"
			"  <Import Project=\"Sentry.Godot.props\" Condition=\"Exists('Sentry.Godot.props')\" />\n"
			"</Project>\n";
};

} // unnamed namespace

TEST_SUITE("CsprojPatcher") {
	TEST_CASE_FIXTURE(CsprojFixture, "Empty input is an error") {
		auto result = CsprojPatcher::ensure_import("", IMPORT_PATH);
		CHECK(result.status == CsprojPatcher::Status::ERROR);
	}

	TEST_CASE_FIXTURE(CsprojFixture, "Missing import gets patched") {
		auto result = CsprojPatcher::ensure_import(PROJECT_WITHOUT_IMPORT, IMPORT_PATH);
		REQUIRE(result.status == CsprojPatcher::Status::PATCHED);
		CHECK_SNAPSHOT_EQ(as_view(result.patched_content), PROJECT_PATCHED);
	}

	TEST_CASE_FIXTURE(CsprojFixture, "CRLF line endings preserved") {
		auto result = CsprojPatcher::ensure_import(PROJECT_WITHOUT_IMPORT_CRLF, IMPORT_PATH);
		REQUIRE(result.status == CsprojPatcher::Status::PATCHED);
		CHECK_SNAPSHOT_EQ(as_view(result.patched_content), PROJECT_PATCHED_CRLF);
	}

	TEST_CASE_FIXTURE(CsprojFixture, "Tab indentation preserved") {
		auto result = CsprojPatcher::ensure_import(PROJECT_WITHOUT_IMPORT_TABS, IMPORT_PATH);
		REQUIRE(result.status == CsprojPatcher::Status::PATCHED);
		CHECK_SNAPSHOT_EQ(as_view(result.patched_content), PROJECT_PATCHED_TABS);
	}

	TEST_CASE_FIXTURE(CsprojFixture, "Commented-out import is ignored") {
		auto result = CsprojPatcher::ensure_import(PROJECT_WITH_COMMENTED_IMPORT, IMPORT_PATH);
		REQUIRE(result.status == CsprojPatcher::Status::PATCHED);
		CHECK_SNAPSHOT_EQ(as_view(result.patched_content), PROJECT_PATCHED_FROM_COMMENTED);
	}

	TEST_CASE_FIXTURE(CsprojFixture, "Already-patched is unchanged") {
		auto result = CsprojPatcher::ensure_import(PROJECT_WITH_IMPORT, IMPORT_PATH);
		CHECK(result.status == CsprojPatcher::Status::PATCH_NOT_NEEDED);
	}
}

#endif // TESTS_ENABLED
