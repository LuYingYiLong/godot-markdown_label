#include "markdown_label.h"
#include "markdown_parser.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include <algorithm>
#include <climits>
#include <cmath>
#include <vector>

using namespace godot;

namespace {
	static int32_t get_theme_font_size_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_font_size(p_name, p_type)) {
			const int32_t value = theme->get_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}
		if (theme.is_valid() && theme->has_constant(p_name, p_type)) {
			const int32_t value = theme->get_constant(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}
		if (p_owner != nullptr && p_owner->has_theme_font_size(p_name, p_type)) {
			const int32_t value = p_owner->get_theme_font_size(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}
		if (p_owner != nullptr && p_owner->has_theme_constant(p_name, p_type)) {
			const int32_t value = p_owner->get_theme_constant(p_name, p_type);
			if (value > 0) {
				return value;
			}
		}

		return p_default;
	}

	static int32_t get_theme_constant_or(Control* p_owner, const StringName& p_type, const StringName& p_name, int32_t p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_constant(p_name, p_type)) {
			return theme->get_constant(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_constant(p_name, p_type)) {
			return p_owner->get_theme_constant(p_name, p_type);
		}

		return p_default;
	}

	static Color get_theme_color_or(Control* p_owner, const StringName& p_type, const StringName& p_name, const Color& p_default) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_color(p_name, p_type)) {
			return theme->get_color(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_color(p_name, p_type)) {
			return p_owner->get_theme_color(p_name, p_type);
		}

		return p_default;
	}

	static Ref<Font> get_theme_font_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_font(p_name, p_type)) {
			return theme->get_font(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_font(p_name, p_type)) {
			return p_owner->get_theme_font(p_name, p_type);
		}
		if (p_owner != nullptr) {
			return p_owner->get_theme_default_font();
		}

		return Ref<Font>();
	}

	static Ref<StyleBox> get_theme_stylebox_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_stylebox(p_name, p_type)) {
			return theme->get_stylebox(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_stylebox(p_name, p_type)) {
			return p_owner->get_theme_stylebox(p_name, p_type);
		}

		return Ref<StyleBox>();
	}

	static Ref<Texture2D> get_theme_icon_or(Control* p_owner, const StringName& p_type, const StringName& p_name) {
		const Ref<Theme> theme = p_owner == nullptr ? Ref<Theme>() : p_owner->get_theme();
		if (theme.is_valid() && theme->has_icon(p_name, p_type)) {
			return theme->get_icon(p_name, p_type);
		}
		if (p_owner != nullptr && p_owner->has_theme_icon(p_name, p_type)) {
			return p_owner->get_theme_icon(p_name, p_type);
		}
		return Ref<Texture2D>();
	}

	static Ref<StyleBoxFlat> create_internal_stylebox(const Color& p_background_color, const Color& p_border_color, int32_t p_border_left, int32_t p_border_top, int32_t p_border_right, int32_t p_border_bottom, float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom, int32_t p_corner_radius = 0) {
		Ref<StyleBoxFlat> stylebox;
		stylebox.instantiate();
		stylebox->set_bg_color(p_background_color);
		stylebox->set_border_color(p_border_color);
		stylebox->set_border_width(SIDE_LEFT, p_border_left);
		stylebox->set_border_width(SIDE_TOP, p_border_top);
		stylebox->set_border_width(SIDE_RIGHT, p_border_right);
		stylebox->set_border_width(SIDE_BOTTOM, p_border_bottom);
		stylebox->set_content_margin(SIDE_LEFT, p_margin_left);
		stylebox->set_content_margin(SIDE_TOP, p_margin_top);
		stylebox->set_content_margin(SIDE_RIGHT, p_margin_right);
		stylebox->set_content_margin(SIDE_BOTTOM, p_margin_bottom);
		stylebox->set_corner_radius_all(p_corner_radius);
		return stylebox;
	}

	static float get_stylebox_margin_or(const Ref<StyleBox>& p_stylebox, Side p_side, float p_default) {
		if (p_stylebox.is_valid()) {
			return p_stylebox->get_margin(p_side);
		}

		return p_default;
	}

	static Ref<Font> first_valid_font(const Ref<Font>& p_primary, const Ref<Font>& p_secondary, const Ref<Font>& p_fallback) {
		if (p_primary.is_valid()) {
			return p_primary;
		}
		if (p_secondary.is_valid()) {
			return p_secondary;
		}

		return p_fallback;
	}

	static Ref<Font> get_span_font(const MarkdownInlineSpan& p_span, const MarkdownThemeCache& p_cache, const Ref<Font>& p_normal_font) {
		if (p_span.code) {
			return p_cache.font.code.is_valid() ? p_cache.font.code : p_normal_font;
		}
		if (p_span.bold && p_span.italic) {
			return first_valid_font(p_cache.font.bold_italic, Ref<Font>(), p_normal_font);
		}
		if (p_span.bold) {
			return first_valid_font(p_cache.font.bold, Ref<Font>(), p_normal_font);
		}
		if (p_span.italic) {
			return first_valid_font(p_cache.font.italic, Ref<Font>(), p_normal_font);
		}

		return p_normal_font;
	}


	static HashMap<String, Ref<Texture2D>> s_image_cache;

	static Ref<Texture2D> load_image_texture(const String& p_uri) {
		if (s_image_cache.has(p_uri)) {
			return s_image_cache[p_uri];
		}
		Ref<Texture2D> tex = ResourceLoader::get_singleton()->load(p_uri);
		if (tex.is_valid()) {
			s_image_cache[p_uri] = tex;
		}
		return tex;
	}

	static Vector2 fit_texture_size(const Ref<Texture2D>& p_texture, float p_max_width) {
		if (p_texture.is_null() || p_texture->get_width() <= 0 || p_texture->get_height() <= 0) {
			return Vector2(1.0f, 1.0f);
		}

		const float texture_width = static_cast<float>(p_texture->get_width());
		const float texture_height = static_cast<float>(p_texture->get_height());
		const float max_width = std::max<float>(1.0f, p_max_width);
		const float scale = texture_width > max_width ? max_width / texture_width : 1.0f;
		return Vector2(std::max<float>(1.0f, texture_width * scale), std::max<float>(1.0f, texture_height * scale));
	}

	static void add_spans_to_paragraph(const Ref<TextParagraph>& p_paragraph, const String& p_text, const std::vector<MarkdownInlineSpan>& p_spans, const MarkdownThemeCache& p_cache, const Ref<Font>& p_normal_font, int32_t p_font_size, HashMap<String, Ref<Texture2D>>& p_image_map, float p_max_width) {
		int64_t pos = 0;
		for (const MarkdownInlineSpan& span : p_spans) {
			if (span.start > pos) {
				p_paragraph->add_string(p_text.substr(pos, span.start - pos), p_normal_font, std::max<int32_t>(1, p_font_size));
			}
			if (!span.image_uri.is_empty()) {
				Ref<Texture2D> tex = load_image_texture(span.image_uri);
				if (tex.is_valid()) {
					String name = String("img_") + String::num_int64(p_image_map.size());
					p_image_map[name] = tex;
					const int32_t object_length = std::max<int32_t>(1, static_cast<int32_t>(span.end - span.start));
					p_paragraph->add_object(name, fit_texture_size(tex, p_max_width), INLINE_ALIGNMENT_CENTER, object_length);
				}
				else {
					MarkdownInlineSpan fallback_span = span;
					fallback_span.italic = true;
					const Ref<Font> font = get_span_font(fallback_span, p_cache, p_normal_font);
					p_paragraph->add_string(span.text, font, std::max<int32_t>(1, p_font_size));
				}
			}
			else {
				const Ref<Font> font = get_span_font(span, p_cache, p_normal_font);
				const int32_t font_size = span.code ? p_cache.font_size.code : p_font_size;
				p_paragraph->add_string(span.text, font, std::max<int32_t>(1, font_size));
			}
			pos = span.end;
		}
		if (pos < static_cast<int64_t>(p_text.length())) {
			p_paragraph->add_string(p_text.substr(pos), p_normal_font, std::max<int32_t>(1, p_font_size));
		}
	}

	static Vector2 get_inline_code_vertical_padding(const std::vector<MarkdownInlineSpan>& p_spans, const MarkdownThemeCache& p_cache) {
		bool has_code_span = false;
		for (const MarkdownInlineSpan& span : p_spans) {
			if (span.code) {
				has_code_span = true;
				break;
			}
		}
		if (!has_code_span) {
			return Vector2();
		}

		const float margin_top = get_stylebox_margin_or(p_cache.stylebox.inline_code, SIDE_TOP, 1.0f);
		const float margin_bottom = get_stylebox_margin_or(p_cache.stylebox.inline_code, SIDE_BOTTOM, 1.0f);
		return Vector2(std::max<float>(0.0f, margin_top), std::max<float>(0.0f, margin_bottom));
	}

	static bool is_search_word_character(char32_t p_character) {
		return (p_character >= 'a' && p_character <= 'z') || (p_character >= 'A' && p_character <= 'Z') || (p_character >= '0' && p_character <= '9') || p_character == '_';
	}
	static bool is_whole_word_match(const String& p_text, int64_t p_start, int64_t p_length) {
		if (p_start > 0 && is_search_word_character(p_text[p_start - 1])) {
			return false;
		}

		const int64_t end = p_start + p_length;
		if (end < p_text.length() && is_search_word_character(p_text[end])) {
			return false;
		}

		return true;
	}

	static const char* block_spacing_type(const MarkdownBlock& p_block) {
		switch (p_block.type) {
		case MARKDOWN_BLOCK_HEADING:
			return "heading";
		case MARKDOWN_BLOCK_CODE:
			return "code_block";
		case MARKDOWN_BLOCK_TABLE:
			return "table";
		case MARKDOWN_BLOCK_BLOCKQUOTE:
			return "blockquote";
		case MARKDOWN_BLOCK_TASK_LIST:
		case MARKDOWN_BLOCK_UNORDERED_LIST:
		case MARKDOWN_BLOCK_ORDERED_LIST:
			return "list";
		default:
			return "text";
		}
	}

} // !namespace

