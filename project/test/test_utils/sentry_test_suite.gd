class_name SentryTestSuite
extends GdUnitTestSuite
## Sentry test suite extensions for gdUnit4.


## Perform queries and assertions on JSON content.
func assert_json(json: Variant) -> JSONAssert:
	return JSONAssert.new(json)
