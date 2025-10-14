extends GdUnitTestSuite
## Test Feedback class.


func test_feedback_properties() -> void:
	var feedback := SentryFeedback.new()

	assert_str(feedback.associated_event_id).is_empty()
	feedback.associated_event_id = "082ce03eface41dd94b8c6b005382d5e"
	assert_str(feedback.associated_event_id).is_equal("082ce03eface41dd94b8c6b005382d5e")

	assert_str(feedback.name).is_empty()
	feedback.name = "Bob"
	assert_str(feedback.name).is_equal(feedback.name)

	assert_str(feedback.contact_email).is_empty()
	feedback.contact_email = "bob@example.com"
	assert_str(feedback.contact_email).is_equal("bob@example.com")

	assert_str(feedback.message).is_empty()
	feedback.message = "something happened"
	assert_str(feedback.message).is_equal("something happened")