void MarkdownThemeCache::build(Control* p_owner, int32_t p_extra_font_size) {
	// Font sizes
	font_size.text = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "text_font_size", 16) + p_extra_font_size);
	font_size.code = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "code_text_font_size", 16) + p_extra_font_size);
	font_size.code_block = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "code_block_font_size", 16) + p_extra_font_size);
	font_size.list_marker = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "list_marker_font_size", 16) + p_extra_font_size);
	font_size.table_header = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "table_header_font_size", 16) + p_extra_font_size);
	font_size.table_cell = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", "table_cell_font_size", 16) + p_extra_font_size);

	static const int32_t default_heading_sizes[] = { 28, 24, 21, 19, 17, 15 };
	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font_size";
		font_size.heading[i] = std::max<int32_t>(1, get_theme_font_size_or(p_owner, "MarkdownLabel", StringName(name), default_heading_sizes[i]) + p_extra_font_size);
	}

	// Constants
	constant.line_separation = get_theme_constant_or(p_owner, "MarkdownLabel", "line_separation", 2);
	constant.paragraph_separation = get_theme_constant_or(p_owner, "MarkdownLabel", "paragraph_separation", 10);
	constant.list_indent = std::max<int32_t>(0, get_theme_constant_or(p_owner, "MarkdownLabel", "list_indent", 26));
	constant.list_marker_gap = std::max<int32_t>(0, get_theme_constant_or(p_owner, "MarkdownLabel", "list_marker_gap", 6));
	constant.table_striped = std::max<int32_t>(0, get_theme_constant_or(p_owner, "MarkdownLabel", "table_striped", 0));
	// Fonts
	font.text = get_theme_font_or(p_owner, "MarkdownLabel", "text_font");
	font.bold = get_theme_font_or(p_owner, "MarkdownLabel", "bold_font");
	font.bold_italic = get_theme_font_or(p_owner, "MarkdownLabel", "bold_italic_font");
	if (!font.bold_italic.is_valid()) {
		font.bold_italic = font.text;
	}
	font.italic = get_theme_font_or(p_owner, "MarkdownLabel", "italic_font");
	if (!font.italic.is_valid()) {
		font.italic = font.text;
	}
	font.code = get_theme_font_or(p_owner, "MarkdownLabel", "code_text_font");
	if (!font.code.is_valid()) {
		font.code = font.text;
	}
	font.code_block = get_theme_font_or(p_owner, "MarkdownLabel", "code_block_font");
	if (!font.code_block.is_valid()) {
		font.code_block = font.code;
	}
	font.list_marker = get_theme_font_or(p_owner, "MarkdownLabel", "list_marker_font");
	if (!font.list_marker.is_valid()) {
		font.list_marker = font.text;
	}
	font.table_header = get_theme_font_or(p_owner, "MarkdownLabel", "table_header_font");
	if (!font.table_header.is_valid()) {
		font.table_header = font.text;
	}
	font.table_cell = get_theme_font_or(p_owner, "MarkdownLabel", "table_cell_font");
	if (!font.table_cell.is_valid()) {
		font.table_cell = font.text;
	}

	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font";
		font.heading[i] = get_theme_font_or(p_owner, "MarkdownLabel", StringName(name));
		if (!font.heading[i].is_valid()) {
			font.heading[i] = font.text;
		}
	}

	// Colors
	color.text = get_theme_color_or(p_owner, "MarkdownLabel", "text_font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.link = get_theme_color_or(p_owner, "MarkdownLabel", "link_color", Color(0.36f, 0.62f, 1.0f, 1.0f));
	color.selection_color = get_theme_color_or(p_owner, "MarkdownLabel", "selection_color", Color(0.1f, 0.1f, 1.0f, 0.8f));
	stylebox.selection = get_theme_stylebox_or(p_owner, "MarkdownLabel", "selection_stylebox");
	if (!stylebox.selection.is_valid()) {
		stylebox.selection = create_internal_stylebox(color.selection_color, Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	}
	color.code_text = get_theme_color_or(p_owner, "MarkdownLabel", "code_text_font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	color.code_block = get_theme_color_or(p_owner, "MarkdownLabel", "code_block_font_color", Color(0.86f, 0.88f, 0.91f, 1.0f));
	color.highlight = get_theme_color_or(p_owner, "MarkdownLabel", "highlight_color", Color(1.0f, 0.92f, 0.35f, 0.5f));
	color.highlight_font = get_theme_color_or(p_owner, "MarkdownLabel", "highlight_font_color", color.text);
	color.strikethrough = get_theme_color_or(p_owner, "MarkdownLabel", "strikethrough_color", Color(0.7f, 0.7f, 0.7f, 1.0f));
	color.list_marker = get_theme_color_or(p_owner, "MarkdownLabel", "list_marker_color", color.text);
	color.table_header_font = get_theme_color_or(p_owner, "MarkdownLabel", "table_header_font_color", color.text);
	color.table_cell_font = get_theme_color_or(p_owner, "MarkdownLabel", "table_cell_font_color", color.text);

	for (int32_t i = 0; i < 6; i++) {
		const String name = String("h") + String::num_int64(i + 1) + "_font_color";
		color.heading[i] = get_theme_color_or(p_owner, "MarkdownLabel", StringName(name), color.text);
	}

	// StyleBoxes
	stylebox.inline_code = get_theme_stylebox_or(p_owner, "MarkdownLabel", "inline_code");
	if (!stylebox.inline_code.is_valid()) {
		stylebox.inline_code = create_internal_stylebox(Color(0.16f, 0.18f, 0.20f, 1.0f), Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
	}

	stylebox.code_block_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "code_block_panel");
	if (!stylebox.code_block_panel.is_valid()) {
		stylebox.code_block_panel = create_internal_stylebox(Color(0.08f, 0.09f, 0.11f, 1.0f), Color(0, 0, 0, 0), 0, 0, 0, 0, 8.0f, 8.0f, 8.0f, 8.0f);
	}

	stylebox.blockquote = get_theme_stylebox_or(p_owner, "MarkdownLabel", "blockquote_panel");
	if (!stylebox.blockquote.is_valid()) {
		const int32_t border_width = std::max<int32_t>(1, get_theme_constant_or(p_owner, "MarkdownLabel", "blockquote_border_width", 4));
		stylebox.blockquote = create_internal_stylebox(Color(0.94f, 0.94f, 0.94f, 1.0f), Color(0.18f, 0.40f, 0.95f, 1.0f), border_width, 0, 0, 0, 28.0f, 16.0f, 8.0f, 16.0f, 4);
	}

	stylebox.blockquote_nested = get_theme_stylebox_or(p_owner, "MarkdownLabel", "blockquote_nested");
	if (!stylebox.blockquote_nested.is_valid()) {
		const int32_t border_width = std::max<int32_t>(1, get_theme_constant_or(p_owner, "MarkdownLabel", "blockquote_nested_border_width", get_theme_constant_or(p_owner, "MarkdownLabel", "blockquote_border_width", 4)));
		stylebox.blockquote_nested = create_internal_stylebox(Color(0, 0, 0, 0), Color(0.18f, 0.40f, 0.95f, 1.0f), border_width, 0, 0, 0, 28.0f, 0.0f, 8.0f, 0.0f);
	}

	stylebox.search_result = get_theme_stylebox_or(p_owner, "MarkdownLabel", "search_result");
	if (!stylebox.search_result.is_valid()) {
		stylebox.search_result = create_internal_stylebox(Color(1.0f, 0.72f, 0.16f, 0.42f), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	}

	stylebox.highlight = get_theme_stylebox_or(p_owner, "MarkdownLabel", "highlight");
	if (!stylebox.highlight.is_valid()) {
		stylebox.highlight = create_internal_stylebox(Color(1.0f, 0.92f, 0.35f, 0.5f), Color(0, 0, 0, 0), 0, 0, 0, 0, 2.0f, 1.0f, 2.0f, 1.0f);
	}

	stylebox.separator = get_theme_stylebox_or(p_owner, "MarkdownLabel", "separator");
	if (!stylebox.separator.is_valid()) {
		stylebox.separator = create_internal_stylebox(Color(0.28f, 0.30f, 0.34f, 1.0f), Color(0, 0, 0, 0), 0, 0, 0, 0, 0.0f, 2.0f, 0.0f, 1.0f);
	}

	stylebox.table_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_panel");

	stylebox.table_header_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_header_panel");
	if (!stylebox.table_header_panel.is_valid()) {
		stylebox.table_header_panel = create_internal_stylebox(Color(0.18f, 0.19f, 0.22f, 1.0f), Color(0.28f, 0.30f, 0.34f, 1.0f), 1, 1, 1, 1, 6.0f, 4.0f, 6.0f, 4.0f);
	}

	stylebox.table_cell_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_cell_panel");
	if (!stylebox.table_cell_panel.is_valid()) {
		stylebox.table_cell_panel = create_internal_stylebox(Color(0, 0, 0, 0), Color(0.28f, 0.30f, 0.34f, 1.0f), 1, 1, 1, 1, 6.0f, 4.0f, 6.0f, 4.0f);
	}

	stylebox.table_striped_panel = get_theme_stylebox_or(p_owner, "MarkdownLabel", "table_striped_panel");
	if (!stylebox.table_striped_panel.is_valid()) {
		stylebox.table_striped_panel = create_internal_stylebox(Color(0.10f, 0.11f, 0.13f, 1.0f), Color(0.28f, 0.30f, 0.34f, 1.0f), 1, 1, 1, 1, 6.0f, 4.0f, 6.0f, 4.0f);
	}

	icon.task_checked = get_theme_icon_or(p_owner, "MarkdownLabel", "task_checked");
	if (!icon.task_checked.is_valid()) {
		icon.task_checked = ResourceLoader::get_singleton()->load("uid://urg40ascuevg");
	}
	icon.task_unchecked = get_theme_icon_or(p_owner, "MarkdownLabel", "task_unchecked");
	if (!icon.task_unchecked.is_valid()) {
		icon.task_unchecked = ResourceLoader::get_singleton()->load("uid://dygdo6qf74db");
	}
}

// MarkdownLabelCanvas
void MarkdownLabelCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_markdown_text", "text"), &MarkdownLabelCanvas::set_markdown_text);
	ClassDB::bind_method(D_METHOD("append_text_incremental", "delta"), &MarkdownLabelCanvas::append_text_incremental);
	ClassDB::bind_method(D_METHOD("finish_stream"), &MarkdownLabelCanvas::finish_stream);
	ClassDB::bind_method(D_METHOD("clear_document"), &MarkdownLabelCanvas::clear_document);
	ClassDB::bind_method(D_METHOD("set_streaming_enabled", "enabled"), &MarkdownLabelCanvas::set_streaming_enabled);
	ClassDB::bind_method(D_METHOD("set_max_unstable_lines", "lines"), &MarkdownLabelCanvas::set_max_unstable_lines);
	ClassDB::bind_method(D_METHOD("select_all"), &MarkdownLabelCanvas::select_all);
	ClassDB::bind_method(D_METHOD("select_between_points", "from", "to"), &MarkdownLabelCanvas::select_between_points);
	ClassDB::bind_method(D_METHOD("get_link_uri_at_point", "position"), &MarkdownLabelCanvas::get_link_uri_at_point);
	ClassDB::bind_method(D_METHOD("clear_selection"), &MarkdownLabelCanvas::clear_selection);
	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &MarkdownLabelCanvas::search);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &MarkdownLabelCanvas::get_selected_text);
	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &MarkdownLabelCanvas::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_content_height"), &MarkdownLabelCanvas::get_content_height);
	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &MarkdownLabelCanvas::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &MarkdownLabelCanvas::get_anchors);
	ClassDB::bind_method(D_METHOD("get_anchor_offset", "anchor"), &MarkdownLabelCanvas::get_anchor_offset);
	ClassDB::bind_method(D_METHOD("get_search_result_y"), &MarkdownLabelCanvas::get_search_result_y);
	ClassDB::bind_method(D_METHOD("get_bottom_position"), &MarkdownLabelCanvas::get_bottom_position);
}

void MarkdownLabelCanvas::set_label(MarkdownLabel* p_label) {
	label = p_label;
	mark_layout_dirty();
}

void MarkdownLabelCanvas::set_markdown_text(const String& p_text) {
	raw_text = p_text;
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	parser_state.reset();
	cached_blocks.clear();
	set_process_internal(false);
	mark_layout_dirty();
}

