#include "hurdygurdy.hpp"

HgString HgString::create(HgArena& arena, usize capacity) {
    HgString str;
    str.chars = arena.alloc<char>(capacity);
    str.capacity = capacity;
    str.length = 0;
    return str;
}

HgString HgString::create(HgArena& arena, HgStringView init) {
    HgString str;
    str.chars = arena.alloc<char>(init.length);
    str.capacity = init.length;
    str.length = init.length;
    std::memcpy(str.chars, init.chars, init.length);
    return str;
}

void HgString::reserve(HgArena& arena, usize new_capacity) {
    chars = arena.realloc(chars, capacity, new_capacity);
    capacity = new_capacity;
}

void HgString::grow(HgArena& arena, f32 factor) {
    hg_assert(factor > 1.0f);
    hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
    reserve(arena, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
}

HgString& HgString::insert(HgArena& arena, usize index, char c) {
    hg_assert(index <= length);

    usize new_length = length + 1;
    while (capacity < new_length) {
        grow(arena);
    }

    if (index != length)
        std::memmove(&chars[index + 1], &chars[index], length - index);
    chars[index] = c;
    length = new_length;

    return *this;
}

HgString& HgString::insert(HgArena& arena, usize index, HgStringView str) {
    hg_assert(index <= length);

    usize new_length = length + str.length;
    while (capacity < new_length) {
        grow(arena);
    }

    if (index != length)
        std::memmove(&chars[index + str.length], &chars[index], length - index);
    std::memcpy(&chars[index], str.chars, str.length);
    length = new_length;

    return *this;
}

bool hg_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

bool hg_is_numeral_base10(char c) {
    return c >= '0' && c <= '9';
}

bool hg_is_integer_base10(HgStringView str) {
    if (str.length == 0)
        return false;

    usize head = 0;
    if (!hg_is_numeral_base10(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    ++head;
    while (head < str.length) {
        if (!hg_is_numeral_base10(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool hg_is_float_base10(HgStringView str) {
    if (str.length == 0)
        return false;

    bool has_decimal = false;
    bool has_exponent = false;

    usize head = 0;

    if (!hg_is_numeral_base10(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '.')
        has_decimal = true;

    ++head;
    while (head < str.length) {
        if (hg_is_numeral_base10(str[head])) {
            ++head;
            continue;
        }

        if (str[head] == '.' && !has_decimal) {
            has_decimal = true;
            ++head;
            continue;
        }

        if (str[head] == 'e' && !has_exponent) {
            has_exponent = true;
            ++head;
            if (hg_is_numeral_base10(str[head]) || str[head] == '+' || str[head] == '-') {
                ++head;
                continue;
            }
            return false;
        }

        if (str[head] == 'f' && head == str.length - 1)
            break;

        return false;
    }

    return has_decimal || has_exponent;
}

i64 hg_str_to_int_base10(HgStringView str) {
    hg_assert(hg_is_integer_base10(str));

    i64 power = 1;
    i64 ret = 0;

    usize head = str.length - 1;
    while (head > 0) {
        ret += (i64)(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+') {
        if (str[head] == '-')
            ret *= -1;
        else
            ret += (i64)(str[head] - '0') * power;
    }

    return ret;
}

f64 hg_str_to_float_base10(HgStringView str) {
    hg_assert(hg_is_float_base10(str));

    f64 ret = 0.0;
    usize head = 0;

    bool is_negative = str[head] == '-';
    if (is_negative || str[head] == '+')
        ++head;

    if (hg_is_numeral_base10(str[head])) {
        usize int_part_begin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e') {
            ++head;
        }
        ret += (f64)hg_str_to_int_base10({&str[int_part_begin], &str[head]});
    }

    if (head < str.length && str[head] == '.') {
        ++head;

        f64 power = 0.1;
        while (head < str.length && hg_is_numeral_base10(str[head])) {
            ret += (f64)(str[head] - '0') * power;
            power *= 0.1;
            ++head;
        }
    }

    if (head < str.length && str[head] == 'e') {
        ++head;

        bool exp_is_negative = str[head] == '-';
        if (exp_is_negative || str[head] == '+')
            ++head;

        usize exp_begin = head;
        while (head < str.length && hg_is_numeral_base10(str[head])) {
            ++head;
        }

        i64 exp = hg_str_to_int_base10({&str[exp_begin], str.chars + head});
        if (exp != 0) {
            if (exp_is_negative) {
                for (i64 i = 0; i < exp; ++i) {
                    ret *= 0.1;
                }
            } else {
                for (i64 i = 0; i < exp; ++i) {
                    ret *= 10.0;
                }
            }
        } else {
            ret = 1.0;
        }
    }

    if (is_negative)
        ret *= -1.0;

    return ret;
}

HgString hg_int_to_str_base10(HgArena& arena, i64 num) {
    hg_arena_scope(scratch, hg_get_scratch(arena));

    if (num == 0)
        return HgString::create(arena, "0");

    bool is_negative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgString reverse = reverse.create(scratch, 16);
    while (unum != 0) {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        reverse.append(scratch, '0' + (char)digit);
    }

    HgString ret = ret.create(arena, reverse.length + (is_negative ? 1 : 0));
    if (is_negative)
        ret.append(arena, '-');
    for (usize i = reverse.length - 1; i < reverse.length; --i) {
        ret.append(arena, reverse[i]);
    }
    return ret;
}

HgString hg_float_to_str_base10(HgArena& arena, f64 num, u64 decimal_count) {
    hg_arena_scope(scratch, hg_get_scratch(arena));

    if (num == 0.0)
        return HgString::create(arena, "0.0");

    HgString int_str = hg_int_to_str_base10(scratch, (i64)std::abs(num));

    HgString dec_str = HgString::create(scratch, decimal_count + 1);
    dec_str.append(scratch, '.');

    f64 dec_part = std::abs(num);
    for (usize i = 0; i < decimal_count; ++i) {
        dec_part *= 10.0;
        dec_str.append(scratch, '0' + (char)((u64)dec_part % 10));
    }

    HgString ret{};
    if (num < 0.0)
        ret.append(arena, '-');
    ret.append(arena, int_str);
    ret.append(arena, dec_str);
    return ret;
}

struct HgJsonParser {
    HgArena& arena;
    HgStringView text;
    usize head;
    usize line;

    HgJsonParser(HgArena& arena_val) : arena{arena_val} {}

    HgJson parse_next();
    HgJson parse_struct();
    HgJson parse_array();
    HgJson parse_string();
    HgJson parse_number();
    HgJson parse_boolean();
};

HgJson HgJsonParser::parse_next() {
    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head >= text.length)
        return {};

    switch (text[head]) {
        case '{':
            ++head;
            return parse_struct();
        case '[':
            ++head;
            return parse_array();
        case '\'': [[fallthrough]];
        case '"':
            ++head;
            return parse_string();
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return parse_number();
        case 't': [[fallthrough]];
        case 'f':
            return parse_boolean();
        case '}': {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", found unexpected token \"}\"\n");
            return {nullptr, error};
        }
        case ']': {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", found unexpected token \"]\"\n");
            return {nullptr, error};
        }
    }
    if (hg_is_numeral_base10(text[head])) {
        return parse_number();
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
    error->next = nullptr;

    usize begin = head;
    while (head < text.length && !hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", found unexpected token \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

    return {nullptr, error};
}

HgJson HgJsonParser::parse_struct() {
    HgJson json{};
    json.file = arena.alloc<HgJson::Node>(1);
    json.file->type = HgJson::jstruct;
    json.file->jstruct.fields = nullptr;

    HgJson::Field* last_field = nullptr;
    HgJson::Error* last_error = nullptr;

    for (;;) {
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length) {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            break;
        }
        if (text[head] == ']') {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", struct ends with \"]\" instead of \"}\"\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == '}') {
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJson value = parse_next();

        if (value.file != nullptr) {
            if (value.file->type != HgJson::field) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", struct has a literal instead of a field\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else if (value.file->field.value == nullptr) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", struct has a field named \"")
                    .append(arena, value.file->field.name)
                    .append(arena, "\" which has no value\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else {
                if (last_field == nullptr)
                    json.file->jstruct.fields = &value.file->field;
                else
                    last_field->next = &value.file->field;
                last_field = &value.file->field;
            }
        }
        if (value.errors != nullptr) {
            if (last_error == nullptr)
                json.errors = last_error = value.errors;
            else
                last_error->next = value.errors;
            last_error = value.errors;
        }
    }

    return json;
}

HgJson HgJsonParser::parse_array() {
    HgJson json{};
    json.file = arena.alloc<HgJson::Node>(1);
    json.file->type = HgJson::array;

    HgJson::Type type = HgJson::none;
    HgJson::Elem* last_elem = nullptr;
    HgJson::Error* last_error = nullptr;

    for (;;) {
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head >= text.length) {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", expected struct to terminate\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            break;
        }
        if (text[head] == '}') {
            HgJson::Error* error = arena.alloc<HgJson::Error>(1);
            error->next = nullptr;
            error->msg = HgString{}
                .append(arena, "on line ")
                .append(arena, hg_int_to_str_base10(arena, (i64)line))
                .append(arena, ", array ends with \"}\" instead of \"]\"\n");
            if (last_error == nullptr)
                json.errors = last_error = error;
            else
                last_error->next = error;
            last_error = error;
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }
        if (text[head] == ']') {
            ++head;
            while (head < text.length && hg_is_whitespace(text[head])) {
                if (text[head] == '\n')
                    ++line;
                ++head;
            }
            if (head < text.length && text[head] == ',')
                ++head;
            break;
        }

        HgJson::Elem* elem = arena.alloc<HgJson::Elem>(1);
        elem->next = nullptr;

        HgJson value = parse_next();
        elem->value = value.file;

        if (value.file != nullptr) {
            if (type == HgJson::none) {
                if (value.file->type != HgJson::field) {
                    type = value.file->type;
                } else {
                    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                    error->next = nullptr;
                    error->msg = HgString{}
                        .append(arena, "on line ")
                        .append(arena, hg_int_to_str_base10(arena, (i64)line))
                        .append(arena, ", array has a field as an element\n");
                    if (last_error == nullptr)
                        json.errors = last_error = error;
                    else
                        last_error->next = error;
                    last_error = error;
                }
            }
            if (value.file->type != type) {
                HgJson::Error* error = arena.alloc<HgJson::Error>(1);
                error->next = nullptr;
                error->msg = HgString{}
                    .append(arena, "on line ")
                    .append(arena, hg_int_to_str_base10(arena, (i64)line))
                    .append(arena, ", array has element which is not the same type as the first valid element\n");
                if (last_error == nullptr)
                    json.errors = last_error = error;
                else
                    last_error->next = error;
                last_error = error;
            } else {
                if (last_elem == nullptr)
                    json.file->array.elems = elem;
                else
                    last_elem->next = elem;
                last_elem = elem;
            }
        }
        if (value.errors != nullptr) {
            if (last_error == nullptr)
                json.errors = last_error = value.errors;
            else
                last_error->next = value.errors;
            last_error = value.errors;
        }
    }

    return json;
}

HgJson HgJsonParser::parse_string() {
    usize begin = head;
    while (head < text.length && text[head] != '"') {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    usize end = head;
    if (head < text.length) {
        ++head;
        HgString str = str.create(arena, end - begin);
        for (usize i = begin; i < end; ++i) {
            char c = text[i];
            if (c == '\\') {
                // escape sequences : TODO
            }
            str.append(arena, c);
        }

        HgJson json{};
        json.file = arena.alloc<HgJson::Node>(1);

        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ':') {
            ++head;
            json.file->type = HgJson::field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            HgJson next = parse_next();
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = HgJson::string;
            json.file->string = str;
        }
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;
        return json;
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected string to terminate\n");
    return {nullptr, error};
}

HgJson HgJsonParser::parse_number() {
    bool is_float = false;
    usize begin = head;
    while (head < text.length && (
        hg_is_numeral_base10(text[head]) ||
        text[head] == '-' ||
        text[head] == '+' ||
        text[head] == '.' ||
        text[head] == 'e'
    )) {
        if (text[head] == '.' || text[head] == 'e')
            is_float = true;
        ++head;
    }
    HgStringView num{&text[begin], &text[head]};
    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (head < text.length && text[head] == ',')
        ++head;

    if (is_float) {
        if (hg_is_float_base10(num)) {
            HgJson::Node* node = arena.alloc<HgJson::Node>(1);
            node->type = HgJson::floating;
            node->floating = hg_str_to_float_base10(num);
            return {node, nullptr};
        }
    } else {
        if (hg_is_integer_base10(num)) {
            HgJson::Node* node = arena.alloc<HgJson::Node>(1);
            node->type = HgJson::integer;
            node->integer = hg_str_to_int_base10(num);
            return {node, nullptr};
        }
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);

    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected numeral value, found \"")
        .append(arena, num)
        .append(arena, "\"\n");

    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']') {
        return {nullptr, error};
    } else {
        HgJson next = parse_next();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson HgJsonParser::parse_boolean() {
    if (head + 4 < text.length && HgStringView{&text[head], 4} == "true") {
        head += 4;
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJson::Node* node = arena.alloc<HgJson::Node>(1);
        node->type = HgJson::boolean;
        node->boolean = true;
        return {node, nullptr};
    }
    if (head + 5 < text.length && HgStringView{&text[head], 5} == "false") {
        head += 5;
        while (head < text.length && hg_is_whitespace(text[head])) {
            if (text[head] == '\n')
                ++line;
            ++head;
        }
        if (head < text.length && text[head] == ',')
            ++head;

        HgJson::Node* node = arena.alloc<HgJson::Node>(1);
        node->type = HgJson::boolean;
        node->boolean = false;
        return {node, nullptr};
    }

    HgJson::Error* error = arena.alloc<HgJson::Error>(1);

    usize begin = head;
    while (head < text.length && !hg_is_whitespace(text[head])
        && text[head] != ','
        && text[head] != '}'
        && text[head] != ']'
    ) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    error->msg = HgString{}
        .append(arena, "on line ")
        .append(arena, hg_int_to_str_base10(arena, (i64)line))
        .append(arena, ", expected boolean value, found \"")
        .append(arena, {&text[begin], &text[head]})
        .append(arena, "\"\n");

    if (text[head] == ',')
        ++head;

    while (head < text.length && hg_is_whitespace(text[head])) {
        if (text[head] == '\n')
            ++line;
        ++head;
    }
    if (text[head] == '}' || text[head] == ']') {
        return {nullptr, error};
    } else {
        HgJson next = parse_next();
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson HgJson::parse(HgArena& arena, HgStringView text) {
    HgJsonParser parser{arena};
    parser.text = text;
    parser.head = 0;
    parser.line = 1;
    return parser.parse_next();
}

