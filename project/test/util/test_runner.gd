class_name MyTestRunner
extends "res://addons/gdUnit4/src/core/runners/GdUnitTestSessionRunner.gd"

# Usage:
#	var TestRunner := load("res://my_test_runner.gd")
#	var runner = TestRunner.new()
#	add_child(runner)
#	runner.include_tests("res://tests/")

signal finished(result_code)

enum Result {
	SUCCESS = 0,
	FAILURE = 100,
	WARNINGS = 101,
	TESTS_NOT_FOUND = 104,
	DIDNT_RUN = 105,
	SCRIPT_ERRORS = 106
}


class Stats:
	var num_total: int = 0
	var num_failed: int = 0
	var num_errors: int = 0
	var num_warnings: int = 0
	var num_skipped: int = 0
	var num_flaky: int = 0

	func clear():
		num_total = 0
		num_failed = 0
		num_errors = 0
		num_warnings = 0
		num_skipped = 0
		num_flaky = 0


var stats := Stats.new()
var suite_stats := Stats.new()
var result_code: int = Result.DIDNT_RUN

var _included_tests := PackedStringArray()
var _broken_scripts := PackedStringArray()


## Initialize test execution
func init_runner() -> void:
	print_rich(Fmt.step(), Fmt.prominent("Initializing test runner..."))
	_discover_tests()

	if _test_cases.is_empty():
		if _broken_scripts.is_empty():
			print_rich(Fmt.error("No test cases found!"))
			_finish(Result.TESTS_NOT_FOUND)
		else:
			_print_broken_scripts()
			_finish(Result.SCRIPT_ERRORS)
		return
	_state = RUN


## Returns the exit code based on test results.[br]
## Maps test report status to process exit codes.
func get_exit_code() -> int:
	if not _broken_scripts.is_empty():
		return Result.SCRIPT_ERRORS
	elif stats.num_total == 0:
		return Result.DIDNT_RUN
	elif stats.num_failed > 0 or stats.num_errors > 0:
		return Result.FAILURE
	elif stats.num_warnings > 0:
		return Result.WARNINGS
	return Result.SUCCESS


## Cleanup and quit the runner.
func quit(_exit_code: int) -> void:
	_finish(_exit_code)


## Add file or dir for test discovery.
func include_tests(path: String) -> void:
	_included_tests.append(path)


## Discover tests added with include_tests(), collecting scripts that fail to load.
func _discover_tests() -> void:
	var gdunit_test_discover_added := GdUnitSignals.instance().gdunit_test_discover_added

	var scanner := GdUnitTestSuiteScanner.new()
	for path in _included_tests:
		# The scanner drops scripts it fails to load, so check them separately.
		for script_path in _find_scripts(path):
			# NOTE: A script that fails to parse still loads, only as an invalid resource.
			var script := load(script_path) as Script
			if script == null or not script.can_instantiate():
				_broken_scripts.append(script_path)

		for script in scanner.scan(path):
			print_rich(Fmt.step(), "Scanning: ", Fmt.suite(script.resource_path))
			GdUnitTestDiscoverer.discover_tests(script, func(test: GdUnitTestCase) -> void:
				print_rich(Fmt.substep(), "Discovered %s" % Fmt.case(test.display_name))
				_test_cases.append(test)
				gdunit_test_discover_added.emit(test)
			)


## Returns the GDScript paths at the given path, which is either a script or a directory.[br]
## Returns nothing if the path doesn't exist or isn't a GDScript.
static func _find_scripts(path: String) -> PackedStringArray:
	var dir := DirAccess.open(path)
	if dir == null:
		if path.get_extension() == "gd" and FileAccess.file_exists(path):
			return PackedStringArray([path])
		return PackedStringArray()

	# Directories excluded from the Godot project are excluded from testing too.
	if dir.file_exists(".gdignore"):
		return PackedStringArray()

	var scripts := PackedStringArray()
	for file_name in DirAccess.get_files_at(path):
		if file_name.get_extension() == "gd":
			scripts.append(path.path_join(file_name))
	for dir_name in DirAccess.get_directories_at(path):
		scripts.append_array(_find_scripts(path.path_join(dir_name)))
	return scripts


func _finish(code: int) -> void:
	_state = EXIT
	result_code = code
	GdUnitTools.dispose_all()
	await GdUnitMemoryObserver.gc_on_guarded_instances()
	await get_tree().process_frame
	await get_tree().physics_frame
	finished.emit(result_code)