void MarkdownLabelCanvas::append_text_incremental(const String& p_delta) {
	raw_text += p_delta;

	if (streaming_enabled) {
		std::vector<String> all_lines = split_lines(raw_text);
		int32_t reparse_line = find_reparse_line(cached_blocks, all_lines, parser_state, max_unstable_lines);
		rebuild_tail(raw_text, reparse_line, cached_blocks, parser_state);
	}
	else {
		parser_state.reset();
		cached_blocks = parse_markdown_blocks(raw_text);
	}

	mark_layout_dirty();
}

void MarkdownLabelCanvas::finish_stream() {
	parser_state.reset();
	cached_blocks = parse_markdown_blocks(raw_text);
	mark_layout_dirty();
}

void MarkdownLabelCanvas::clear_document() {
	raw_text = String();
	plain_text = String();
	items.clear();
	lines.clear();
	cached_blocks.clear();
	parser_state.reset();
	rendered_block_count = 0;
	selection_anchor = 0;
	selection_caret = 0;
	search_match_start = -1;
	search_match_end = -1;
	dragging_selection = false;
	selection_dragged = false;
	set_process_internal(false);
	content_height = 1.0f;
	layout_dirty = false;
	anchor_offsets.clear();
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();
	queue_redraw();
}

void MarkdownLabelCanvas::set_streaming_enabled(bool p_enabled) {
	streaming_enabled = p_enabled;
}

void MarkdownLabelCanvas::set_max_unstable_lines(int32_t p_lines) {
	max_unstable_lines = std::max<int32_t>(1, std::min<int32_t>(p_lines, 256));
}

void MarkdownLabelCanvas::mark_layout_dirty() {
	layout_dirty = true;
	queue_redraw();
}

bool MarkdownLabelCanvas::has_selection() const {
	return selection_anchor != selection_caret;
}

void MarkdownLabelCanvas::select_all() {
	rebuild_layout();
	selection_anchor = 0;
	selection_caret = plain_text.length();
	queue_redraw();
}

void MarkdownLabelCanvas::select_between_points(const Vector2& p_from, const Vector2& p_to) {
	rebuild_layout();
	selection_anchor = hit_test_document(p_from);
	selection_caret = hit_test_document(p_to);
	queue_redraw();
}

String MarkdownLabelCanvas::get_link_uri_at_point(const Vector2& p_position) {
	rebuild_layout();
	return link_uri_at_position(p_position);
}


void MarkdownLabelCanvas::clear_selection() {
	selection_anchor = selection_caret;
	queue_redraw();
}

Vector2i MarkdownLabelCanvas::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) {
	rebuild_layout();

	const String needle = p_text;
	if (needle.is_empty() || plain_text.is_empty()) {
		search_match_start = -1;
		search_match_end = -1;
		queue_redraw();
		return Vector2i(-1, -1);
	}

	const bool match_case = (p_flags & 1) != 0;
	const bool whole_words = (p_flags & 2) != 0;
	const bool backwards = (p_flags & 4) != 0;
	const String haystack = match_case ? plain_text : plain_text.to_lower();
	const String search_needle = match_case ? needle : needle.to_lower();
	const int64_t needle_length = search_needle.length();
	int64_t start_offset = line_column_to_offset(p_from_line, p_from_column);

	if (backwards) {
		int64_t index = std::min<int64_t>(start_offset, std::max<int64_t>(0, haystack.length() - needle_length));
		while (index >= 0) {
			if (haystack.substr(static_cast<int32_t>(index), static_cast<int32_t>(needle_length)) == search_needle && (!whole_words || is_whole_word_match(plain_text, index, needle_length))) {
				search_match_start = index;
				search_match_end = index + needle_length;
				queue_redraw();
				return offset_to_line_column(index);
			}
			index--;
		}
	}
	else {
		int64_t index = std::max<int64_t>(0, std::min<int64_t>(start_offset, haystack.length()));
		while (index <= haystack.length() - needle_length) {
			const int64_t found = haystack.find(search_needle, static_cast<int32_t>(index));
			if (found < 0) {
				break;
			}
			if (!whole_words || is_whole_word_match(plain_text, found, needle_length)) {
				search_match_start = found;
				search_match_end = found + needle_length;
				queue_redraw();
				return offset_to_line_column(found);
			}
			index = found + 1;
		}
	}

	search_match_start = -1;
	search_match_end = -1;
	queue_redraw();
	return Vector2i(-1, -1);
}

String MarkdownLabelCanvas::get_selected_text() const {
	const int64_t from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t to = std::max<int64_t>(selection_anchor, selection_caret);
	if (from >= to || from < 0 || to > plain_text.length()) {
		return String();
	}

	return plain_text.substr(static_cast<int32_t>(from), static_cast<int32_t>(to - from));
}

int32_t MarkdownLabelCanvas::get_rendered_block_count() {
	rebuild_layout();
	return rendered_block_count;
}

float MarkdownLabelCanvas::get_content_height() {
	rebuild_layout();
	return content_height;
}

bool MarkdownLabelCanvas::has_anchor(const String& p_anchor) const {
	return anchor_offsets.has(p_anchor);
}

PackedStringArray MarkdownLabelCanvas::get_anchors() const {
	PackedStringArray result;
	const Array keys = anchor_offsets.keys();
	for (int32_t i = 0; i < keys.size(); i++) {
		result.append(keys[i]);
	}
	return result;
}

float MarkdownLabelCanvas::get_anchor_offset(const String& p_anchor) const {
	if (!anchor_offsets.has(p_anchor)) {
		return -1.0f;
	}
	return anchor_offsets[p_anchor];
}

float MarkdownLabelCanvas::get_search_result_y() const {
	if (search_match_start < 0 || search_match_end <= search_match_start) {
		return -1.0f;
	}

	for (const MarkdownCanvasItem& item : items) {
		if (item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownBlockquoteLine& line : item.quote_lines) {
				if (search_match_start >= line.global_start && search_match_start < line.global_end) {
					return item.rect.position.y;
				}
			}
		}
		if (search_match_start >= item.paragraph_global_start && search_match_start < item.paragraph_global_end) {
			return item.rect.position.y;
		}
		if (search_match_start >= item.header_global_start && search_match_start < item.header_global_end) {
			return item.rect.position.y;
		}
	}

	return -1.0f;
}

float MarkdownLabelCanvas::get_bottom_position() const {
	return content_height;
}

Vector2 MarkdownLabelCanvas::_get_minimum_size() const {
	return Vector2(1.0, content_height);
}

void MarkdownLabelCanvas::build_theme_cache() {
	const int32_t extra = label == nullptr ? 0 : label->get_extra_font_size();
	theme_cache.build(label, extra);
}

