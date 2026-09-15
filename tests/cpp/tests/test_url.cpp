#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/util/url.h"

using sentry::util::parse_url;
using sentry::util::URLParts;

TEST_SUITE("URL parsing") {
	TEST_CASE("Absolute URLs split components and redact sensitive fields") {
		URLParts parts;
		REQUIRED_CHECK(parse_url("Custom+V1.2-test://user:secret@example.com:8080/data?token=secret#top", parts) == OK);
		CHECK(parts.scheme == "custom+v1.2-test");
		CHECK(parts.has_authority);
		CHECK(parts.host == "example.com");
		CHECK(parts.port == 8080);
		CHECK(parts.path == "/data");
		CHECK(parts.query == "token=secret");
		CHECK(parts.fragment == "top");
		CHECK(parts.redacted() == "custom+v1.2-test://example.com:8080/data");
	}

	TEST_CASE("Ports distinguish omitted empty zero and numeric boundaries") {
		const struct {
			const char *suffix;
			int32_t port;
		} cases[] = {
			{ "", -1 },
			{ ":", -1 },
			{ ":0", 0 },
			{ ":65535", 65535 },
			{ ":000000000000000000000000000000443", 443 },
		};
		for (const auto &test : cases) {
			CAPTURE(test.suffix);
			URLParts parts;
			REQUIRED_CHECK(parse_url(String("https://example.com") + test.suffix, parts) == OK);
			CHECK(parts.host == "example.com");
			CHECK(parts.port == test.port);
			CHECK(parts.path.is_empty());
			const String expected = test.port < 0 ? String("https://example.com") : "https://example.com:" + itos(test.port);
			CHECK(parts.redacted() == expected);
		}
	}

	TEST_CASE("Network-path references preserve authority while redacting userinfo") {
		URLParts parts;
		REQUIRED_CHECK(parse_url("//user:secret@example.com:8080/path?token=secret#top", parts) == OK);
		CHECK(parts.has_authority);
		CHECK(parts.scheme.is_empty());
		CHECK(parts.host == "example.com");
		CHECK(parts.port == 8080);
		CHECK(parts.redacted() == "//example.com:8080/path");
		REQUIRED_CHECK(parse_url("///path", parts) == OK);
		CHECK(parts.has_authority);
		CHECK(parts.host.is_empty());
		CHECK(parts.redacted() == "///path");
	}

	TEST_CASE("IPv6 brackets separate address colons from the port delimiter") {
		URLParts parts;
		REQUIRED_CHECK(parse_url("http://[2001:db8::1]:9000/health", parts) == OK);
		CHECK(parts.host == "[2001:db8::1]");
		CHECK(parts.port == 9000);
		CHECK(parts.redacted() == "http://[2001:db8::1]:9000/health");
		for (const char *url : { "//user:secret@[::1]/path", "//user:secret@[::1]:/path" }) {
			CAPTURE(url);
			REQUIRED_CHECK(parse_url(url, parts) == OK);
			CHECK(parts.host == "[::1]");
			CHECK(parts.port == -1);
			CHECK(parts.redacted() == "//[::1]/path");
		}
	}

	TEST_CASE("Query and fragment delimiters work without a path") {
		URLParts parts;
		REQUIRED_CHECK(parse_url("https://example.com?next=https://other.com/#frag?notquery", parts) == OK);
		CHECK(parts.host == "example.com");
		CHECK(parts.path.is_empty());
		CHECK(parts.query == "next=https://other.com/");
		CHECK(parts.fragment == "frag?notquery");
		CHECK(parts.redacted() == "https://example.com");
	}

	TEST_CASE("Bare paths and invalid schemes do not create an authority") {
		URLParts parts;
		for (const char *path : { "", "/", "/a", "/data", "a/https://example.com/b", "1https://example.com/a", "ht_tps://example.com/a", "://example.com/a" }) {
			CAPTURE(path);
			REQUIRED_CHECK(parse_url("https://example.com:443/", parts) == OK);
			REQUIRED_CHECK(parse_url(path, parts) == OK);
			CHECK(parts.scheme.is_empty());
			CHECK_FALSE(parts.has_authority);
			CHECK(parts.host.is_empty());
			CHECK(parts.port == -1);
			CHECK(parts.path == path);
			CHECK(parts.redacted() == path);
		}
		REQUIRED_CHECK(parse_url("/data?next=https://example.com/#frag?notquery", parts) == OK);
		CHECK(parts.path == "/data");
		CHECK(parts.query == "next=https://example.com/");
		CHECK(parts.fragment == "frag?notquery");
		CHECK(parts.redacted() == "/data");
	}

	TEST_CASE("Empty components remain valid at delimiter boundaries") {
		const struct {
			const char *url;
			const char *redacted;
		} cases[] = {
			{ "?", "" },
			{ "#", "" },
			{ "?#", "" },
			{ "//", "//" },
			{ "//@", "//" },
			{ "https://", "https://" },
		};
		for (const auto &test : cases) {
			CAPTURE(test.url);
			URLParts parts;
			REQUIRED_CHECK(parse_url(test.url, parts) == OK);
			CHECK(parts.host.is_empty());
			CHECK(parts.path.is_empty());
			CHECK(parts.query.is_empty());
			CHECK(parts.fragment.is_empty());
			CHECK(parts.port == -1);
			CHECK(parts.redacted() == test.redacted);
		}
	}

	TEST_CASE("Unicode components own their text after parsing") {
		String url = U"https://user:secret@example.com/雪/😀?名前=値#章";
		URLParts parts;
		REQUIRED_CHECK(parse_url(url, parts) == OK);
		url = String();
		CHECK(parts.path == String(U"/雪/😀"));
		CHECK(parts.query == String(U"名前=値"));
		CHECK(parts.fragment == String(U"章"));
		CHECK(parts.redacted() == String(U"https://example.com/雪/😀"));
	}

	TEST_CASE("Malformed authorities fail without changing the output") {
		URLParts parts;
		REQUIRED_CHECK(parse_url("https://example.com:443/data?a=1#top", parts) == OK);
		for (const char *url : {
					 "http://example.com:+80/",
					 "http://example.com:-1/",
					 "http://example.com:443x/",
					 "http://example.com:65536/",
					 "http://example.com:184467440737095516160/",
					 "http://[::1/",
					 "http://::1]/",
					 "http://[::1]extra:80/",
					 "http://[[::1]:80/",
					 "http://[::1]]:80/",
					 "http://::1/",
					 "//user:secret@example.com:+80/",
					 "//user:secret@[::1/",
			 }) {
			CAPTURE(url);
			CHECK(parse_url(url, parts) == ERR_INVALID_DATA);
			CHECK(parts.scheme == "https");
			CHECK(parts.has_authority);
			CHECK(parts.host == "example.com");
			CHECK(parts.port == 443);
			CHECK(parts.path == "/data");
			CHECK(parts.query == "a=1");
			CHECK(parts.fragment == "top");
		}
	}
}

#endif // TESTS_ENABLED
