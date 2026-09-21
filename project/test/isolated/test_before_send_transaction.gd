extends SentryTestSuite
## Verifies the `before_send_transaction` callback.


signal transaction_processed

var captured_transactions: Array[String]
var before_send_transaction: Callable # (transaction: SentryEvent) -> SentryEvent


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.traces_sample_rate = 1.0
		options.before_send_transaction = func(transaction: SentryEvent) -> SentryEvent:
			return before_send_transaction.call(transaction)
	)


func before_test() -> void:
	super()
	captured_transactions.clear()
	before_send_transaction = _record_transaction


func _record_transaction(transaction: SentryEvent) -> SentryEvent:
	var json := transaction.to_json()
	if OS.get_thread_caller_id() == OS.get_main_thread_id():
		_record_transaction_json(json)
	else:
		_record_transaction_json.call_deferred(json)
	return transaction


func _record_transaction_json(json: String) -> void:
	captured_transactions.append(json)
	transaction_processed.emit()


func _wait_for_captured_transaction_json() -> String:
	if captured_transactions.is_empty():
		await await_signal_on(self, "transaction_processed")
	if captured_transactions.is_empty():
		return ""
	return captured_transactions[0]


func _capture_transaction() -> void:
	var span := SentrySDK.start_span("test.before_send_transaction")
	span.end()


func test_callback_can_modify_transaction() -> void:
	before_send_transaction = func(transaction: SentryEvent) -> SentryEvent:
		transaction.set_tag("processed", "true")
		return _record_transaction(transaction)

	_capture_transaction()
	var transaction_json := await _wait_for_captured_transaction_json()

	assert_json(transaction_json).describe("the callback can modify a transaction") \
		.at("/tags") \
		.must_contain("processed", "true") \
		.verify()


func test_transaction_does_not_reach_before_send() -> void:
	_capture_transaction()
	await _wait_for_captured_transaction_json()

	assert_int(captured_events.size()).is_equal(0)
	await assert_signal(self).wait_until(500).is_not_emitted("event_captured")