void MarkdownLabelCanvas::rebuild_layout() {
	const float width = std::max<float>(1.0f, get_size().x);
	if (!layout_dirty && std::abs(width - layout_width) < 0.01f) {
		return;
	}

	layout_dirty = false;
	layout_width = width;
	items.clear();
	lines.clear();
	plain_text = String();
	rendered_block_count = 0;

	build_theme_cache();

	float y = 0.0f;

	std::vector<MarkdownBlock> blocks;
	if (streaming_enabled && !cached_blocks.empty()) {
		blocks = cached_blocks;
	}
	else {
		blocks = parse_markdown_blocks(raw_text);
		cached_blocks = blocks;
	}

	for (int32_t block_idx = 0; block_idx < static_cast<int32_t>(blocks.size()); block_idx++) {
		const MarkdownBlock& block = blocks[block_idx];
		MarkdownCanvasItem item;
		String item_text;
		std::vector<MarkdownInlineSpan> item_spans;
		Ref<Font> font = theme_cache.font.text;
		int32_t font_size = theme_cache.font_size.text;
		float left_padding = 0.0f;
		float right_padding = 0.0f;
		float top_padding = 0.0f;
		float bottom_padding = 0.0f;
		const char* spacing_type = block_spacing_type(block);
		const String spacing_before_name = String(spacing_type) + "_space_before";
		const String spacing_after_name = String(spacing_type) + "_space_after";
		const int32_t block_spacing_before = std::max<int32_t>(0, get_theme_constant_or(label, "MarkdownLabel", StringName(spacing_before_name), 0));
		const int32_t block_spacing_after = std::max<int32_t>(0, get_theme_constant_or(label, "MarkdownLabel", StringName(spacing_after_name), theme_cache.constant.paragraph_separation));

		item.text_color = theme_cache.color.text;
		item.background_color = Color(0, 0, 0, 0);
		item.border_color = Color(0, 0, 0, 0);
		item.type = block.type;

		switch (block.type) {
		case MARKDOWN_BLOCK_HEADING: {
			const int32_t index = std::min<int32_t>(std::max<int32_t>(block.level - 1, 0), 5);
			font_size = theme_cache.font_size.heading[index];
			font = theme_cache.font.heading[index];
			item.text_color = theme_cache.color.heading[index];
			item.heading = true;
			item.heading_level = block.level;
			item.anchor = block.anchor;

			MarkdownInlineSpanParseResult parsed = parse_inline_spans(block.text);
			item_text = parsed.output_text;
			item_spans = parsed.spans;
		} break;
		case MARKDOWN_BLOCK_TASK_LIST:
		case MARKDOWN_BLOCK_UNORDERED_LIST:
		case MARKDOWN_BLOCK_ORDERED_LIST: {
			for (int32_t i = 0; i < block.items.size(); i++) {
				if (!item_text.is_empty()) {
					item_text += "\n";
				}
				MarkdownInlineSpanParseResult parsed = parse_inline_spans(block.items[i]);
				const int64_t item_start = item_text.length();
				item_text += parsed.output_text;
				for (MarkdownInlineSpan& span : parsed.spans) {
					span.start += item_start;
					span.end += item_start;
					item_spans.push_back(span);
				}
			}
			left_padding = static_cast<float>(theme_cache.constant.list_indent) * (1.0f + static_cast<float>(block.indent));
		} break;
		case MARKDOWN_BLOCK_CODE:
			item_text = block.text;
			font = theme_cache.font.code_block;
			font_size = theme_cache.font_size.code_block;
			item.text_color = theme_cache.color.code_block;
			item.background_color = get_theme_color_or(label, "MarkdownLabel", "code_block_background_color", Color(0.08f, 0.09f, 0.11f, 1.0f));
			item.panel_stylebox = theme_cache.stylebox.code_block_panel;
			item.framed = true;
			item.code = true;
			item.code_language = block.argument;
			left_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_LEFT, 8.0f);
			top_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_TOP, 8.0f);
			right_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_RIGHT, 8.0f);
			bottom_padding = get_stylebox_margin_or(item.panel_stylebox, SIDE_BOTTOM, 8.0f);
			break;
		case MARKDOWN_BLOCK_BLOCKQUOTE: {
			if (y > 0.0f) {
				y += static_cast<float>(block_spacing_before);
			}

			item.panel_stylebox = theme_cache.stylebox.blockquote;
			item.framed = true;
			item.blockquote_depth = 1;
			item.type = MARKDOWN_BLOCK_BLOCKQUOTE;

			const float panel_top = get_stylebox_margin_or(item.panel_stylebox, SIDE_TOP, 16.0f);
			const float panel_right = get_stylebox_margin_or(item.panel_stylebox, SIDE_RIGHT, 8.0f);
			const float panel_bottom = get_stylebox_margin_or(item.panel_stylebox, SIDE_BOTTOM, 16.0f);
			const float panel_left = get_stylebox_margin_or(item.panel_stylebox, SIDE_LEFT, 28.0f);
			const float quote_indent = static_cast<float>(std::max<int32_t>(12, get_theme_constant_or(label, "MarkdownLabel", "blockquote_indent", 25)));
			const float quote_line_gap = static_cast<float>(std::max<int32_t>(0, get_theme_constant_or(label, "MarkdownLabel", "blockquote_line_gap", 14)));
			const float nested_left = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_LEFT, 28.0f);
			const float nested_top = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_TOP, 0.0f);
			const float nested_right = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_RIGHT, 8.0f);
			const float nested_bottom = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_BOTTOM, 0.0f);
			const PackedStringArray quote_text_lines = block.text.split("\n", true);
			std::vector<int32_t> quote_depths;
			quote_depths.reserve(quote_text_lines.size());
			for (int32_t line_index = 0; line_index < quote_text_lines.size(); line_index++) {
				const int32_t depth = line_index < static_cast<int32_t>(block.item_levels.size()) ? std::max<int32_t>(1, block.item_levels[line_index]) : 1;
				quote_depths.push_back(depth);
			}

			if (!plain_text.is_empty()) {
				plain_text += "\n\n";
			}
			item.global_start = plain_text.length();
			item.paragraph_global_start = item.global_start;

			float quote_y = y + panel_top;
			for (int32_t line_index = 0; line_index < quote_text_lines.size(); line_index++) {
				const int32_t depth = quote_depths[line_index];
				const int32_t previous_depth = line_index > 0 ? quote_depths[line_index - 1] : 1;
				const int32_t next_depth = line_index + 1 < static_cast<int32_t>(quote_depths.size()) ? quote_depths[line_index + 1] : 1;
				if (depth > previous_depth) {
					quote_y += nested_top * static_cast<float>(depth - previous_depth);
				}

				float line_left = panel_left;
				float line_width = std::max<float>(1.0f, width - panel_left - panel_right);
				if (depth > 1) {
					const float nested_rect_left = panel_left + static_cast<float>(depth - 2) * quote_indent;
					line_left = nested_rect_left + nested_left;
					line_width = std::max<float>(1.0f, width - nested_rect_left - nested_left - nested_right - panel_right);
				}

				MarkdownInlineSpanParseResult parsed = parse_inline_spans(quote_text_lines[line_index]);
				const String paragraph_text = parsed.output_text.is_empty() ? String(" ") : parsed.output_text;

				MarkdownBlockquoteLine quote_line;
				quote_line.text = parsed.output_text;
				quote_line.spans = parsed.spans;
				quote_line.depth = depth;
				quote_line.paragraph.instantiate();
				quote_line.paragraph->set_width(line_width);
				quote_line.paragraph->set_line_spacing(theme_cache.constant.line_separation);
				add_spans_to_paragraph(quote_line.paragraph, paragraph_text, quote_line.spans, theme_cache, font, font_size, quote_line.image_map, line_width);

				const Vector2 paragraph_size = quote_line.paragraph->get_size();
				const Vector2 inline_code_padding = get_inline_code_vertical_padding(quote_line.spans, theme_cache);
				quote_line.text_rect = Rect2(line_left, quote_y + inline_code_padding.x, line_width, paragraph_size.y);

				if (line_index > 0) {
					plain_text += "\n";
				}
				quote_line.global_start = plain_text.length();
				plain_text += quote_line.text;
				quote_line.global_end = plain_text.length();
				item.quote_lines.push_back(quote_line);

				item.blockquote_depth = std::max<int32_t>(item.blockquote_depth, depth);
				quote_y += paragraph_size.y + inline_code_padding.x + inline_code_padding.y + quote_line_gap;
				if (next_depth < depth) {
					quote_y += nested_bottom * static_cast<float>(depth - next_depth);
				}
			}

			if (!item.quote_lines.empty()) {
				quote_y -= quote_line_gap;
			}
			item.global_end = plain_text.length();
			item.paragraph_global_end = item.global_end;
			item.rect = Rect2(0.0f, y, width, std::max<float>(1.0f, quote_y - y + panel_bottom));
			item.text_rect = item.rect;
			item.text_color = theme_cache.color.text;
			items.push_back(item);
			y += item.rect.size.y + static_cast<float>(block_spacing_after);
			rendered_block_count++;
			continue;
		}
		case MARKDOWN_BLOCK_THEMATIC_BREAK:
			item.panel_stylebox = theme_cache.stylebox.separator;
			item.framed = true;
			break;
		case MARKDOWN_BLOCK_IMAGE: {
			item_text = String("[Image: ") + block.argument + "]";
			MarkdownInlineSpan span;
			span.text = item_text;
			span.italic = true;
			span.start = 0;
			span.end = item_text.length();
			item_spans.push_back(span);
		} break;
		case MARKDOWN_BLOCK_TABLE: {
			item.type = MARKDOWN_BLOCK_TABLE;
			const int32_t columns = block.columns > 0 ? block.columns : 1;
			const float table_left = get_stylebox_margin_or(theme_cache.stylebox.table_panel, SIDE_LEFT, 0.0f);
			const float table_top = get_stylebox_margin_or(theme_cache.stylebox.table_panel, SIDE_TOP, 0.0f);
			const float table_right = get_stylebox_margin_or(theme_cache.stylebox.table_panel, SIDE_RIGHT, 0.0f);
			const float column_width = std::max<float>(1.0f, (width - table_left - table_right) / static_cast<float>(columns));
			const bool striped = theme_cache.constant.table_striped != 0;
			item.panel_stylebox = theme_cache.stylebox.table_panel;

			std::vector<float> row_heights;
			for (int32_t row_idx = 0; row_idx < block.rows.size(); row_idx++) {
				float row_height = 0.0f;
				for (int32_t col_idx = 0; col_idx < columns && col_idx < block.rows[row_idx].size(); col_idx++) {
					MarkdownTableCell cell;
					cell.header = (row_idx < block.header_rows);
					cell.text = block.rows[row_idx][col_idx];
					MarkdownInlineSpanParseResult parsed = parse_inline_spans(cell.text);
					cell.text = parsed.output_text;
					cell.spans = parsed.spans;
					const Ref<Font> cell_font = cell.header ? theme_cache.font.table_header : theme_cache.font.table_cell;
					const int32_t cell_font_size = cell.header ? theme_cache.font_size.table_header : theme_cache.font_size.table_cell;
					cell.text_color = cell.header ? theme_cache.color.table_header_font : theme_cache.color.table_cell_font;

					if (cell.header) {
						cell.stylebox = theme_cache.stylebox.table_header_panel;
					}
					else if (striped && row_idx % 2 == 0) {
						cell.stylebox = theme_cache.stylebox.table_striped_panel;
					}
					else {
						cell.stylebox = theme_cache.stylebox.table_cell_panel;
					}

					const float cell_left = get_stylebox_margin_or(cell.stylebox, SIDE_LEFT, 6.0f);
					const float cell_top = get_stylebox_margin_or(cell.stylebox, SIDE_TOP, 4.0f);
					const float cell_right = get_stylebox_margin_or(cell.stylebox, SIDE_RIGHT, 6.0f);
					const float cell_bottom = get_stylebox_margin_or(cell.stylebox, SIDE_BOTTOM, 4.0f);

					cell.paragraph.instantiate();
					cell.paragraph->set_width(std::max<float>(1.0f, column_width - cell_left - cell_right));
					cell.paragraph->set_line_spacing(theme_cache.constant.line_separation);
					add_spans_to_paragraph(cell.paragraph, cell.text, cell.spans, theme_cache, cell_font, cell_font_size, cell.image_map, std::max<float>(1.0f, column_width - cell_left - cell_right));

					const float cell_height = cell.paragraph->get_size().y + cell_top + cell_bottom;
					row_height = std::max<float>(row_height, cell_height);
					item.cells.push_back(cell);
				}
				row_heights.push_back(row_height);
			}

			float table_y = y + table_top;
			int32_t cell_index = 0;
			for (int32_t row_idx = 0; row_idx < block.rows.size(); row_idx++) {
				for (int32_t col_idx = 0; col_idx < columns && cell_index < static_cast<int32_t>(item.cells.size()); col_idx++, cell_index++) {
					MarkdownTableCell& cell = item.cells[cell_index];
					const float cell_left = get_stylebox_margin_or(cell.stylebox, SIDE_LEFT, 6.0f);
					const float cell_top = get_stylebox_margin_or(cell.stylebox, SIDE_TOP, 4.0f);
					cell.rect = Rect2(table_left + static_cast<float>(col_idx) * column_width, table_y, column_width, row_heights[row_idx]);
					cell.text_rect = Rect2(cell.rect.position.x + cell_left, cell.rect.position.y + cell_top, column_width - cell_left - get_stylebox_margin_or(cell.stylebox, SIDE_RIGHT, 6.0f), cell.paragraph->get_size().y);

					if (!plain_text.is_empty()) plain_text += " ";
					cell.global_start = plain_text.length();
					plain_text += cell.text;
					cell.global_end = plain_text.length();
				}
				table_y += row_heights[row_idx];
			}

			item.rect = Rect2(0.0f, y, width, table_y - y + get_stylebox_margin_or(theme_cache.stylebox.table_panel, SIDE_BOTTOM, 0.0f));
			item.text_rect = item.rect;
			y = item.rect.get_end().y;
			rendered_block_count++;
			items.push_back(item);
			continue;
		}
		case MARKDOWN_BLOCK_PARAGRAPH:
		default: {
			MarkdownInlineSpanParseResult parsed = parse_inline_spans(block.text);
			item_text = parsed.output_text;
			item_spans = parsed.spans;
		} break;
		}

		if (item_text.is_empty() && block.type != MARKDOWN_BLOCK_THEMATIC_BREAK) {
			continue;
		}

		if (block.type == MARKDOWN_BLOCK_THEMATIC_BREAK) {
			const float sep_height = get_stylebox_margin_or(item.panel_stylebox, SIDE_TOP, 2.0f) + get_stylebox_margin_or(item.panel_stylebox, SIDE_BOTTOM, 1.0f);
			if (y > 0.0f) {
				y += static_cast<float>(block_spacing_before);
			}
			item.rect = Rect2(0.0f, y, width, std::max<float>(1.0f, sep_height));
			item.text_rect = item.rect;
			y += item.rect.size.y + static_cast<float>(block_spacing_after);
			items.push_back(item);
			rendered_block_count++;
			continue;
		}

		if (!plain_text.is_empty()) {
			plain_text += "\n\n";
		}
		item.global_start = plain_text.length();
		plain_text += item_text;
		item.global_end = plain_text.length();
		item.paragraph_global_start = item.global_start;
		item.paragraph_global_end = item.global_end;
		item.text = item_text;
		item.spans = item_spans;

		uint64_t block_hash = compute_block_hash(block);
		item.content_hash = block_hash;

		item.paragraph.instantiate();
		item.paragraph->set_width(std::max<float>(1.0f, width - left_padding - right_padding));
		item.paragraph->set_line_spacing(theme_cache.constant.line_separation);
		add_spans_to_paragraph(item.paragraph, item_text, item.spans, theme_cache, font, font_size, item.image_map, std::max<float>(1.0f, width - left_padding - right_padding));

		const Vector2 paragraph_size = item.paragraph->get_size();
		const Vector2 inline_code_padding = get_inline_code_vertical_padding(item.spans, theme_cache);
		top_padding += inline_code_padding.x;
		bottom_padding += inline_code_padding.y;
		if (y > 0.0f) {
			y += static_cast<float>(block_spacing_before);
		}
		item.text_rect = Rect2(left_padding, y + top_padding, std::max<float>(1.0f, width - left_padding - right_padding), paragraph_size.y);
		item.rect = Rect2(0.0f, y, width, paragraph_size.y + top_padding + bottom_padding);

		const int32_t item_index = static_cast<int32_t>(items.size());
		for (int32_t line_index = 0; line_index < item.paragraph->get_line_count(); line_index++) {
			const Vector2i range = item.paragraph->get_line_range(line_index);
			MarkdownCanvasLine line;
			line.item_index = item_index;
			line.paragraph_line = line_index;
			line.global_start = item.paragraph_global_start + range.x;
			line.global_end = item.paragraph_global_start + range.y;
			line.position = item.text_rect.position;
			for (int32_t previous_line = 0; previous_line < line_index; previous_line++) {
				line.position.y += item.paragraph->get_line_size(previous_line).y + theme_cache.constant.line_separation;
			}
			line.size = item.paragraph->get_line_size(line_index);
			lines.push_back(line);
		}

		if ((block.type == MARKDOWN_BLOCK_UNORDERED_LIST || block.type == MARKDOWN_BLOCK_ORDERED_LIST || block.type == MARKDOWN_BLOCK_TASK_LIST)) {
			float list_line_y = item.text_rect.position.y;
			for (int32_t marker_index = 0; marker_index < block.items.size(); marker_index++) {
				String marker_text;
				if (block.type == MARKDOWN_BLOCK_ORDERED_LIST) {
					marker_text = String::num_int64(marker_index + 1) + ".";
				}
				else if (block.type == MARKDOWN_BLOCK_TASK_LIST) {
					marker_text = " ";
				}
				else {
					marker_text = String::chr(0x2022);
				}
				float marker_size_x = 0.0f;
				if (block.type == MARKDOWN_BLOCK_TASK_LIST) {
					marker_size_x = static_cast<float>(theme_cache.font_size.list_marker);
				} else if (theme_cache.font.list_marker.is_valid()) {
					const Vector2 ms = theme_cache.font.list_marker->get_string_size(marker_text, HORIZONTAL_ALIGNMENT_RIGHT, static_cast<int32_t>(item.text_rect.position.x - theme_cache.constant.list_marker_gap), theme_cache.font_size.list_marker);
					marker_size_x = ms.x;
				}
				float marker_x = std::max<float>(0.0f, item.text_rect.position.x - static_cast<float>(theme_cache.constant.list_marker_gap) - marker_size_x);
				float marker_y = list_line_y + item.paragraph->get_line_ascent(0);

				MarkdownListMarker marker;
				marker.text = marker_text;
				marker.position = Vector2(std::max<float>(0.0f, marker_x), marker_y);
				marker.font = theme_cache.font.list_marker;
				marker.font_size = theme_cache.font_size.list_marker;
				marker.color = theme_cache.color.list_marker;
				if (block.type == MARKDOWN_BLOCK_TASK_LIST && marker_index < static_cast<int32_t>(block.task_checked.size())) {
					marker.task_checked = block.task_checked[marker_index];
					marker.icon = marker.task_checked ? theme_cache.icon.task_checked : theme_cache.icon.task_unchecked;
				}
				item.list_markers.push_back(marker);
				list_line_y += item.paragraph->get_line_size(0).y + theme_cache.constant.line_separation;
			}
		}

		items.push_back(item);
		y += item.rect.size.y + static_cast<float>(block_spacing_after);
		if (!block.anchor.is_empty()) {
			anchor_offsets[block.anchor] = item.rect.position.y;
		}
		rendered_block_count++;
	}

	content_height = std::max<float>(1.0f, y);
	set_custom_minimum_size(Vector2(1.0, content_height));
	update_minimum_size();

	const int64_t text_length = plain_text.length();
	selection_anchor = std::min<int64_t>(selection_anchor, text_length);
	selection_caret = std::min<int64_t>(selection_caret, text_length);
	if (search_match_start >= text_length || search_match_end > text_length || search_match_end <= search_match_start) {
		search_match_start = -1;
		search_match_end = -1;
	}
}

