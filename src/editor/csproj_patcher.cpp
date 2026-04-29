#include "csproj_patcher.h"

#include <cstring>

namespace {

struct AttributeToken {
	std::string_view key;
	std::string_view value;
};

bool _begins_with_at(std::string_view p_str, size_t p_pos, std::string_view p_prefix) {
	if (p_pos + p_prefix.size() > p_str.size()) {
		return false;
	}
	return std::memcmp(p_str.data() + p_pos, p_prefix.data(), p_prefix.size()) == 0;
}

inline bool _is_whitespace(const char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::string_view _read_tag_name(std::string_view p_tag, bool &r_is_closing_tag) {
	r_is_closing_tag = false;
	if (p_tag.size() < 2 || p_tag[0] != '<') {
		return {};
	}
	size_t i = 1;
	if (p_tag[i] == '/') {
		r_is_closing_tag = true;
		++i;
	}
	const size_t start = i;
	while (i < p_tag.size() && !_is_whitespace(p_tag[i]) && p_tag[i] != '/' && p_tag[i] != '>') {
		++i;
	}
	return p_tag.substr(start, i - start);
}

inline void _skip_whitespace(std::string_view p_tag, size_t &r_idx) {
	while (r_idx < p_tag.size() && _is_whitespace(p_tag[r_idx])) {
		++r_idx;
	}
}

inline bool _is_end_of_tag(std::string_view p_tag, size_t p_idx) {
	return p_idx >= p_tag.size() || p_tag[p_idx] == '>' || p_tag[p_idx] == '/';
}

AttributeToken _read_next_attribute_token(std::string_view p_tag, size_t &r_idx) {
	AttributeToken token;

	_skip_whitespace(p_tag, r_idx);

	if (_is_end_of_tag(p_tag, r_idx)) {
		return {};
	}

	// Read attribute name (until '=', whitespace, or end).
	const size_t name_start = r_idx;
	while (!_is_end_of_tag(p_tag, r_idx) && !_is_whitespace(p_tag[r_idx]) && p_tag[r_idx] != '=') {
		++r_idx;
	}
	token.key = p_tag.substr(name_start, r_idx - name_start);

	if (token.key.empty()) {
		return {};
	}

	_skip_whitespace(p_tag, r_idx);
	if (_is_end_of_tag(p_tag, r_idx) || p_tag[r_idx] != '=') {
		// Attribute without value.
		return token;
	}
	++r_idx; // skip '='

	_skip_whitespace(p_tag, r_idx);
	if (_is_end_of_tag(p_tag, r_idx)) {
		return {};
	}

	// Read quoted value.
	const char quote = p_tag[r_idx];
	if (quote != '"' && quote != '\'') {
		return {};
	}

	++r_idx; // skip opening quote
	const size_t value_start = r_idx;
	while (r_idx < p_tag.size() && p_tag[r_idx] != quote) {
		++r_idx;
	}

	if (r_idx >= p_tag.size() || p_tag[r_idx] != quote) {
		return {};
	}

	token.value = p_tag.substr(value_start, r_idx - value_start);
	++r_idx; // skip closing quote

	return token;
}

std::string_view _read_attribute_value(std::string_view p_tag, std::string_view p_attribute_name) {
	if (p_tag.empty() || p_tag[0] != '<') {
		return {};
	}

	size_t i = 1; // past '<'
	if (i < p_tag.size() && p_tag[i] == '/') {
		++i;
	}

	// Skip tag name
	while (!_is_end_of_tag(p_tag, i) && !_is_whitespace(p_tag[i])) {
		++i;
	}

	while (!_is_end_of_tag(p_tag, i)) {
		AttributeToken token = _read_next_attribute_token(p_tag, i);
		if (token.key.empty()) {
			// No attributes or error.
			break;
		}
		if (token.key == p_attribute_name) {
			return token.value;
		}
	}

	return {};
}

bool _is_same_path(std::string_view p_path1, std::string_view p_path2) {
	if (p_path1.size() != p_path2.size()) {
		return false;
	}
	for (size_t i = 0; i < p_path1.size(); ++i) {
		const char c1 = p_path1[i];
		const char c2 = p_path2[i];
		if (c1 == c2) {
			continue;
		}
		const bool c1_is_sep = (c1 == '/' || c1 == '\\');
		const bool c2_is_sep = (c2 == '/' || c2 == '\\');
		if (c1_is_sep && c2_is_sep) {
			continue;
		}
		return false;
	}
	return true;
}

inline void _vector_append(std::vector<uint8_t> &p_vec, std::string_view p_str) {
	p_vec.insert(p_vec.end(), p_str.begin(), p_str.end());
}

std::string_view _detect_line_ending(std::string_view p_content) {
	bool detected_return = false;
	for (size_t i = 0; i < p_content.size(); ++i) {
		const char c = p_content[i];
		if (c == '\r') {
			detected_return = true;
		} else if (c == '\n') {
			return detected_return ? "\r\n" : "\n";
		} else {
			detected_return = false;
		}
	}
	return "\n";
}

std::string_view _detect_indent(std::string_view p_content, size_t p_project_closing_tag) {
	constexpr const char *default_indent = "  ";
	if (p_project_closing_tag == 0) {
		return default_indent;
	}

	// Find previous line with a tag.
	const size_t bracket_above = p_content.rfind('<', p_project_closing_tag - 1);
	const size_t prev_nl = p_content.rfind('\n', bracket_above);
	if (prev_nl == std::string_view::npos) {
		return default_indent;
	}
	const size_t prev_line_with_tag = prev_nl + 1;

	// Count indent chars.
	size_t num_indent = 0;
	for (size_t i = prev_line_with_tag; i < p_content.size(); ++i) {
		const char c = p_content[i];
		if (c == ' ' || c == '\t') {
			++num_indent;
		} else {
			break;
		}
	}

	return p_content.substr(prev_line_with_tag, num_indent);
}

} // unnamed namespace

namespace editor {

CsprojPatcher::Result CsprojPatcher::ensure_import(const std::string_view p_csproj_content, std::string_view p_import_path) {
	if (p_csproj_content.empty()) {
		return Result{ Status::ERROR, {}, "Csproj content is empty" };
	}
	if (p_import_path.empty()) {
		return Result{ Status::ERROR, {}, "Import path is empty" };
	}

	enum class State {
		NORMAL,
		COMMENT,
		TAG_OPEN,
		TAG_SINGLE_QUOTES,
		TAG_DOUBLE_QUOTES,
		CDATA,
	};

	std::vector<State> states;
	states.push_back(State::NORMAL);

	size_t i = 0;

	const size_t content_len = p_csproj_content.size();
	size_t tag_start = 0;
	size_t project_closing_tag = SIZE_MAX;
	bool import_found = false;

	while (i < content_len) {
		const char c = p_csproj_content[i];
		switch (states.back()) {
			case State::NORMAL: {
				if (c == '<') {
					if (_begins_with_at(p_csproj_content, i, "<!--")) {
						states.push_back(State::COMMENT);
						i += 4;
						break;
					}
					if (_begins_with_at(p_csproj_content, i, "<![CDATA[")) {
						states.push_back(State::CDATA);
						i += 9;
						break;
					}
					tag_start = i;
					states.push_back(State::TAG_OPEN);
					++i;
				} else {
					++i;
				}
			} break;
			case State::COMMENT: {
				if (_begins_with_at(p_csproj_content, i, "-->")) {
					i += 3;
					states.pop_back();
				} else {
					++i;
				}
			} break;
			case State::TAG_OPEN: {
				if (c == '\'') {
					states.push_back(State::TAG_SINGLE_QUOTES);
					++i;
				} else if (c == '"') {
					states.push_back(State::TAG_DOUBLE_QUOTES);
					++i;
				} else if (c == '>') {
					const std::string_view tag = p_csproj_content.substr(tag_start, i + 1 - tag_start);
					bool is_closing_tag;
					const std::string_view name = _read_tag_name(tag, is_closing_tag);
					if (is_closing_tag && name == "Project") {
						project_closing_tag = tag_start;
					} else if (!is_closing_tag && name == "Import") {
						const std::string_view attr_value = _read_attribute_value(tag, "Project");
						if (!attr_value.empty() && _is_same_path(attr_value, p_import_path)) {
							import_found = true;
						}
					}
					states.pop_back();
					++i;
				} else {
					++i;
				}
			} break;
			case State::TAG_SINGLE_QUOTES: {
				if (c == '\'') {
					states.pop_back();
				}
				++i;
			} break;
			case State::TAG_DOUBLE_QUOTES: {
				if (c == '"') {
					states.pop_back();
				}
				++i;
			} break;
			case State::CDATA: {
				if (_begins_with_at(p_csproj_content, i, "]]>")) {
					states.pop_back();
					i += 3;
				} else {
					++i;
				}
			} break;
		}
	}

	if (states.size() > 1) {
		const char *msg;
		switch (states.back()) {
			case State::COMMENT: {
				msg = "XML comment not closed";
			} break;
			case State::CDATA: {
				msg = "XML CDATA not closed";
			} break;
			case State::TAG_OPEN:
			case State::TAG_SINGLE_QUOTES:
			case State::TAG_DOUBLE_QUOTES: {
				msg = "XML tag not closed";
			} break;
			default: {
				msg = "XML parsing error";
			} break;
		}
		return Result{ Status::ERROR, {}, msg };
	}

	if (import_found) {
		return { Status::PATCH_NOT_NEEDED, {} };
	}

	if (project_closing_tag == SIZE_MAX) {
		return Result{ Status::ERROR, {}, "No <Project> tag found" };
	}

	std::vector<uint8_t> patched;
	constexpr size_t import_approx_len = 64;
	patched.reserve(content_len + import_approx_len + 2 * p_import_path.size());

	const uint8_t *src = reinterpret_cast<const uint8_t *>(p_csproj_content.data());
	patched.insert(patched.end(), src, src + project_closing_tag);

	std::string_view indent = _detect_indent(p_csproj_content, project_closing_tag);
	_vector_append(patched, indent);
	_vector_append(patched, "<Import Project=\"");
	_vector_append(patched, p_import_path);
	_vector_append(patched, "\" Condition=\"Exists('");
	_vector_append(patched, p_import_path);
	_vector_append(patched, "')\" />");
	_vector_append(patched, _detect_line_ending(p_csproj_content));

	patched.insert(patched.end(), src + project_closing_tag, src + p_csproj_content.length());

	return Result{ Status::PATCHED, patched };
}

} //namespace editor