## Process test events.
func _on_gdunit_event(event: GdUnitEvent) -> void:
	match event.type():
		GdUnitEvent.INIT:
			print_rich(Fmt.step(), Fmt.prominent("Initializing..."))
			stats.clear()
			suite_stats.clear()

		GdUnitEvent.STOP:
			print_rich(Fmt.step(), Fmt.prominent("Finished all tests."))
			_print_stats(stats, "Overall Summary")
			_print_broken_scripts()

		GdUnitEvent.TESTSUITE_BEFORE:
			print_rich(Fmt.step(), Fmt.prominent("Loading: "), Fmt.suite(event.resource_path()))
			suite_stats.clear()

		GdUnitEvent.TESTSUITE_AFTER:
			print_rich(Fmt.substep(), Fmt.prominent("Finished: "), Fmt.suite(event.resource_path()))
			_print_failure_report(event.reports())
			_print_stats(suite_stats, "Summary")
			stats.num_total += suite_stats.num_total
			stats.num_errors += suite_stats.num_errors
			stats.num_failed += suite_stats.num_failed
			stats.num_skipped += suite_stats.num_skipped
			stats.num_flaky += suite_stats.num_flaky
			stats.num_warnings += suite_stats.num_warnings

		GdUnitEvent.TESTCASE_BEFORE:
			var test := _test_session.find_test_by_id(event.guid())
			print_rich(Fmt.substep(), Fmt.prominent("Started: "),
					Fmt.suite(test.suite_name), " > ", Fmt.case(test.display_name))

		GdUnitEvent.TESTCASE_AFTER:
			suite_stats.num_total += 1
			suite_stats.num_failed +=  event.failed_count()
			suite_stats.num_errors += event.error_count()
			suite_stats.num_warnings += 1 if event.is_warning() else 0
			suite_stats.num_skipped += event.skipped_count()
			suite_stats.num_flaky += 1 if event.is_flaky() else 0

			var test := _test_session.find_test_by_id(event.guid())
			if event.is_success():
				_print_result(Fmt.bold(Fmt.success("PASSED")), test.suite_name, test.display_name)
			elif event.is_skipped():
				_print_result(Fmt.bold(Fmt.muted("SKIPPED")), test.suite_name, test.display_name)
			elif event.is_failed() or event.is_error():
				_print_result(Fmt.bold(Fmt.error("FAILED")), test.suite_name, test.display_name)
				_print_failure_report(event.reports())
			elif event.is_warning():
				_print_result(Fmt.bold(Fmt.warning("WARNING")), test.suite_name, test.display_name)
				_print_failure_report(event.reports())


# *** CONSOLE OUTPUT

## Reports test scripts that failed to load. Such scripts are absent from the test
## results above, so they are listed separately to explain the failed run.
func _print_broken_scripts() -> void:
	if _broken_scripts.is_empty():
		return
	print_rich(Fmt.substep(), Fmt.bold(Fmt.error("Scripts failed to load:")))
	for script_path in _broken_scripts:
		print_rich("    ", Fmt.error(script_path))


static func _print_result(status: String, test_suite: String, test_case: String) -> void:
	print_rich(Fmt.substep(), status, ": ",
			Fmt.suite(test_suite), " > ", Fmt.case(test_case))


static func _print_failure_report(reports: Array[GdUnitReport]) -> void:
	for report in reports:
		if (
			report.is_failure()
			or report.is_error()
			or report.is_warning()
			or report.is_skipped()
		):
			var text: String = str(report).indent(" ".repeat(4))
			print_rich(text)


static func _print_stats(p_stats: Stats, p_header: String) -> void:
	var total: String = "  %d total" % p_stats.num_total
	var errors: String = "  %d errors" % p_stats.num_errors
	var failed: String = "  %d failed" % p_stats.num_failed
	var skipped: String = "  %d skipped" % p_stats.num_skipped
	var flaky: String = "  %d flaky" % p_stats.num_flaky
	print_rich(
		Fmt.substep(),
		Fmt.bold(Fmt.accent(p_header)), ": ",
		Fmt.primary(total),
		Fmt.error(errors) if p_stats.num_errors > 0 else Fmt.primary(errors),
		Fmt.error(failed) if p_stats.num_failed > 0 else Fmt.primary(failed),
		Fmt.primary(skipped),
		Fmt.warning(flaky) if p_stats.num_flaky > 0 else Fmt.primary(flaky)
	)


class Fmt:
	static func colored(text: String, color: Color) -> String:
		return "[color=%s]%s[/color]" % [color.to_html(), text]

	static func bold(text: String) -> String:
		return "[b]%s[/b]" % text

	static func step() -> String: return bold(colored("==> ", Color.WHITE))
	static func substep() -> String: return bold(colored("--> ", Color.WHITE))
	static func prominent(text: String) -> String: return bold(colored(text, Color.WHITE))

	static func primary(text: String) -> String: return colored(text, Color.WHITE)
	static func accent(text: String) -> String: return colored(text, Color.DODGER_BLUE)
	static func muted(text: String) -> String: return colored(text, Color.GRAY)
	static func success(text: String) -> String: return colored(text, Color.GREEN)
	static func warning(text: String) -> String: return colored(text, Color.GOLDENROD)
	static func error(text: String) -> String: return colored(text, Color.RED)

	static func suite(text: String) -> String: return colored(text, Color.MEDIUM_PURPLE)
	static func case(text: String) -> String: return colored(text, Color.CYAN)