void MarkdownLabelCanvas::draw_selection_for_item(const MarkdownCanvasItem& p_item) {
	if (p_item.type == MARKDOWN_BLOCK_TABLE) {
		for (const MarkdownTableCell& cell : p_item.cells) {
			draw_selection_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end);
		}
		return;
	}
	if (p_item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
		for (const MarkdownBlockquoteLine& line : p_item.quote_lines) {
			draw_selection_for_paragraph(line.paragraph, line.text_rect, line.global_start, line.global_end);
		}
		return;
	}
	draw_selection_for_paragraph(p_item.paragraph, p_item.text_rect, p_item.paragraph_global_start, p_item.paragraph_global_end);
}
void MarkdownLabelCanvas::draw_search_highlight_for_item(const MarkdownCanvasItem& p_item) {
	if (search_match_start < 0 || search_match_end <= search_match_start) {
		return;
	}

	if (p_item.type == MARKDOWN_BLOCK_TABLE) {
		for (const MarkdownTableCell& cell : p_item.cells) {
			if (search_match_start < cell.global_end && search_match_end > cell.global_start) {
				draw_range_background_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end, search_match_start, search_match_end, theme_cache.stylebox.search_result);
			}
		}
		return;
	}
	if (p_item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
		for (const MarkdownBlockquoteLine& line : p_item.quote_lines) {
			if (search_match_start < line.global_end && search_match_end > line.global_start) {
				draw_range_background_for_paragraph(line.paragraph, line.text_rect, line.global_start, line.global_end, search_match_start, search_match_end, theme_cache.stylebox.search_result);
			}
		}
		return;
	}
	draw_range_background_for_paragraph(p_item.paragraph, p_item.text_rect, p_item.paragraph_global_start, p_item.paragraph_global_end, search_match_start, search_match_end, theme_cache.stylebox.search_result);
}

void MarkdownLabelCanvas::draw_range_background_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end, int64_t p_range_start, int64_t p_range_end, const Ref<StyleBox>& p_stylebox) {
	if (p_paragraph.is_null()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	if (p_range_end <= p_global_start || p_range_start >= p_global_end) {
		return;
	}

	float y = p_text_rect.position.y;
	for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
		const Vector2i range = p_paragraph->get_line_range(line_index);
		const int64_t local_from = std::max<int64_t>(range.x, p_range_start - p_global_start);
		const int64_t local_to = std::min<int64_t>(range.y, p_range_end - p_global_start);
		const Vector2 line_size = p_paragraph->get_line_size(line_index);
		if (local_from < local_to) {
			const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(p_paragraph->get_line_rid(line_index), local_from, local_to);
			for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
				const Vector2 selected_range = selected_ranges[range_index];
				draw_stylebox_or_rect(p_stylebox, Rect2(p_text_rect.position.x + selected_range.x, y, selected_range.y - selected_range.x, line_size.y), Color());
			}
		}
		y += line_size.y + p_paragraph->get_line_spacing();
	}
}

void MarkdownLabelCanvas::draw_selection_for_paragraph(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, int64_t p_global_start, int64_t p_global_end) {
	if (!has_selection()) {
		return;
	}

	const int64_t selection_from = std::min<int64_t>(selection_anchor, selection_caret);
	const int64_t selection_to = std::max<int64_t>(selection_anchor, selection_caret);
	draw_range_background_for_paragraph(p_paragraph, p_text_rect, p_global_start, p_global_end, selection_from, selection_to, theme_cache.stylebox.selection);
}

void MarkdownLabelCanvas::draw_stylebox_or_rect(const Ref<StyleBox>& p_stylebox, const Rect2& p_rect, const Color& p_fallback_color) {
	if (p_stylebox.is_valid()) {
		p_stylebox->draw(get_canvas_item(), p_rect);
		return;
	}

	if (p_fallback_color.a > 0.0f) {
		draw_rect(p_rect, p_fallback_color, true);
	}
}

void MarkdownLabelCanvas::draw_paragraph_images(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const HashMap<String, Ref<Texture2D>>& p_image_map) {
	if (p_paragraph.is_null() || p_image_map.is_empty()) {
		return;
	}

	for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
		const Array objects = p_paragraph->get_line_objects(line_index);
		for (int32_t object_index = 0; object_index < objects.size(); object_index++) {
			const String object_name = objects[object_index];
			if (!p_image_map.has(object_name)) {
				continue;
			}

			const Ref<Texture2D> texture = p_image_map[object_name];
			if (texture.is_null()) {
				continue;
			}

			const Rect2 object_rect = p_paragraph->get_line_object_rect(line_index, object_name);
			texture->draw_rect(get_canvas_item(), Rect2(p_text_rect.position + object_rect.position, object_rect.size), false);
		}
	}
}

void MarkdownLabelCanvas::draw_span_decorations(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<MarkdownInlineSpan>& p_spans, int64_t p_global_start, bool p_draw_code_backgrounds, bool p_draw_link_underlines) {
	if (p_paragraph.is_null() || p_spans.empty()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	const Color& link_color = theme_cache.color.link;

	for (const MarkdownInlineSpan& span : p_spans) {
		if (!(p_draw_code_backgrounds && (span.code || span.highlight)) && !(p_draw_link_underlines && (!span.link_uri.is_empty() || span.strikethrough))) {
			continue;
		}

		float y = p_text_rect.position.y;
		for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
			const Vector2i range = p_paragraph->get_line_range(line_index);
			const int64_t local_from = std::max<int64_t>(range.x, span.start);
			const int64_t local_to = std::min<int64_t>(range.y, span.end);
			const Vector2 line_size = p_paragraph->get_line_size(line_index);
			if (local_from < local_to) {
				const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(p_paragraph->get_line_rid(line_index), local_from, local_to);
				for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
					const Vector2 selected_range = selected_ranges[range_index];
						if (p_draw_code_backgrounds && span.highlight) {
							const float hl_top = get_stylebox_margin_or(theme_cache.stylebox.highlight, SIDE_TOP, 1.0f);
							const float hl_bottom = get_stylebox_margin_or(theme_cache.stylebox.highlight, SIDE_BOTTOM, 1.0f);
							const float hl_left = get_stylebox_margin_or(theme_cache.stylebox.highlight, SIDE_LEFT, 2.0f);
							const float hl_right = get_stylebox_margin_or(theme_cache.stylebox.highlight, SIDE_RIGHT, 2.0f);
							const Rect2 hl_rect = Rect2(p_text_rect.position.x + selected_range.x - hl_left, y - hl_top, selected_range.y - selected_range.x + hl_left + hl_right, line_size.y + hl_top + hl_bottom);
							draw_stylebox_or_rect(theme_cache.stylebox.highlight, hl_rect, theme_cache.color.highlight);
						}
						if (p_draw_code_backgrounds && span.code) {
						const float margin_left = get_stylebox_margin_or(theme_cache.stylebox.inline_code, SIDE_LEFT, 2.0f);
						const float margin_top = get_stylebox_margin_or(theme_cache.stylebox.inline_code, SIDE_TOP, 1.0f);
						const float margin_right = get_stylebox_margin_or(theme_cache.stylebox.inline_code, SIDE_RIGHT, 2.0f);
						const float margin_bottom = get_stylebox_margin_or(theme_cache.stylebox.inline_code, SIDE_BOTTOM, 1.0f);
						const Rect2 code_rect = Rect2(p_text_rect.position.x + selected_range.x - margin_left, y - margin_top, selected_range.y - selected_range.x + margin_left + margin_right, line_size.y + margin_top + margin_bottom);
						draw_stylebox_or_rect(theme_cache.stylebox.inline_code, code_rect, Color());
					}
					if (p_draw_link_underlines && span.strikethrough) {
							const float strike_y = y + line_size.y * 0.5f;
							draw_line(Vector2(p_text_rect.position.x + selected_range.x, strike_y), Vector2(p_text_rect.position.x + selected_range.y, strike_y), theme_cache.color.strikethrough, 1.0f);
						}
						if (p_draw_link_underlines && !span.link_uri.is_empty()) {
						const float underline_y = y + line_size.y - 2.0f;
						draw_line(Vector2(p_text_rect.position.x + selected_range.x, underline_y), Vector2(p_text_rect.position.x + selected_range.y, underline_y), link_color, 1.0f);
					}
				}
			}
			y += line_size.y + p_paragraph->get_line_spacing();
		}
	}
}

