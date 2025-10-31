class_name MyTestRunner
extends "res://addons/gdUnit4/src/core/runners/GdUnitTestSessionRunner.gd"

# Usage:
#	var TestRunner := load("res://my_test_runner.gd")
#	var runner = TestRunner.new()
#	add_child(runner)
#	runner.include_tests("res://tests/")

const GdUnitTools := preload("res://addons/gdUnit4/src/core/GdUnitTools.gd")

signal finished(result_code)

enum Result {
	SUCCESS = 0,
	FAILURE = 100,
	WARNINGS = 101,
	DIDNT_RUN = 200,
	TESTS_NOT_FOUND = 204
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
		num_skipped = 0
		num_flaky = 0


var stats := Stats.new()
var suite_stats := Stats.new()
var result_code: int = Result.DIDNT_RUN

var _included_tests := PackedStringArray()


## Initialize test execution
func init_runner() -> void:
	print_rich(_step(), _prominent("Initializing test runner..."))
	_test_cases = _discover_tests()
	if _test_cases.is_empty():
		print_rich(_error("No test cases found!"))
		_finish(Result.TESTS_NOT_FOUND)
		return
	_state = RUN


## Returns the exit code based on test results.[br]
## Maps test report status to process exit codes.
func get_exit_code() -> int:
	if stats.num_total == 0:
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


## Discover tests added with include_tests()
func _discover_tests() -> Array[GdUnitTestCase]:
	var gdunit_test_discover_added := GdUnitSignals.instance().gdunit_test_discover_added

	var scanner := GdUnitTestSuiteScanner.new()
	for path in _included_tests:
		var scripts := scanner.scan(path)
		for script in scripts:
			print_rich(_step(), "Scanning: ", script.resource_path)
			GdUnitTestDiscoverer.discover_tests(script, func(test: GdUnitTestCase) -> void:
				print_rich(_substep(), "Discovered %s" % test.display_name)
				_test_cases.append(test)
				gdunit_test_discover_added.emit(test)
			)

	return _test_cases


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
			print_rich(_step(), _prominent("Initializing..."))
			stats.clear()
			suite_stats.clear()

		GdUnitEvent.STOP:
			print_rich(_step(), _prominent("Finished all tests."))
			_print_stats(stats, "Overall Summary")

		GdUnitEvent.TESTSUITE_BEFORE:
			print_rich(_step(), _prominent("Loading: "), _suite(event.resource_path()))
			suite_stats.clear()

		GdUnitEvent.TESTSUITE_AFTER:
			print_rich(_substep(), _prominent("Finished: "), _suite(event.resource_path()))
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
			print_rich(_substep(), _prominent("Started: "),
					_suite(test.suite_name), " > ", _case(test.display_name))

		GdUnitEvent.TESTCASE_AFTER:
			suite_stats.num_total += 1
			suite_stats.num_errors += event.error_count()
			suite_stats.num_failed +=  event.failed_count()
			suite_stats.num_skipped += event.skipped_count()
			suite_stats.num_flaky += 1 if event.is_flaky() else 0
			suite_stats.num_warnings += 1 if event.is_warning() else 0

			var test := _test_session.find_test_by_id(event.guid())
			if event.is_success():
				_print_result(_bold(_success("PASSED")), test.suite_name, test.display_name)
			elif event.is_skipped():
				_print_result(_bold(_muted("SKIPPED")), test.suite_name, test.display_name)
			elif event.is_failed() or event.is_error():
				_print_result(_bold(_error("FAILED")), test.suite_name, test.display_name)
				_print_failure_report(event.reports())
			elif event.is_warning():
				_print_result(_bold(_warning("WARNING")), test.suite_name, test.display_name)
				_print_failure_report(event.reports())


# *** CONSOLE OUTPUT

static func _colored(text: String, color: Color) -> String:
	return "[color=%s]%s[/color]" % [color.to_html(), text]


static func _bold(text: String) -> String:
	return "[b]%s[/b]" % text


static func _step() -> String: return _bold(_colored("==> ", Color.WHITE))
static func _substep() -> String: return _bold(_colored("--> ", Color.WHITE))
static func _prominent(text: String) -> String: return _bold(_colored(text, Color.WHITE))

static func _primary(text: String) -> String: return _colored(text, Color.WHITE)
static func _accent(text: String) -> String: return _colored(text, Color.DODGER_BLUE)
static func _muted(text: String) -> String: return _colored(text, Color.GRAY)
static func _success(text: String) -> String: return _colored(text, Color.GREEN)
static func _warning(text: String) -> String: return _colored(text, Color.GOLDENROD)
static func _error(text: String) -> String: return _colored(text, Color.RED)

static func _suite(text: String) -> String: return _colored(text, Color.DARK_CYAN)
static func _case(text: String) -> String: return _colored(text, Color.CYAN)


static func _print_result(status: String, test_suite: String, test_case: String) -> void:
	print_rich(_substep(), status, ": ",
			_suite(test_suite), " > ", _case(test_case))


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
		_substep(),
		_bold(_accent(p_header)), ": ",
		_primary(total),
		_error(errors) if p_stats.num_errors > 0 else _primary(errors),
		_error(failed) if p_stats.num_failed > 0 else _primary(failed),
		_primary(skipped),
		_warning(flaky) if p_stats.num_flaky > 0 else _primary(flaky)
	)