void MarkdownLabelCanvas::draw_paragraph_text(const Ref<TextParagraph>& p_paragraph, const Rect2& p_text_rect, const std::vector<MarkdownInlineSpan>& p_spans, int64_t p_global_start, const Color& p_default_color) {
	if (p_paragraph.is_null()) {
		return;
	}

	TextServerManager* text_server_manager = TextServerManager::get_singleton();
	if (text_server_manager == nullptr) {
		return;
	}
	const Ref<TextServer> text_server = text_server_manager->get_primary_interface();
	if (text_server.is_null()) {
		return;
	}

	const RID canvas = get_canvas_item();
	const Color& code_color = theme_cache.color.code_text;
	const Color& link_color = theme_cache.color.link;

	float y = p_text_rect.position.y;
	for (int32_t line_index = 0; line_index < p_paragraph->get_line_count(); line_index++) {
		const RID line_rid = p_paragraph->get_line_rid(line_index);
		const Vector2 line_position = Vector2(p_text_rect.position.x, y + p_paragraph->get_line_ascent(line_index));
		const Vector2i range = p_paragraph->get_line_range(line_index);
		if (p_spans.empty()) {
			text_server->shaped_text_draw(line_rid, canvas, line_position, -1, -1, p_default_color);
		}
		else {
			for (const MarkdownInlineSpan& span : p_spans) {
				Color span_color = span.custom_color ? span.color : p_default_color;
				if (!span.link_uri.is_empty()) {
					span_color = link_color;
				}
				else if (span.highlight) {
					span_color = theme_cache.color.highlight_font;
				}
				else if (span.code) {
					span_color = code_color;
				}
				const int64_t local_from = std::max<int64_t>(range.x, span.start);
				const int64_t local_to = std::min<int64_t>(range.y, span.end);
				if (local_from >= local_to) {
					continue;
				}
				const PackedVector2Array selected_ranges = text_server->shaped_text_get_selection(line_rid, local_from, local_to);
				for (int32_t range_index = 0; range_index < selected_ranges.size(); range_index++) {
					const Vector2 selected_range = selected_ranges[range_index];
					text_server->shaped_text_draw(line_rid, canvas, line_position, selected_range.x, selected_range.y, span_color);
				}
			}
		}
		y += p_paragraph->get_line_size(line_index).y + p_paragraph->get_line_spacing();
	}
}

void MarkdownLabelCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
		mark_layout_dirty();
	}

	if (p_what == NOTIFICATION_FOCUS_EXIT) {
		if (label != nullptr && label->is_deselect_on_focus_loss_enabled()) {
			clear_selection();
		}
	}

	if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		if (!dragging_selection || !selection_dragged || label == nullptr) {
			return;
		}

		VScrollBar* v_scroll = label->get_v_scroll_bar();
		if (v_scroll == nullptr) {
			return;
		}

		const Size2 label_size = label->get_size();
		const Vector2 label_mouse = label->get_local_mouse_position();

		const float scroll_margin = 24.0f;
		float scroll_delta = 0.0f;
		const float above = scroll_margin - label_mouse.y;
		const float below = label_mouse.y - (label_size.y - scroll_margin);

		if (above > 0.0f) {
			const float base_speed = 80.0f;
			scroll_delta = -base_speed * (above / scroll_margin) * static_cast<float>(get_process_delta_time());
		}
		else if (below > 0.0f) {
			const float base_speed = 80.0f;
			scroll_delta = base_speed * (below / scroll_margin) * static_cast<float>(get_process_delta_time());
		}

		if (scroll_delta != 0.0f) {
			const float max_scroll = v_scroll->get_max() - v_scroll->get_page();
			const float new_value = std::max<float>(0.0f, std::min<float>(max_scroll, v_scroll->get_value() + scroll_delta));
			v_scroll->set_value(new_value);

			const Vector2 canvas_mouse = get_local_mouse_position();
			selection_caret = hit_test_document(canvas_mouse);
			queue_redraw();
		}

		return;
	}

	if (p_what != NOTIFICATION_DRAW) {
		return;
	}

	rebuild_layout();

	for (const MarkdownCanvasItem& item : items) {
		if (item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			draw_stylebox_or_rect(item.panel_stylebox, item.rect, item.background_color);

			const float quote_indent = static_cast<float>(std::max<int32_t>(12, get_theme_constant_or(label, "MarkdownLabel", "blockquote_indent", 25)));
			const float panel_left = get_stylebox_margin_or(item.panel_stylebox, SIDE_LEFT, 28.0f);
			const float panel_right = get_stylebox_margin_or(item.panel_stylebox, SIDE_RIGHT, 8.0f);
			const float nested_top = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_TOP, 0.0f);
			const float nested_bottom = get_stylebox_margin_or(theme_cache.stylebox.blockquote_nested, SIDE_BOTTOM, 0.0f);

			for (int32_t depth = 2; depth <= item.blockquote_depth; depth++) {
				int32_t run_start = -1;
				float run_top = 0.0f;
				float run_bottom = 0.0f;

				for (int32_t line_index = 0; line_index <= static_cast<int32_t>(item.quote_lines.size()); line_index++) {
					const bool line_in_run = line_index < static_cast<int32_t>(item.quote_lines.size()) && item.quote_lines[line_index].depth >= depth;
					if (line_in_run && run_start < 0) {
						run_start = line_index;
						run_top = item.quote_lines[line_index].text_rect.position.y - nested_top;
						run_bottom = item.quote_lines[line_index].text_rect.get_end().y + nested_bottom;
					}
					else if (line_in_run) {
						run_bottom = item.quote_lines[line_index].text_rect.get_end().y + nested_bottom;
					}
					else if (run_start >= 0) {
						const float nested_rect_left = item.rect.position.x + panel_left + static_cast<float>(depth - 2) * quote_indent;
						const float nested_rect_width = std::max<float>(1.0f, item.rect.get_end().x - nested_rect_left - panel_right);
						const Rect2 nested_rect = Rect2(nested_rect_left, run_top, nested_rect_width, std::max<float>(1.0f, run_bottom - run_top));
						draw_stylebox_or_rect(theme_cache.stylebox.blockquote_nested, nested_rect, Color());
						run_start = -1;
					}
				}
			}

			for (const MarkdownBlockquoteLine& line : item.quote_lines) {
				draw_span_decorations(line.paragraph, line.text_rect, line.spans, line.global_start, true, false);
				if (search_match_start >= 0 && search_match_start < line.global_end && search_match_end > line.global_start) {
					draw_range_background_for_paragraph(line.paragraph, line.text_rect, line.global_start, line.global_end, search_match_start, search_match_end, theme_cache.stylebox.search_result);
				}
				draw_selection_for_paragraph(line.paragraph, line.text_rect, line.global_start, line.global_end);
				draw_paragraph_text(line.paragraph, line.text_rect, line.spans, line.global_start, item.text_color);
				draw_paragraph_images(line.paragraph, line.text_rect, line.image_map);
				draw_span_decorations(line.paragraph, line.text_rect, line.spans, line.global_start, false, true);
			}
			continue;
		}

		if (item.type == MARKDOWN_BLOCK_TABLE) {
			draw_stylebox_or_rect(item.panel_stylebox, item.rect, Color());
			for (const MarkdownTableCell& cell : item.cells) {
				draw_stylebox_or_rect(cell.stylebox, cell.rect, Color(0, 0, 0, 0));
				draw_span_decorations(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, true, false);
				if (search_match_start >= 0 && search_match_start < cell.global_end && search_match_end > cell.global_start) {
					draw_range_background_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end, search_match_start, search_match_end, theme_cache.stylebox.search_result);
				}
				draw_selection_for_paragraph(cell.paragraph, cell.text_rect, cell.global_start, cell.global_end);
				draw_paragraph_text(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, cell.text_color);
				draw_paragraph_images(cell.paragraph, cell.text_rect, cell.image_map);
				draw_span_decorations(cell.paragraph, cell.text_rect, cell.spans, cell.global_start, false, true);
			}
		}
		else if (item.framed) {
			draw_stylebox_or_rect(item.panel_stylebox, item.rect, item.background_color);
		}

		if (item.type != MARKDOWN_BLOCK_TABLE && item.type != MARKDOWN_BLOCK_THEMATIC_BREAK) {
			draw_span_decorations(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, true, false);
			draw_search_highlight_for_item(item);
			draw_selection_for_item(item);
		}

		const RID canvas_item_rid = get_canvas_item();
		for (const MarkdownListMarker& marker : item.list_markers) {
			if (marker.icon.is_valid()) {
				const float icon_size = static_cast<float>(marker.font_size);
				const float icon_y = marker.position.y - icon_size * 0.85f;
		marker.icon->draw_rect(canvas_item_rid, Rect2(marker.position.x, icon_y, icon_size, icon_size), false);
			}
			if (marker.font.is_valid()) {
				marker.font->draw_string(canvas_item_rid, marker.position, marker.text, HORIZONTAL_ALIGNMENT_LEFT, -1, marker.font_size, marker.color);
			}
		}

		if (item.type != MARKDOWN_BLOCK_THEMATIC_BREAK) {
			draw_paragraph_text(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, item.text_color);
			draw_paragraph_images(item.paragraph, item.text_rect, item.image_map);
			draw_span_decorations(item.paragraph, item.text_rect, item.spans, item.paragraph_global_start, false, true);
		}
	}
}

int64_t MarkdownLabelCanvas::hit_test_document(const Vector2& p_position) const {
	if (items.empty()) {
		return 0;
	}

	for (const MarkdownCanvasItem& item : items) {
		if (p_position.y < item.rect.position.y) {
			return item.global_start;
		}
		if (p_position.y <= item.rect.get_end().y) {
			if (item.type == MARKDOWN_BLOCK_TABLE) {
				for (const MarkdownTableCell& cell : item.cells) {
					if (cell.rect.has_point(p_position)) {
						const Vector2 local_position = p_position - cell.text_rect.position;
						const int32_t local_character = std::min<int32_t>(std::max<int32_t>(cell.paragraph->hit_test(local_position), 0), static_cast<int32_t>(cell.global_end - cell.global_start));
						return cell.global_start + local_character;
					}
				}
				return item.cells.empty() ? item.global_start : item.cells[0].global_start;
			}
			if (item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
				for (const MarkdownBlockquoteLine& line : item.quote_lines) {
					if (p_position.y <= line.text_rect.get_end().y) {
						const Vector2 local_position = p_position - line.text_rect.position;
						const int32_t local_character = std::min<int32_t>(std::max<int32_t>(line.paragraph->hit_test(local_position), 0), static_cast<int32_t>(line.global_end - line.global_start));
						return line.global_start + local_character;
					}
				}
				return p_position.x < item.rect.get_center().x ? item.global_start : item.global_end;
			}
			if (!item.paragraph.is_valid()) {
				return p_position.x < item.rect.get_center().x ? item.global_start : item.global_end;
			}
			const Vector2 local_position = p_position - item.text_rect.position;
			const int32_t local_character = std::min<int32_t>(std::max<int32_t>(item.paragraph->hit_test(local_position), 0), static_cast<int32_t>(item.paragraph_global_end - item.paragraph_global_start));
			return item.paragraph_global_start + local_character;
		}
	}

	return plain_text.length();
}

String MarkdownLabelCanvas::link_uri_at_position(const Vector2& p_position) const {
	return link_uri_at_character(hit_test_document(p_position));
}

int64_t MarkdownLabelCanvas::line_column_to_offset(int32_t p_line, int32_t p_column) const {
	const int32_t target_line = std::max<int32_t>(0, p_line);
	const int32_t target_column = std::max<int32_t>(0, p_column);
	int32_t current_line = 0;
	int32_t current_column = 0;

	for (int64_t index = 0; index < plain_text.length(); index++) {
		if (current_line == target_line && current_column >= target_column) {
			return index;
		}

		if (plain_text[index] == '\n') {
			if (current_line == target_line) {
				return index;
			}
			current_line++;
			current_column = 0;
		}
		else {
			current_column++;
		}
	}

	return plain_text.length();
}

Vector2i MarkdownLabelCanvas::offset_to_line_column(int64_t p_offset) const {
	const int64_t target_offset = std::max<int64_t>(0, std::min<int64_t>(p_offset, plain_text.length()));
	int32_t line = 0;
	int32_t column = 0;

	for (int64_t index = 0; index < target_offset; index++) {
		if (plain_text[index] == '\n') {
			line++;
			column = 0;
		}
		else {
			column++;
		}
	}

	return Vector2i(line, column);
}

String MarkdownLabelCanvas::link_uri_at_character(int64_t p_character) const {
	for (const MarkdownCanvasItem& item : items) {
		if (p_character < item.global_start || p_character > item.global_end) {
			continue;
		}
		if (item.type == MARKDOWN_BLOCK_TABLE) {
			for (const MarkdownTableCell& cell : item.cells) {
				if (p_character >= cell.global_start && p_character <= cell.global_end) {
					const int64_t local_character = p_character - cell.global_start;
					for (const MarkdownInlineSpan& span : cell.spans) {
						if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.link_uri;
						}
					}
					return String();
				}
			}
			continue;
		}
		if (item.type == MARKDOWN_BLOCK_BLOCKQUOTE) {
			for (const MarkdownBlockquoteLine& line : item.quote_lines) {
				if (p_character >= line.global_start && p_character <= line.global_end) {
					const int64_t local_character = p_character - line.global_start;
					for (const MarkdownInlineSpan& span : line.spans) {
						if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
							return span.link_uri;
						}
					}
					return String();
				}
			}
			continue;
		}
		if (p_character >= item.paragraph_global_start && p_character <= item.paragraph_global_end) {
			const int64_t local_character = p_character - item.paragraph_global_start;
			for (const MarkdownInlineSpan& span : item.spans) {
				if (!span.link_uri.is_empty() && local_character >= span.start && local_character <= span.end) {
					return span.link_uri;
				}
			}
		}
	}

	return String();
}

String MarkdownLabelCanvas::image_tooltip_at_position(const Vector2& p_position) const {
	const int64_t ch = hit_test_document(p_position);
	for (const MarkdownCanvasItem& item : items) {
		if (ch >= item.global_start && ch <= item.global_end) {
			if (ch >= item.paragraph_global_start && ch <= item.paragraph_global_end) {
				const int64_t local_ch = ch - item.paragraph_global_start;
				for (const MarkdownInlineSpan& span : item.spans) {
					if (!span.image_uri.is_empty() && local_ch >= span.start && local_ch <= span.end) {
						if (!span.image_title.is_empty()) return span.image_title;
						if (!span.link_uri.is_empty()) return span.link_uri;
						return span.image_uri;
					}
				}
			}
		}
	}
	return String();
}

void MarkdownLabelCanvas::_gui_input(const Ref<InputEvent>& p_event) {
	if (label == nullptr) {
		return;
	}

	const Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid()) {
		if (mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN) {
			if (mouse_button->is_ctrl_pressed()) {
				int32_t delta = mouse_button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 1 : -1;
				label->set_extra_font_size(std::max<int32_t>(0, label->get_extra_font_size() + delta));
				accept_event();
				return;
			}
			return;
		}

		if (mouse_button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (mouse_button->is_pressed()) {
				press_position = mouse_button->get_position();
				selection_anchor = hit_test_document(press_position);
				selection_caret = selection_anchor;
				queue_redraw();
				dragging_selection = true;
				selection_dragged = false;
				if (!has_focus()) {
					grab_focus();
				}
				accept_event();
				return;
			}

			if (dragging_selection) {
				dragging_selection = false;
				set_process_internal(false);
				if (!selection_dragged) {
					const Vector2 up_position = mouse_button->get_position();
					const String uri = link_uri_at_position(up_position);
					if (!uri.is_empty()) {
						label->activate_link_uri(uri);
					}
				}
				accept_event();
				return;
			}
			return;
		}
		return;
	}

	const Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid()) {
		if (dragging_selection) {
			const Vector2 current_position = mouse_motion->get_position();
			if (current_position.distance_to(press_position) > 4.0f) {
				selection_dragged = true;
			}
			selection_caret = hit_test_document(current_position);
			set_process_internal(true);
			queue_redraw();
			accept_event();
			return;
		}

		const String uri = link_uri_at_position(mouse_motion->get_position());
		const String img_tip = image_tooltip_at_position(mouse_motion->get_position());
		set_default_cursor_shape((!uri.is_empty() || !img_tip.is_empty()) ? CURSOR_POINTING_HAND : CURSOR_ARROW);
		if (!uri.is_empty()) {
			set_tooltip_text(uri);
		} else if (!img_tip.is_empty()) {
			set_tooltip_text(img_tip);
		} else {
			set_tooltip_text(String());
		}
		return;
	}

	const Ref<InputEventKey> key_event = p_event;
	if (key_event.is_valid() && key_event->is_pressed()) {
		if (key_event->is_ctrl_pressed() && key_event->get_keycode() == KEY_A) {
			select_all();
			accept_event();
			return;
		}
		if (key_event->is_ctrl_pressed() && key_event->get_keycode() == KEY_C) {
			const String selected = get_selected_text();
			if (!selected.is_empty()) {
				DisplayServer::get_singleton()->clipboard_set(selected);
			}
			accept_event();
			return;
		}
		return;
	}
}

// MarkdownLabel
void MarkdownLabel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_markdown_text", "text"), &MarkdownLabel::set_markdown_text);
	ClassDB::bind_method(D_METHOD("get_markdown_text"), &MarkdownLabel::get_markdown_text);

	ClassDB::bind_method(D_METHOD("append_text", "delta"), &MarkdownLabel::append_text);
	ClassDB::bind_method(D_METHOD("finish_stream"), &MarkdownLabel::finish_stream);
	ClassDB::bind_method(D_METHOD("clear"), &MarkdownLabel::clear);

	ClassDB::bind_method(D_METHOD("set_streaming_enabled", "enabled"), &MarkdownLabel::set_streaming_enabled);
	ClassDB::bind_method(D_METHOD("is_streaming_enabled"), &MarkdownLabel::is_streaming_enabled);

	ClassDB::bind_method(D_METHOD("set_max_unstable_lines", "lines"), &MarkdownLabel::set_max_unstable_lines);
	ClassDB::bind_method(D_METHOD("get_max_unstable_lines"), &MarkdownLabel::get_max_unstable_lines);

	ClassDB::bind_method(D_METHOD("set_auto_scroll", "enabled"), &MarkdownLabel::set_auto_scroll);
	ClassDB::bind_method(D_METHOD("is_auto_scroll"), &MarkdownLabel::is_auto_scroll);

	ClassDB::bind_method(D_METHOD("set_content_margin", "margin"), &MarkdownLabel::set_content_margin);
	ClassDB::bind_method(D_METHOD("get_content_margin"), &MarkdownLabel::get_content_margin);

	ClassDB::bind_method(D_METHOD("set_extra_font_size", "size"), &MarkdownLabel::set_extra_font_size);
	ClassDB::bind_method(D_METHOD("get_extra_font_size"), &MarkdownLabel::get_extra_font_size);

	ClassDB::bind_method(D_METHOD("set_selection_enabled", "enabled"), &MarkdownLabel::set_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_selection_enabled"), &MarkdownLabel::is_selection_enabled);

	ClassDB::bind_method(D_METHOD("set_deselect_on_focus_loss_enabled", "enabled"), &MarkdownLabel::set_deselect_on_focus_loss_enabled);
	ClassDB::bind_method(D_METHOD("is_deselect_on_focus_loss_enabled"), &MarkdownLabel::is_deselect_on_focus_loss_enabled);

	ClassDB::bind_method(D_METHOD("set_drag_and_drop_selection_enabled", "enabled"), &MarkdownLabel::set_drag_and_drop_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_drag_and_drop_selection_enabled"), &MarkdownLabel::is_drag_and_drop_selection_enabled);

	ClassDB::bind_method(D_METHOD("set_open_external_links", "enabled"), &MarkdownLabel::set_open_external_links);
	ClassDB::bind_method(D_METHOD("get_open_external_links"), &MarkdownLabel::get_open_external_links);

	ClassDB::bind_method(D_METHOD("select_all"), &MarkdownLabel::select_all);
	ClassDB::bind_method(D_METHOD("clear_selection"), &MarkdownLabel::clear_selection);
	ClassDB::bind_method(D_METHOD("get_selected_text"), &MarkdownLabel::get_selected_text);

	ClassDB::bind_method(D_METHOD("search", "text", "flags", "from_line", "from_column"), &MarkdownLabel::search);

	ClassDB::bind_method(D_METHOD("get_link_uri_at_point", "position"), &MarkdownLabel::get_link_uri_at_point);
	ClassDB::bind_method(D_METHOD("activate_link_uri", "uri"), &MarkdownLabel::activate_link_uri);

	ClassDB::bind_method(D_METHOD("has_anchor", "anchor"), &MarkdownLabel::has_anchor);
	ClassDB::bind_method(D_METHOD("get_anchors"), &MarkdownLabel::get_anchors);
	ClassDB::bind_method(D_METHOD("scroll_to_anchor", "anchor", "align"), &MarkdownLabel::scroll_to_anchor, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("scroll_to_search_result", "align"), &MarkdownLabel::scroll_to_search_result, DEFVAL(0.0f));

	ClassDB::bind_method(D_METHOD("load_markdown_file", "path"), &MarkdownLabel::load_markdown_file);
	ClassDB::bind_method(D_METHOD("refresh"), &MarkdownLabel::refresh);

	ClassDB::bind_method(D_METHOD("get_rendered_block_count"), &MarkdownLabel::get_rendered_block_count);
	ClassDB::bind_method(D_METHOD("get_content_height"), &MarkdownLabel::get_content_height);

	ClassDB::bind_method(D_METHOD("get_h_scroll_bar"), &MarkdownLabel::get_h_scroll_bar);
	ClassDB::bind_method(D_METHOD("get_v_scroll_bar"), &MarkdownLabel::get_v_scroll_bar);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_markdown_text", "get_markdown_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "content_margin", PROPERTY_HINT_RANGE, "0,128"), "set_content_margin", "get_content_margin");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "extra_font_size", PROPERTY_HINT_RANGE, "0,16"), "set_extra_font_size", "get_extra_font_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "open_external_links"), "set_open_external_links", "get_open_external_links");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selection_enabled"), "set_selection_enabled", "is_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "deselect_on_focus_loss_enabled"), "set_deselect_on_focus_loss_enabled", "is_deselect_on_focus_loss_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_and_drop_selection_enabled"), "set_drag_and_drop_selection_enabled", "is_drag_and_drop_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "streaming_enabled"), "set_streaming_enabled", "is_streaming_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_unstable_lines", PROPERTY_HINT_RANGE, "1,256"), "set_max_unstable_lines", "get_max_unstable_lines");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_scroll"), "set_auto_scroll", "is_auto_scroll");

	ADD_SIGNAL(MethodInfo("link_pressed", PropertyInfo(Variant::STRING, "uri")));
	ADD_SIGNAL(MethodInfo("text_changed"));
	ADD_SIGNAL(MethodInfo("render_warning", PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "line")));
	ADD_SIGNAL(MethodInfo("extra_font_size_changed", PropertyInfo(Variant::INT, "font_size")));
}

void MarkdownLabel::ensure_controls() {
	if (scroll_container != nullptr) {
		return;
	}

	set_theme_type_variation("MarkdownLabel");

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_name("_MarkdownScroll");
	scroll_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(scroll_container, false, Node::INTERNAL_MODE_BACK);

	margin_container = memnew(MarginContainer);
	margin_container->set_name("_MarkdownMargin");
	margin_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	margin_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	margin_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->add_child(margin_container, false, Node::INTERNAL_MODE_BACK);

	canvas = memnew(MarkdownLabelCanvas);
	canvas->set_name("_MarkdownCanvas");
	canvas->set_label(this);
	canvas->set_focus_mode(Control::FOCUS_CLICK);
	canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas->set_custom_minimum_size(Vector2(0.0, 1.0));
	canvas->set_streaming_enabled(streaming_enabled);
	canvas->set_max_unstable_lines(max_unstable_lines);
	canvas->set_markdown_text(raw_text);
	margin_container->add_child(canvas, false, Node::INTERNAL_MODE_BACK);

	apply_theme_settings();
}

void MarkdownLabel::apply_theme_settings() {
	if (margin_container == nullptr) {
		return;
	}

	margin_container->add_theme_constant_override("margin_left", content_margin);
	margin_container->add_theme_constant_override("margin_right", content_margin);
	margin_container->add_theme_constant_override("margin_top", content_margin);
}

void MarkdownLabel::_notification(int p_what) {
	if (p_what == NOTIFICATION_POSTINITIALIZE || p_what == NOTIFICATION_POST_ENTER_TREE) {
		ensure_controls();
	}

	if (p_what == NOTIFICATION_READY) {
		refresh();
	}

	if (p_what == NOTIFICATION_THEME_CHANGED) {
		apply_theme_settings();
		if (canvas != nullptr) {
			refresh();
		}
	}
}

void MarkdownLabel::set_markdown_text(const String& p_text) {
	raw_text = p_text;
	if (canvas != nullptr) {
		canvas->set_markdown_text(p_text);
	}
	emit_signal("text_changed");
}

String MarkdownLabel::get_markdown_text() const {
	return raw_text;
}

void MarkdownLabel::append_text(const String& p_delta) {
	ensure_controls();
	raw_text += p_delta;
	canvas->set_streaming_enabled(streaming_enabled);
	canvas->set_max_unstable_lines(max_unstable_lines);
	canvas->append_text_incremental(p_delta);
	if (auto_scroll && scroll_container != nullptr) {
		VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
		if (v_scroll != nullptr) {
			scroll_container->call_deferred("set_deferred", "scroll_vertical", static_cast<int32_t>(v_scroll->get_max()));
		}
	}
	emit_signal("text_changed");
}

void MarkdownLabel::finish_stream() {
	if (canvas != nullptr) {
		canvas->finish_stream();
	}
	if (auto_scroll && scroll_container != nullptr) {
		VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
		if (v_scroll != nullptr) {
			scroll_container->call_deferred("set_deferred", "scroll_vertical", static_cast<int32_t>(v_scroll->get_max()));
		}
	}
}

void MarkdownLabel::clear() {
	raw_text = String();
	if (canvas != nullptr) {
		canvas->clear_document();
	}
	emit_signal("text_changed");
}

void MarkdownLabel::set_streaming_enabled(bool p_enabled) {
	streaming_enabled = p_enabled;
	if (canvas != nullptr) {
		canvas->set_streaming_enabled(p_enabled);
	}
}

bool MarkdownLabel::is_streaming_enabled() const {
	return streaming_enabled;
}

void MarkdownLabel::set_max_unstable_lines(int32_t p_lines) {
	max_unstable_lines = p_lines;
	if (canvas != nullptr) {
		canvas->set_max_unstable_lines(p_lines);
	}
}

int32_t MarkdownLabel::get_max_unstable_lines() const {
	return max_unstable_lines;
}

void MarkdownLabel::set_auto_scroll(bool p_enabled) {
	auto_scroll = p_enabled;
}

bool MarkdownLabel::is_auto_scroll() const {
	return auto_scroll;
}

void MarkdownLabel::set_content_margin(int32_t p_margin) {
	content_margin = std::max<int32_t>(0, std::min<int32_t>(p_margin, 128));
	apply_theme_settings();
}

int32_t MarkdownLabel::get_content_margin() const {
	return content_margin;
}

void MarkdownLabel::set_extra_font_size(int32_t p_extra) {
	extra_font_size = std::max<int32_t>(0, std::min<int32_t>(p_extra, 16));
	emit_signal("extra_font_size_changed", extra_font_size);
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
}

int32_t MarkdownLabel::get_extra_font_size() const {
	return extra_font_size;
}

void MarkdownLabel::set_selection_enabled(bool p_enabled) {
	selection_enabled = p_enabled;
}

bool MarkdownLabel::is_selection_enabled() const {
	return selection_enabled;
}

void MarkdownLabel::set_deselect_on_focus_loss_enabled(bool p_enabled) {
	deselect_on_focus_loss_enabled = p_enabled;
}

bool MarkdownLabel::is_deselect_on_focus_loss_enabled() const {
	return deselect_on_focus_loss_enabled;
}

void MarkdownLabel::set_drag_and_drop_selection_enabled(bool p_enabled) {
	drag_and_drop_selection_enabled = p_enabled;
}

bool MarkdownLabel::is_drag_and_drop_selection_enabled() const {
	return drag_and_drop_selection_enabled;
}

void MarkdownLabel::set_open_external_links(bool p_enabled) {
	open_external_links = p_enabled;
}

bool MarkdownLabel::get_open_external_links() const {
	return open_external_links;
}

void MarkdownLabel::select_all() {
	ensure_controls();
	canvas->select_all();
}

void MarkdownLabel::clear_selection() {
	if (canvas != nullptr) {
		canvas->clear_selection();
	}
}

String MarkdownLabel::get_selected_text() const {
	if (canvas == nullptr) {
		return String();
	}
	return canvas->get_selected_text();
}

Vector2i MarkdownLabel::search(const String& p_text, int32_t p_flags, int32_t p_from_line, int32_t p_from_column) {
	ensure_controls();
	return canvas->search(p_text, p_flags, p_from_line, p_from_column);
}

String MarkdownLabel::get_link_uri_at_point(const Vector2& p_position) {
	ensure_controls();
	return canvas->get_link_uri_at_point(p_position);
}

void MarkdownLabel::activate_link_uri(const String& p_uri) {
	emit_signal("link_pressed", p_uri);
	if (open_external_links && (p_uri.begins_with("http://") || p_uri.begins_with("https://"))) {
		OS::get_singleton()->shell_open(p_uri);
	}
}

bool MarkdownLabel::has_anchor(const String& p_anchor) const {
	if (canvas == nullptr) {
		return false;
	}
	return canvas->has_anchor(p_anchor);
}

PackedStringArray MarkdownLabel::get_anchors() const {
	if (canvas == nullptr) {
		return PackedStringArray();
	}
	return canvas->get_anchors();
}

bool MarkdownLabel::scroll_to_anchor(const String& p_anchor, float p_align) {
	ensure_controls();
	if (scroll_container == nullptr || canvas == nullptr) {
		return false;
	}

	const float offset = canvas->get_anchor_offset(p_anchor);
	if (offset < 0.0f) {
		return false;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr) {
		return false;
	}

	const float viewport_height = scroll_container->get_size().y;
	const float target = std::max<float>(0.0f, offset - viewport_height * p_align);
	v_scroll->set_value(std::min<float>(target, v_scroll->get_max() - v_scroll->get_page()));
	return true;
}

bool MarkdownLabel::scroll_to_search_result(float p_align) {
	ensure_controls();
	if (scroll_container == nullptr || canvas == nullptr) {
		return false;
	}

	const float y = canvas->get_search_result_y();
	if (y < 0.0f) {
		return false;
	}

	VScrollBar* v_scroll = scroll_container->get_v_scroll_bar();
	if (v_scroll == nullptr) {
		return false;
	}

	const float viewport_height = scroll_container->get_size().y;
	const float target = std::max<float>(0.0f, y - viewport_height * p_align);
	v_scroll->set_value(std::min<float>(target, v_scroll->get_max() - v_scroll->get_page()));
	return true;
}

Error MarkdownLabel::load_markdown_file(const String& p_path) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_null()) {
		return ERR_FILE_CANT_OPEN;
	}

	const String content = file->get_as_text();
	file->close();

	set_markdown_text(content);
	return OK;
}

void MarkdownLabel::refresh() {
	if (canvas != nullptr) {
		canvas->mark_layout_dirty();
	}
}

int32_t MarkdownLabel::get_rendered_block_count() const {
	if (canvas == nullptr) {
		return 0;
	}
	return canvas->get_rendered_block_count();
}

float MarkdownLabel::get_content_height() const {
	if (canvas == nullptr) {
		return 1.0f;
	}
	return canvas->get_content_height();
}

HScrollBar* MarkdownLabel::get_h_scroll_bar() const {
	if (scroll_container == nullptr) {
		return nullptr;
	}
	return scroll_container->get_h_scroll_bar();
}

VScrollBar* MarkdownLabel::get_v_scroll_bar() const {
	if (scroll_container == nullptr) {
		return nullptr;
	}
	return scroll_container->get_v_scroll_bar();
}
