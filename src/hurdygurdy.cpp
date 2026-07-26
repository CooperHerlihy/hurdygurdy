#include "hurdygurdy.hpp"
#include "internal.hpp"

#include <ctime>

#include <condition_variable>
#include <mutex>
#include <random>

#include <emmintrin.h>

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

// static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node);
//
// static void serialJsonWriteString(StringBuilder* str, StringView string)
// {
//     str->append('"');
//     for (u32 i = 0; i < string.length; ++i)
//     {
//         switch (string[i])
//         {
//         case '\\':
//             str->append("\\\\");
//             break;
//         case '\"':
//             str->append("\\\"");
//             break;
//         case '\n':
//             str->append("\\n");
//             break;
//         case '\r':
//             str->append("\\r");
//             break;
//         case '\f':
//             str->append("\\f");
//             break;
//         case '\b':
//             str->append("\\b");
//             break;
//         default:
//             str->append(string[i]);
//             break;
//         }
//     }
//     str->append('"');
// }
//
// static void serialJsonWriteArray(StringBuilder* str, u32 indentation, SerialObject object)
// {
//     if (object.childCount > 0)
//     {
//         str->append("[\n");
//
//         SerialNode* elem = object.firstChild;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             str->append("    ");
//         }
//         serialJsonWriteNode(str, indentation + 1, elem);
//
//         elem = elem->next;
//
//         for (u32 i = 1; i < object.childCount; ++i)
//         {
//             str->append(",\n");
//             for (u32 j = 0; j < indentation + 1; ++j)
//             {
//                 str->append("    ");
//             }
//             serialJsonWriteNode(str, indentation + 1, elem);
//
//             elem = elem->next;
//         }
//
//         str->append( '\n');
//         for (u32 k = 0; k < indentation; ++k)
//         {
//             str->append("    ");
//         }
//         str->append(']');
//     }
//     else
//     {
//         str->append("[]");
//     }
// }
//
// static void serialJsonWriteObject(Arena* arena, StringBuilder* str, u32 indentation, SerialNode* node)
// {
//     if (node->count > 0)
//     {
//         stringAppend(arena, str, "{\n");
//
//         SerialNode* field = node->children;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         serialJsonWriteString(arena, str, field->name);
//         stringAppend(arena, str, " : ");
//         serialJsonWriteNode(arena, str, indentation + 1, field);
//
//         field = field->next;
//
//         for (u32 i = 1; i < node->count; ++i)
//         {
//             stringAppend(arena, str, ",\n");
//             for (u32 i = 0; i < indentation + 1; ++i)
//             {
//                 stringAppend(arena, str, "    ");
//             }
//             serialJsonWriteString(arena, str, field->name);
//             stringAppend(arena, str, " : ");
//             serialJsonWriteNode(arena, str, indentation + 1, field);
//
//             field = field->next;
//         }
//
//         stringAppendC(arena, str, '\n');
//         for (u32 i = 0; i < indentation; ++i)
//         {
//             stringAppend(arena, str, "    ");
//         }
//         stringAppendC(arena, str, '}');
//     }
//     else
//     {
//         stringAppend(arena, str, "{}");
//     }
// }
//
// static void serialJsonWriteNode(StringBuilder* str, u32 indentation, SerialNode* node)
// {
//     node->data.match(
//         [&](SerialObject& obj) { serialJsonWriteArray(str, indentation, obj); },
//         [&](StringView& strData) { serialJsonWriteString(str, strData); },
//         [&](i64& i) { str->append(integerToString(getScratch(), i)); },
//         [&](f64& f) { str->append(floatToString(getScratch(), f, 6)); },
//         [&](bool& b) { str->append(b ? "true" : "false"); }
//     );
// }
//
// StringView jsonWriteSerial(Arena* arena, Serializer* serial)
// {
//     StringBuilder str{arena};
//     serialJsonWriteNode(&str, 0, serial->current);
//     str.append('\n');
//     return str;
// }

// Serializer jsonReadSerial(Arena* arena, StringView json)
// {
// }

// struct JsonParseState {
//     StringView text = {};
//     u64 head = 0;
//     u64 line = 0;
// };
//
// static Json jsonParseNext(Arena* arena, JsonParseState* state);
// static Json jsonParseStruct(Arena* arena, JsonParseState* state);
// static Json jsonParseArray(Arena* arena, JsonParseState* state);
// static Json jsonParseString(Arena* arena, JsonParseState* state);
// static Json jsonParseNumber(Arena* arena, JsonParseState* state);
// static Json jsonParseBoolean(Arena* arena, JsonParseState* state);
//
// static Json jsonParseNext(Arena* arena, JsonParseState* state)
// {
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->head >= state->text.length)
//         return {};
//
//     switch (state->text[state->head])
//     {
//         case '{':
//             ++state->head;
//             return jsonParseStruct(arena, state);
//         case '[':
//             ++state->head;
//             return jsonParseArray(arena, state);
//         case '\'': [[fallthrough]];
//         case '"':
//             ++state->head;
//             return jsonParseString(arena, state);
//         case '.': [[fallthrough]];
//         case '+': [[fallthrough]];
//         case '-':
//             return jsonParseNumber(arena, state);
//         case 't': [[fallthrough]];
//         case 'f':
//             return jsonParseBoolean(arena, state);
//         case '}': {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(arena, static_cast<i64>(state->line)));
//             msg.append(", found unexpected token \"}\"\n");
//             error->msg = msg;
//             return {nullptr, error};
//         }
//         case ']': {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(arena, static_cast<i64>(state->line)));
//             msg.append(", found unexpected token \"]\"\n");
//             error->msg = msg;
//             return {nullptr, error};
//         }
//     }
//     if (isNumeral(state->text[state->head]))
//     {
//         return jsonParseNumber(arena, state);
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//     error->next = nullptr;
//
//     u64 begin = state->head;
//     while (state->head < state->text.length && !isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", found unexpected token \"");
//     msg.append({&state->text[begin], &state->text[state->head]});
//     msg.append("\"\n");
//     error->msg = msg;
//
//     return {nullptr, error};
// }
//
// static Json jsonParseStruct(Arena* arena, JsonParseState* state)
// {
//     Json json{};
//     json.file = arena->alloc<JsonNode>(1);
//     json.file->type = JsonType::JsonType_struct;
//     json.file->jstruct.fields = nullptr;
//
//     JsonField* lastField = nullptr;
//     JsonError* lastError = nullptr;
//
//     for (;;)
//     {
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head >= state->text.length)
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", expected struct to terminate\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             break;
//         }
//         if (state->text[state->head] == ']')
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", struct ends with \"]\" instead of \"}\"\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//         if (state->text[state->head] == '}')
//         {
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//
//         Json value = jsonParseNext(arena, state);
//
//         if (value.file != nullptr)
//         {
//             if (value.file->type != JsonType::JsonType_field)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", struct has a literal instead of a field\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else if (value.file->field.value == nullptr)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", struct has a field named \"");
//                 msg.append(value.file->field.name);
//                 msg.append("\" which has no value\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else {
//                 if (lastField == nullptr)
//                     json.file->jstruct.fields = &value.file->field;
//                 else
//                     lastField->next = &value.file->field;
//                 lastField = &value.file->field;
//             }
//         }
//         if (value.errors != nullptr)
//         {
//             if (lastError == nullptr)
//                 json.errors = lastError = value.errors;
//             else
//                 lastError->next = value.errors;
//             lastError = value.errors;
//         }
//     }
//
//     return json;
// }
//
// static Json jsonParseArray(Arena* arena, JsonParseState* state)
// {
//     Json json{};
//     json.file = arena->alloc<JsonNode>(1);
//     json.file->type = JsonType::JsonType_array;
//
//     JsonType type = JsonType::JsonType_none;
//     JsonElem* lastElem = nullptr;
//     JsonError* lastError = nullptr;
//
//     for (;;)
//     {
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head >= state->text.length)
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", expected struct to terminate\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             break;
//         }
//         if (state->text[state->head] == '}')
//         {
//             JsonError* error = arena->alloc<JsonError>(1);
//             error->next = nullptr;
//             StringBuilder msg{arena};
//             msg.append("on line ");
//             msg.append(integerToString(getScratch(), static_cast<i64>(state->line)));
//             msg.append(", array ends with \"}\" instead of \"]\"\n");
//             error->msg = msg;
//             if (lastError == nullptr)
//                 json.errors = lastError = error;
//             else
//                 lastError->next = error;
//             lastError = error;
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//         if (state->text[state->head] == ']')
//         {
//             ++state->head;
//             while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//             {
//                 if (state->text[state->head] == '\n')
//                     ++state->line;
//                 ++state->head;
//             }
//             if (state->head < state->text.length && state->text[state->head] == ',')
//                 ++state->head;
//             break;
//         }
//
//         JsonElem* elem = arena->alloc<JsonElem>(1);
//         elem->next = nullptr;
//
//         Json value = jsonParseNext(arena, state);
//         elem->value = value.file;
//
//         if (value.file != nullptr)
//         {
//             if (type == JsonType::JsonType_none)
//             {
//                 if (value.file->type != JsonType::JsonType_field)
//                 {
//                     type = value.file->type;
//                 } else {
//                     JsonError* error = arena->alloc<JsonError>(1);
//                     error->next = nullptr;
//                     StringBuilder msg{arena};
//                     msg.append("on line ");
//                     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                     msg.append(", array has a field as an element\n");
//                     error->msg = msg;
//                     if (lastError == nullptr)
//                         json.errors = lastError = error;
//                     else
//                         lastError->next = error;
//                     lastError = error;
//                 }
//             }
//             if (value.file->type != type)
//             {
//                 JsonError* error = arena->alloc<JsonError>(1);
//                 error->next = nullptr;
//                 StringBuilder msg{arena};
//                 msg.append("on line ");
//                 msg.append(integerToString(arena, static_cast<i64>(state->line)));
//                 msg.append(", array has element which is not the same type as the first valid element\n");
//                 error->msg = msg;
//                 if (lastError == nullptr)
//                     json.errors = lastError = error;
//                 else
//                     lastError->next = error;
//                 lastError = error;
//             } else {
//                 if (lastElem == nullptr)
//                     json.file->array.elems = elem;
//                 else
//                     lastElem->next = elem;
//                 lastElem = elem;
//             }
//         }
//         if (value.errors != nullptr)
//         {
//             if (lastError == nullptr)
//                 json.errors = lastError = value.errors;
//             else
//                 lastError->next = value.errors;
//             lastError = value.errors;
//         }
//     }
//
//     return json;
// }
//
// static Json jsonParseString(Arena* arena, JsonParseState* state)
// {
//     u64 begin = state->head;
//     while (state->head < state->text.length && state->text[state->head] != '"')
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     u64 end = state->head;
//     if (state->head < state->text.length)
//     {
//         ++state->head;
//         StringBuilder str{};
//         for (u64 i = begin; i < end; ++i)
//         {
//             char c = state->text[i];
//             if (c == '\\')
//             {
//                 // escape sequences : TODO
//             }
//             str.append(c);
//         }
//
//         Json json{};
//         json.file = arena->alloc<JsonNode>(1);
//
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ':')
//         {
//             ++state->head;
//             json.file->type = JsonType::JsonType_field;
//             json.file->field.next = nullptr;
//             json.file->field.name = str;
//             Json next = jsonParseNext(arena, state);
//             json.file->field.value = next.file;
//             json.errors = next.errors;
//         } else {
//             json.file->type = JsonType::JsonType_string;
//             json.file->string = str;
//         }
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//         return json;
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected string to terminate\n");
//     error->msg = msg;
//     return {nullptr, error};
// }
//
// static Json jsonParseNumber(Arena* arena, JsonParseState* state)
// {
//     bool isNumFloat = false;
//     u64 begin = state->head;
//     while (state->head < state->text.length && (
//         isNumeral(state->text[state->head]) ||
//         state->text[state->head] == '-' ||
//         state->text[state->head] == '+' ||
//         state->text[state->head] == '.' ||
//         state->text[state->head] == 'e'
//     ))
//     {
//         if (state->text[state->head] == '.' || state->text[state->head] == 'e')
//             isNumFloat = true;
//         ++state->head;
//     }
//     StringView num{&state->text[begin], &state->text[state->head]};
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->head < state->text.length && state->text[state->head] == ',')
//         ++state->head;
//
//     if (isNumFloat)
//     {
//         if (isFloat(num))
//         {
//             JsonNode* node = arena->alloc<JsonNode>(1);
//             node->type = JsonType::JsonType_float;
//             node->floating = stringToFloat(num);
//             return {node, nullptr};
//         }
//     } else {
//         if (isInteger(num))
//         {
//             JsonNode* node = arena->alloc<JsonNode>(1);
//             node->type = JsonType::JsonType_integer;
//             node->integer = stringToInteger(num);
//             return {node, nullptr};
//         }
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected numeral value, found \"");
//     msg.append(num);
//     msg.append("\"\n");
//     error->msg = msg;
//
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->text[state->head] == '}' || state->text[state->head] == ']')
//     {
//         return {nullptr, error};
//     } else {
//         Json next = jsonParseNext(arena, state);
//         error->next = next.errors;
//         return {next.file, error};
//     }
// }
//
// static Json jsonParseBoolean(Arena* arena, JsonParseState* state)
// {
//     if (state->head + 4 <= state->text.length && StringView{&state->text[state->head], 4} == "true")
//     {
//         state->head += 4;
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//
//         JsonNode* node = arena->alloc<JsonNode>(1);
//         node->type = JsonType::JsonType_bool;
//         node->boolean = true;
//         return {node, nullptr};
//     }
//     if (state->head + 5 <= state->text.length && StringView{&state->text[state->head], 5} == "false")
//     {
//         state->head += 5;
//         while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//         {
//             if (state->text[state->head] == '\n')
//                 ++state->line;
//             ++state->head;
//         }
//         if (state->head < state->text.length && state->text[state->head] == ',')
//             ++state->head;
//
//         JsonNode* node = arena->alloc<JsonNode>(1);
//         node->type = JsonType::JsonType_bool;
//         node->boolean = false;
//         return {node, nullptr};
//     }
//
//     JsonError* error = arena->alloc<JsonError>(1);
//
//     u64 begin = state->head;
//     while (state->head < state->text.length && !isWhitespace(state->text[state->head])
//         && state->text[state->head] != ','
//         && state->text[state->head] != '}'
//         && state->text[state->head] != ']'
//     )
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     StringBuilder msg{arena};
//     msg.append("on line ");
//     msg.append(integerToString(arena, static_cast<i64>(state->line)));
//     msg.append(", expected boolean value, found \"");
//     msg.append({&state->text[begin], &state->text[state->head]});
//     msg.append("\"\n");
//     error->msg = msg;
//
//     if (state->text[state->head] == ',')
//         ++state->head;
//
//     while (state->head < state->text.length && isWhitespace(state->text[state->head]))
//     {
//         if (state->text[state->head] == '\n')
//             ++state->line;
//         ++state->head;
//     }
//     if (state->text[state->head] == '}' || state->text[state->head] == ']')
//     {
//         return {nullptr, error};
//     } else {
//         Json next = jsonParseNext(arena, state);
//         error->next = next.errors;
//         return {next.file, error};
//     }
// }
//
// Json parseJson(Arena* arena, StringView text)
// {
//     HG_ASSERT(arena != nullptr);
//     if (text.length > 0)
//         HG_ASSERT(text.chars != nullptr);
//
//     JsonParseState parseState{};
//     parseState.text = text;
//     parseState.head = 0;
//     parseState.line = 1;
//     return jsonParseNext(arena, &parseState);
// }
// AudioSource* audioSourceAdd(Ecs* ecs, Entity e, Asset<Sound>* audio, bool repeat)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     AudioSource* src = ecsAdd<AudioSource>(ecs, e);
//     *src = {};
//     src->player = audioStreamCreate(8000, 1);
//     src->audio = audio->clone();
//     src->position = 0;
//     src->repeat = repeat;
//
//     return src;
// }
// Ecs ecsCreate()
// {
//     Ecs ecs{};
//     ecs.entities = handlePoolCreate();
//     ecs.components = Map<u64, Component>(128);
//     ecsReset(&ecs);
//     return ecs;
// }
//
// void ecsDestroy(Ecs* ecs)
// {
//     ecsReset(ecs);
//
//     ecs->components.forEach([&](u64*, Component* system)
//     {
//         arrayAnyDestroy(&system->components);
//         system->entities = {};
//         system->indices = {};
//         system->name = {};
//     });
//
//     ecs->components = {};
//     handlePoolDestroy(&ecs->entities);
// }
//
// void ecsReset(Ecs* ecs)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     ecs->components.forEach([&](u64*, Component* system)
//     {
//         for (u32 c = 1; c < system->components.count; ++c)
//         {
//             system->dtor(system->components[c]);
//         }
//         system->entities.count = 1;
//         system->components.count = 1;
//         memset(system->indices.vals, 0, system->indices.count * sizeof(*system->indices.vals));
//     });
//     handlePoolReset(&ecs->entities);
// }
//
// void ecsRegisterComponent(Ecs* ecs, EcsRegisterComponent* config)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     u64 id = hash(config->name);
//     HG_ASSERT(ecs->components.get(id) == nullptr);
//
//     if (ecs->components.count * 2 > ecs->components.capacity)
//         ecs->components.resize(ecs->components.capacity * 2);
//     Component* system = ecs->components.add(id, {});
//
//     system->name = String::create(config->name);
//     system->indices = Array<u32>{0, 1024};
//     system->entities = Array<Entity>{0, 1024};
//     system->components = arrayAnyCreate(config->width, config->align);
//     system->dtor = config->dtor;
//     system->serialize = config->serialize;
//
//     system->entities.push();
//     arrayAnyPush(&system->components);
// }
//
// void ecsUnregisterComponent(Ecs* ecs, u64 componentId)
// {
//     Component* system = ecs->components.get(componentId);
//
//     for (u32 c = 1; c < system->components.count; ++c)
//     {
//         system->dtor(system->components[c]);
//     }
//     arrayAnyDestroy(&system->components);
//     system->entities = {};
//     system->indices = {};
//     system->name = {};
//
//     ecs->components.remove(componentId);
// }
//
// StringView ecsComponentName(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//     return system->name;
// }
//
// Entity ecsSpawn(Ecs* ecs)
// {
//     HG_ASSERT(ecs != nullptr);
//     return {handlePoolAlloc(&ecs->entities)};
// }
//
// void ecsDespawn(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     ecs->components.forEach([&](u64* id, Component*)
//     {
//         if (ecsHas(ecs, e, *id))
//             ecsRemove(ecs, e, *id);
//     });
//     handlePoolFree(&ecs->entities, e.handle);
// }
//
// bool ecsAlive(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     return handlePoolAlive(&ecs->entities, e.handle);
// }
//
// void* ecsAdd(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(!ecsHas(ecs, e, componentId));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     u32 idx = handleIdx(e.handle);
//     if (idx >= system->indices.count)
//     {
//         u32 oldCount = system->indices.count;
//         u32 newCount = idx * 2;
//         system->indices.resize(newCount);
//         for (u32 i = oldCount; i < newCount; ++i)
//             system->indices[i] = 0;
//     }
//     system->indices[idx] = system->entities.count;
//     system->entities.push(e);
//     return arrayAnyPush(&system->components);
// }
//
// void ecsRemove(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(ecsHas(ecs, e, componentId));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     u32 idx = system->indices[handleIdx(e.handle)];
//     system->dtor(system->components[idx]);
//
//     Entity last = system->entities.pop();
//     if (e != last)
//     {
//         system->entities[idx] = last;
//         system->indices[handleIdx(last.handle)] = idx;
//         memcpy(
//             system->components[idx],
//             system->components[system->components.count - 1],
//             system->components.width);
//     }
//     system->indices[handleIdx(e.handle)] = 0;
//     --system->components.count;
// }
//
// bool ecsHas(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Component* system = ecs->components.get(componentId);
//     if (system == nullptr)
//         return false;
//
//     u32 idx = handleIdx(e.handle);
//     return idx < system->indices.count && system->indices[idx] != 0;
// }
//
// void* ecsGet(Ecs* ecs, Entity e, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//     HG_ASSERT(system->indices[handleIdx(e.handle)] != 0);
//     HG_ASSERT(system->indices[handleIdx(e.handle)] < system->entities.count);
//
//     return system->components[system->indices[handleIdx(e.handle)]];
// }
//
// Entity ecsGetEntity(Ecs* ecs, const void* component, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(component != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     return system->entities[static_cast<u32>(reinterpret_cast<uptr>(component) - reinterpret_cast<uptr>(system->components.vals)) / system->components.width];
// }
//
// Entity* ecsEntities(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->entities.count != 0);
//     return ecs->components.get(componentId)->entities.vals + 1;
// }
//
// void* ecsComponents(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->components.count != 0);
//     Component* system = ecs->components.get(componentId);
//     return static_cast<u8*>(system->components.vals) + system->components.width;
// }
//
// u32 ecsCount(Ecs* ecs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecs->components.get(componentId) != nullptr);
//     HG_ASSERT(ecs->components.get(componentId)->entities.count != 0);
//     return ecs->components.get(componentId)->entities.count - 1;
// }
//
// u64 ecsFindSmallest(Ecs* ecs, u64* ids, u32 idCount)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(idCount > 0);
//
//     u32 smallestCount = static_cast<u32>(-1);
//     u64 smallest = ids[0];
//
//     for (u32 i = 1; i < idCount; ++i)
//     {
//         Component* system = ecs->components.get(ids[i]);
//         HG_ASSERT(system != nullptr);
//
//         if (system->entities.count < smallestCount)
//         {
//             smallestCount = system->entities.count;
//             smallest = ids[i];
//         }
//     }
//     return smallest;
// }
//
// static void swapIdxLocation(Ecs* ecs, u32 lhs, u32 rhs, u64 componentId)
// {
//     HG_ASSERT(ecs != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     HG_ASSERT(lhs != 0 && lhs < system->entities.count);
//     HG_ASSERT(rhs != 0 && rhs < system->entities.count);
//
//     Entity lhsEntity = system->entities[lhs];
//     Entity rhsEntity = system->entities[rhs];
//
//     HG_ASSERT(ecsAlive(ecs, lhsEntity));
//     HG_ASSERT(ecsAlive(ecs, rhsEntity));
//     HG_ASSERT(ecsHas(ecs, lhsEntity, componentId));
//     HG_ASSERT(ecsHas(ecs, rhsEntity, componentId));
//
//     ArenaScope scratch = getScratch();
//
//     system->entities[lhs] = rhsEntity;
//     system->entities[rhs] = lhsEntity;
//     system->indices[handleIdx(lhsEntity.handle)] = rhs;
//     system->indices[handleIdx(rhsEntity.handle)] = lhs;
//
//     void* temp = scratch.alloc(system->components.width, 1);
//     memcpy(temp, system->components[lhs], system->components.width);
//     memcpy(system->components[lhs], system->components[rhs], system->components.width);
//     memcpy(system->components[rhs], temp, system->components.width);
// }
//
// namespace {
//     struct QuicksortData {
//         Ecs* ecs = nullptr;
//         Component* system = nullptr;
//         u64 comp = 0;
//         void* data = nullptr;
//         bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs) = nullptr;
//
//         u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
//         {
//             while (inc != dec)
//             {
//                 while (!compare(data, ecs, system->entities[dec], system->entities[pivot]))
//                 {
//                     --dec;
//                     if (dec == inc)
//                         goto finish;
//                 }
//                 while (!compare(data, ecs, system->entities[pivot], system->entities[inc]))
//                 {
//                     ++inc;
//                     if (inc == dec)
//                         goto finish;
//                 }
//                 swapIdxLocation(ecs, inc, dec, comp);
//             }
//
//         finish:
//             if (compare(data, ecs, system->entities[inc], system->entities[pivot]))
//                 swapIdxLocation(ecs, pivot, inc, comp);
//
//             return inc;
//         }
//
//         void quicksort(u32 begin, u32 end)
//         {
//             if (begin + 1 >= end)
//                 return;
//
//             u32 middle = quicksortInter(begin, begin + 1, end - 1);
//             quicksort(begin, middle);
//             quicksort(middle, end);
//         }
//     };
// }
//
// void ecsSort(
//     Ecs* ecs,
//     u64 componentId,
//     void* data,
//     bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs))
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(compare != nullptr);
//
//     Component* system = ecs->components.get(componentId);
//     HG_ASSERT(system != nullptr);
//
//     QuicksortData q{ecs, system, componentId, data, compare};
//     q.quicksort(1, system->entities.count);
// }
//
// template<>
// void serialize(Serializer* s, Ecs* ecs)
// {
//     HG_ASSERT(s != nullptr);
//     HG_ASSERT(ecs != nullptr);
//
//     ArenaScope scratch = getScratch(&s->arena, 1);
//
//     serializeBegin(s);
//     HG_DEFER(serializeEnd(s));
//
//     EntitySerializer ecsSerial{};
//     u32 entityCount = 0;
//     if (s->writing)
//     {
//         ecsSerial.entityToIdx = scratch.alloc<u32>(ecs->entities.handles.count);
//         for (u32 i = 1; i < ecs->entities.handles.count; ++i)
//         {
//             if (ecs->entities.handles[i] != handleNull)
//                 ecsSerial.entityToIdx[handleIdx(ecs->entities.handles[i])] = entityCount++;
//         }
//
//         serialize(s, &entityCount);
//     }
//     else
//     {
//         serialize(s, &entityCount);
//
//         ecsSerial.idxToEntity = scratch.alloc<Entity>(entityCount);
//         for (u32 i = 0; i < entityCount; ++i)
//         {
//             ecsSerial.idxToEntity[i] = ecsSpawn(ecs);
//         }
//     }
//
//     u32 systemCount;
//     if (s->writing)
//         systemCount = ecs->components.count;
//     serializeBegin(s, &systemCount);
//     HG_DEFER(serializeEnd(s));
//
//     u32 systemIdx = static_cast<u32>(-1);
//     for (u32 i = 0; i < systemCount; ++i)
//     {
//         serializeBegin(s);
//         HG_DEFER(serializeEnd(s));
//
//         u64 systemId = static_cast<u64>(-1);
//         Component* system;
//         if (s->writing)
//         {
//             ++systemIdx;
//             while (!ecs->components.hasVal[systemIdx])
//             {
//                 ++systemIdx;
//             }
//             systemId = ecs->components.keys[systemIdx];
//             system = &ecs->components.vals[systemIdx];
//             serialize(s, &system->name);
//         }
//         else
//         {
//             StringView compName;
//             serialize(s, &compName);
//             systemId = hash(compName);
//             system = ecs->components.get(systemId);
//         }
//
//         u32 compCount;
//         if (s->writing)
//             compCount = system->entities.count - 1;
//         serializeBegin(s, &compCount);
//         HG_DEFER(serializeEnd(s));
//
//         for (u32 c = 0; c < compCount; ++c)
//         {
//             serializeBegin(s);
//             HG_DEFER(serializeEnd(s));
//
//             u32 entityIdx;
//             if (s->writing)
//                 entityIdx = ecsSerial.entityToIdx[handleIdx(system->entities[c + 1].handle)];
//             serialize(s, &entityIdx);
//
//             void* compData;
//             if (s->writing)
//                 compData = system->components[c + 1];
//             else
//                 compData = ecsAdd(ecs, ecsSerial.idxToEntity[entityIdx], systemId);
//             system->serialize(s, compData, &ecsSerial);
//         }
//     }
// }
//
// void entitySerialize(Serializer* s, Entity* val, EntitySerializer* ecs)
// {
//     if (s->writing)
//     {
//         u32 idx = *val != entityNull ? ecs->entityToIdx[handleIdx(val->handle)] : static_cast<u32>(-1);
//         serialize(s, reinterpret_cast<i32*>(&idx));
//     }
//     else
//     {
//         u32 idx = static_cast<u32>(-1);
//         serialize(s, reinterpret_cast<i32*>(&idx));
//         *val = idx != static_cast<u32>(-1) ? ecs->idxToEntity[idx] : entityNull;
//     }
// }
//
// template<>
// void ecsSerialize(Serializer* s, Node* node, EntitySerializer* ecs)
// {
//     serializeBegin(s);
//     entitySerialize(s, &node->parent, ecs);
//     entitySerialize(s, &node->nextSibling, ecs);
//     entitySerialize(s, &node->prevSibling, ecs);
//     entitySerialize(s, &node->firstChild, ecs);
//     serializeEnd(s);
// }
//
// Node* nodeAdd(Ecs* ecs, Entity e)
// {
//     Node* node = ecsAdd<Node>(ecs, e);
//     *node = {};
//     return node;
// }
//
// void nodeDestroy(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//     if (node->parent.handle != handleNull)
//     {
//         if (node->prevSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
//         else
//             ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;
//
//         if (node->nextSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
//     }
//
//     Entity child = node->firstChild;
//     while (child.handle != handleNull)
//     {
//         Node* childNode = ecsGet<Node>(ecs, child);
//         Entity next = childNode->nextSibling;
//         nodeDestroy(ecs, child);
//         child = next;
//     }
//
//     ecsDespawn(ecs, e);
// }
//
// void nodeDetach(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//     if (node->parent.handle == handleNull)
//     {
//         HG_ASSERT(node->prevSibling.handle == handleNull);
//         HG_ASSERT(node->nextSibling.handle == handleNull);
//
//         Entity child = node->firstChild;
//         while (child.handle != handleNull)
//         {
//             Node* childNode = ecsGet<Node>(ecs, child);
//             Entity next = childNode->nextSibling;
//             childNode->parent = Entity{};
//             childNode->nextSibling = Entity{};
//             childNode->prevSibling = Entity{};
//             child = next;
//         }
//     } else {
//         if (node->prevSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
//         else
//             ecsGet<Node>(ecs, node->parent)->firstChild = node->nextSibling;
//
//         if (node->nextSibling.handle != handleNull)
//             ecsGet<Node>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
//
//         Entity child = node->firstChild;
//         while (child.handle != handleNull)
//         {
//             Node* childNode = ecsGet<Node>(ecs, child);
//             Entity next = childNode->nextSibling;
//             childNode->parent = Entity{};
//             childNode->nextSibling = Entity{};
//             childNode->prevSibling = Entity{};
//             nodeAddChild(ecs, node->parent, child);
//             child = next;
//         }
//     }
//     *node = {};
// }
//
// void nodeAddChild(Ecs* ecs, Entity parent, Entity child)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, parent));
//     HG_ASSERT(ecsAlive(ecs, child));
//
//     Node* node = ecsGet<Node>(ecs, parent);
//     Node* childNode = ecsGet<Node>(ecs, child);
//
//     HG_ASSERT(childNode->parent.handle == handleNull);
//     HG_ASSERT(childNode->prevSibling.handle == handleNull);
//     HG_ASSERT(childNode->nextSibling.handle == handleNull);
//
//     if (node->firstChild.handle != handleNull)
//     {
//         ecsGet<Node>(ecs, node->firstChild)->prevSibling = child;
//         childNode->nextSibling = node->firstChild;
//     }
//     node->firstChild = child;
//     childNode->parent = parent;
// }
//
// template<>
// void serialize(Serializer* s, Transform* node)
// {
//     serializeObject(s,
//         &node->position,
//         &node->scale,
//         &node->rotation);
// }
//
// Transform* transformAdd(Ecs* ecs, Entity e, Vec3 position, Vec3 scale, Quat rotation)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Transform* tf = ecsAdd<Transform>(ecs, e);
//     *tf = {};
//     tf->position = position;
//     tf->scale = scale;
//     tf->rotation = rotation;
//     transformUpdate(ecs, e);
//
//     return tf;
// }
//
// static void transformUpdateChild(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecsHas<Transform>(ecs, e));
//
//     Node* node = ecsGet<Node>(ecs, e);
//
//     Transform* tf = ecsGet<Transform>(ecs, e);
//
//     tf->mat = ecsGet<Transform>(ecs, node->parent)->mat
//             * matModel3D(tf->position, tf->scale, tf->rotation);
//
//     Entity child = node->firstChild;
//     while (child.handle != handleNull)
//     {
//         transformUpdate(ecs, child);
//         child = ecsGet<Node>(ecs, child)->nextSibling;
//     }
// }
//
// void transformUpdate(Ecs* ecs, Entity e)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//     HG_ASSERT(ecsHas<Transform>(ecs, e));
//
//     if (ecsHas<Node>(ecs, e))
//     {
//         Node* node = ecsGet<Node>(ecs, e);
//         if (node->parent.handle != handleNull && ecsHas<Transform>(ecs, node->parent))
//         {
//             transformUpdateChild(ecs, e);
//         }
//         else
//         {
//             Transform* tf = ecsGet<Transform>(ecs, e);
//             tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);
//
//             Entity child = node->firstChild;
//             while (child.handle != handleNull)
//             {
//                 transformUpdateChild(ecs, child);
//                 child = ecsGet<Node>(ecs, child)->nextSibling;
//             }
//         }
//     }
//     else
//     {
//         Transform* tf = ecsGet<Transform>(ecs, e);
//         tf->mat = matModel3D(tf->position, tf->scale, tf->rotation);
//     }
// }
//
// template<>
// void ecsDtor(AudioSource* src)
// {
//     src->audio = {};
//     audioStreamDestroy(src->player);
// }
//
// void audioUpdate(Ecs* ecs, Entity listener)
// {
//     ecsForEach<AudioSource>(ecs, [&](Entity e, AudioSource* src)
//     {
//         Sound* audio = &*src->audio;
//         if (src->position == audio->size && !src->repeat)
//             return;
//
//         ArenaScope scratch = getScratch();
//
//         u32 width = sizeof(f32);
//
//         u32 total = audio->frequency * width / 8;
//         u32 queued = audioStreamQueuedSize(src->player);
//         if (queued >= total)
//             return;
//         u32 sizeToPush = total - queued;
//
//         f32* queue = static_cast<f32*>(scratch.alloc(sizeToPush, width));
//         u32 queueSize = 0;
//
//         if (src->repeat)
//         {
//             while (queueSize < sizeToPush)
//             {
//                 if (src->position == audio->size)
//                     src->position = 0;
//
//                 u32 sizeToQueue = std::min(sizeToPush - queueSize, static_cast<u32>(audio->size - src->position));
//                 memcpy(reinterpret_cast<u8*>(queue) + queueSize, reinterpret_cast<u8*>(audio->data) + src->position, sizeToQueue);
//                 queueSize += sizeToQueue;
//                 src->position += sizeToQueue;
//             }
//         }
//         else
//         {
//             queueSize = std::min(sizeToPush, static_cast<u32>(audio->size - src->position));
//             memcpy(queue, reinterpret_cast<u8*>(audio->data) + src->position, queueSize);
//             src->position += queueSize;
//         }
//
//         if (ecsHas<Transform>(ecs, e))
//         {
//             HG_ASSERT(ecsHas<Transform>(ecs, listener));
//
//             Vec3 relPos = transformWorldPos(*ecsGet<Transform>(ecs, listener))
//                           - transformWorldPos(*ecsGet<Transform>(ecs, e));
//             f32 factor = 1.0f / vecDot3(relPos, relPos);
//
//             for (u64 i = 0; i < sizeToPush / sizeof(f32); ++i)
//             {
//                 queue[i] *= factor;
//             }
//         }
//
//         HG_ASSERT(queueSize <= sizeToPush);
//         audioStreamPush(src->player, queue, queueSize);
//     });
// }
//
// struct SpritePipelinePush {
//     Mat4 model = {};
//     Vec2 uvPos = {};
//     Vec2 uvSize = {};
//     u32 viewProj = 0;
//     u32 texture = 0;
// };
//
// struct SpritePipelineState {
//     GpuPipeline* pipeline = nullptr;
//     Texture defaultTex = {};
// };
//
// static SpritePipelineState spritePipeline{};
//
// #include "sprite.vert.spv.h"
// #include "sprite.frag.spv.h"
//
// void spritesInit(
//     Format colorFormat,
//     Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//     HG_ASSERT(depthFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = sprite_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(sprite_vert_spv);
//     pipelineConfig.fragmentShader = sprite_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(sprite_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(SpritePipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     pipelineConfig.enableDepthRead = true;
//     pipelineConfig.enableDepthWrite = true;
//     bool enableColorBlend = true;
//     pipelineConfig.colorBlendEnables = &enableColorBlend;
//
//     spritePipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color defaultColors[]{
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
//         {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//     };
//
//     spritePipeline.defaultTex.image = gpuImageCreate(2, 2, Format_r8g8b8a8_srgb,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//
//     spritePipeline.defaultTex.view = gpuViewCreate(
//         spritePipeline.defaultTex.image, GpuAspect_color, GpuFilter_nearest);
//
//     gpuImageWrite(spritePipeline.defaultTex.view, defaultColors);
// }
//
// void spritesDeinit()
// {
//     gpuPipelineDestroy(spritePipeline.pipeline);
//
//     gpuViewDestroy(spritePipeline.defaultTex.view);
//     gpuImageDestroy(spritePipeline.defaultTex.image);
// }
//
// template<>
// void serialize(Serializer* s, Sprite* sprite)
// {
//     serializeObject(s,
//         &sprite->texture,
//         &sprite->uvPos,
//         &sprite->uvSize);
// }
//
// Sprite* spriteAdd(Ecs* ecs, Entity e, Asset<Texture>* texture, Vec2 uvPos, Vec2 uvSize)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Sprite* sprite = ecsAdd<Sprite>(ecs, e);
//     *sprite = {};
//     sprite->texture = texture->clone();
//     sprite->uvPos = uvPos;
//     sprite->uvSize = uvSize;
//
//     return sprite;
// }
//
// template<>
// void ecsDtor(Sprite* sprite)
// {
//     *sprite = {};
// }
//
// void spritesDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(cmd != nullptr);
//
//     gpuBindPipeline(cmd, spritePipeline.pipeline);
//
//     ecsForEach<Sprite, Transform>(ecs, [&](Entity, Sprite* sprite, Transform* tf)
//     {
//         Texture* texture = sprite->texture == nullptr
//             ? &spritePipeline.defaultTex
//             : &*sprite->texture;
//
//         SpritePipelinePush push{};
//         push.model = tf->mat;
//         push.uvPos = sprite->uvPos;
//         push.uvSize = sprite->uvSize;
//         push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
//         push.texture = gpuImageSamplerDescriptor(texture->view);
//
//         gpuPushConstants(cmd, spritePipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, 6, 0, 1);
//     });
// }
//
// struct SkyboxPipelinePush {
//     u32 viewProj = 0;
//     u32 texture = 0;
// };
//
// struct SkyboxPipelineState {
//     GpuPipeline* pipeline = nullptr;
//     Texture defaultTex = {};
// };
//
// static SkyboxPipelineState skyboxPipeline{};
//
// #include "skybox.vert.spv.h"
// #include "skybox.frag.spv.h"
//
// void skyboxInit(Format colorFormat, Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = skybox_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(skybox_vert_spv);
//     pipelineConfig.fragmentShader = skybox_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(skybox_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(SkyboxPipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     bool enableColorBlend = true;
//     pipelineConfig.colorBlendEnables = &enableColorBlend;
//
//     skyboxPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color top = {0x00, 0x22, 0x44, 0xff};
//     Color mid = {0x00, 0x11, 0x33, 0xff};
//     Color bot = {0x00, 0x00, 0x00, 0xff};
//     Color nul = {};
//     Color defaultColors[]{
//         nul, top, nul, nul,
//         mid, mid, mid, mid,
//         nul, bot, nul, nul,
//     };
//
//     GpuImageCreateEx imageConfig{};
//     imageConfig.width = 1;
//     imageConfig.height = 1;
//     imageConfig.format = Format_r8g8b8a8_srgb;
//     imageConfig.arrayLayers = 6;
//     imageConfig.usage = GpuImageUsage_sampled | GpuImageUsage_transferDst;
//     imageConfig.flags = GpuImageConfig_cubeCompatible;
//
//     skyboxPipeline.defaultTex.image = gpuImageCreateEx(&imageConfig);
//
//     GpuViewCreateEx viewConfig{};
//     viewConfig.image = skyboxPipeline.defaultTex.image;
//     viewConfig.baseMipLevel = 0;
//     viewConfig.levelCount = 1;
//     viewConfig.baseArrayLayer = 0;
//     viewConfig.layerCount = 6;
//     viewConfig.aspectFlags = GpuAspect_color;
//     viewConfig.type = GpuViewType_cube;
//     viewConfig.filter = GpuFilter_nearest;
//     viewConfig.edgeMode = GpuSamplerEdgeMode_repeat;
//     viewConfig.border = GpuSamplerBorder_floatTransparentBlack;
//
//     skyboxPipeline.defaultTex.view = gpuViewCreateEx(&viewConfig);
//
//     gpuImageWriteCubemap(skyboxPipeline.defaultTex.view, defaultColors);
// }
//
// void skyboxDeinit()
// {
//     gpuPipelineDestroy(skyboxPipeline.pipeline);
//
//     gpuViewDestroy(skyboxPipeline.defaultTex.view);
//     gpuImageDestroy(skyboxPipeline.defaultTex.image);
// }
//
// template<>
// void serialize(Serializer* s, Skybox* skybox)
// {
//     serializeObject(s,
//         &skybox->texture);
// }
//
// Skybox* skyboxAdd(Ecs* ecs, Entity e, Asset<Texture>* texture)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Skybox* skybox = ecsAdd<Skybox>(ecs, e);
//     *skybox = {texture->clone()};
//
//     return skybox;
// }
//
// template<>
// void ecsDtor(Skybox* skybox)
// {
//     *skybox = {};
// }
//
// void skyboxDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     gpuBindPipeline(cmd, skyboxPipeline.pipeline);
//
//     ecsForEach<Skybox>(ecs, [&](Entity, Skybox* skybox)
//     {
//         Texture* texture = skybox->texture == nullptr
//             ? &skyboxPipeline.defaultTex
//             : &*skybox->texture;
//
//         SkyboxPipelinePush push{};
//         push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
//         push.texture = gpuImageSamplerDescriptor(texture->view);
//
//         gpuPushConstants(cmd, skyboxPipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, 36, 0, 1);
//     });
// }
//
// template<>
// void serialize(Serializer* s, DirLight* light)
// {
//     serializeObject(s,
//         &light->dir,
//         &light->color);
// }
//
// DirLight* dirLightAdd(Ecs* ecs, Entity e, Vec3 dir, Vec4 color)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     DirLight* light = ecsAdd<DirLight>(ecs, e);
//     *light = {dir, color};
//
//     return light;
// }
//
// template<>
// void serialize(Serializer* s, PointLight* light)
// {
//     serializeObject(s,
//         &light->color);
// }
//
// PointLight* pointLightAdd(Ecs* ecs, Entity e, Vec4 color)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     PointLight* light = ecsAdd<PointLight>(ecs, e);
//     *light = {color};
//
//     return light;
// }
//
// struct ModelPipelineDirLightData {
//     Vec4 dir = {};
//     Vec4 color = {};
// };
//
// struct ModelPipelinePointLightData {
//     Vec4 pos = {};
//     Vec4 color = {};
// };
//
// struct ModelPipelinePush {
//     Mat4 model = {};
//     u32 indices = 0;
//     u32 vertices = 0;
//     u32 viewProj = 0;
//     u32 normalMap = 0;
//     u32 colorMap = 0;
//     u32 dirLights = 0;
//     u32 dirLightCount = 0;
//     u32 pointLights = 0;
//     u32 pointLightCount = 0;
// };
//
// struct ModelPipelineState {
//     GpuPipeline* pipeline = nullptr;
//
//     GpuBuffer* dirLightBuffer = nullptr;
//     u32 dirLightCapacity = 0;
//
//     GpuBuffer* pointLightBuffer = nullptr;
//     u32 pointLightCapacity = 0;
//
//     Mesh defaultModel = {};
//     Texture defaultColorMap = {};
//     Texture defaultNormalMap = {};
// };
//
// static ModelPipelineState modelPipeline{};
//
// #include "model.vert.spv.h"
// #include "model.frag.spv.h"
//
// void modelsInit(
//     Format colorFormat,
//     Format depthFormat)
// {
//     HG_ASSERT(colorFormat != Format_undefined);
//     HG_ASSERT(depthFormat != Format_undefined);
//
//     CreateGpuGraphicsPipeline pipelineConfig{};
//     pipelineConfig.vertexShader = model_vert_spv;
//     pipelineConfig.vertexShaderSize = sizeof(model_vert_spv);
//     pipelineConfig.fragmentShader = model_frag_spv;
//     pipelineConfig.fragmentShaderSize = sizeof(model_frag_spv);
//     pipelineConfig.pushConstantSize = sizeof(ModelPipelinePush);
//     pipelineConfig.colorAttachmentFormats = &colorFormat;
//     pipelineConfig.colorAttachmentCount = 1;
//     pipelineConfig.depthAttachmentFormat = depthFormat;
//     pipelineConfig.cullMode = GpuCull_back;
//     pipelineConfig.enableDepthRead = true;
//     pipelineConfig.enableDepthWrite = true;
//
//     modelPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);
//
//     modelPipeline.dirLightCapacity = 16;
//     modelPipeline.dirLightBuffer = gpuBufferCreate(
//         sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
//         GpuBufferUsage_storageBuffer,
//         GpuMemoryUsage_frequentUpdate);
//
//     modelPipeline.pointLightCapacity = 64;
//     modelPipeline.pointLightBuffer = gpuBufferCreate(
//         sizeof(ModelPipelinePointLightData) * modelPipeline.dirLightCapacity,
//         GpuBufferUsage_storageBuffer,
//         GpuMemoryUsage_frequentUpdate);
//
//     MeshVertex cubeVertices[]{
//         {{ 0.5f,-0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,0}},
//         {{ 0.5f, 0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,1}},
//         {{ 0.5f,-0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,0}},
//         {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,0}},
//         {{-0.5f, 0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,1}},
//         {{-0.5f,-0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,1}},
//
//         {{-0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,0}},
//         {{-0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,1}},
//         {{ 0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,0}},
//         {{-0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,1}},
//         {{ 0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,1}},
//
//         {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,0}},
//         {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,0}},
//         {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,1}},
//         {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,1}},
//
//         {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,0}},
//         {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,0}},
//         {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,1}},
//         {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,1}},
//     };
//
//     u32 cubeIndices[]{
//          0,  1,  2,  0,  2,  3,
//          4,  5,  6,  4,  6,  7,
//          8,  9, 10,  8, 10, 11,
//         12, 13, 14, 12, 14, 15,
//         16, 17, 18, 16, 18, 19,
//         20, 21, 22, 20, 22, 23
//     };
//
//     modelPipeline.defaultModel.vertexBuffer = gpuBufferCreate(
//         sizeof(cubeVertices),
//         GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
//
//     modelPipeline.defaultModel.indexBuffer = gpuBufferCreate(
//         sizeof(cubeIndices),
//         GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
//
//     gpuBufferWrite(modelPipeline.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
//     gpuBufferWrite(modelPipeline.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));
//
//     modelPipeline.defaultModel.vertexCount = static_cast<u32>(std::size(cubeVertices));
//     modelPipeline.defaultModel.vertexWidth = static_cast<u32>(sizeof(MeshVertex));
//     modelPipeline.defaultModel.indexCount = static_cast<u32>(std::size(cubeIndices));
//
//     struct Color {
//         u8 r, g, b, a;
//     };
//     Color defaultColors[]{
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//         {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
//         {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
//     };
//
//     Vec4 defaultNormal{0, 0, -1, 0};
//
//     modelPipeline.defaultColorMap.image = gpuImageCreate(3, 3, Format_r8g8b8a8_srgb,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//     modelPipeline.defaultNormalMap.image = gpuImageCreate(1, 1, Format_r32g32b32a32_sfloat,
//         GpuImageUsage_sampled | GpuImageUsage_transferDst);
//
//     modelPipeline.defaultColorMap.view = gpuViewCreate(
//         modelPipeline.defaultColorMap.image, GpuAspect_color, GpuFilter_nearest);
//     modelPipeline.defaultNormalMap.view = gpuViewCreate(
//         modelPipeline.defaultNormalMap.image, GpuAspect_color, GpuFilter_nearest);
//
//     gpuImageWrite(modelPipeline.defaultColorMap.view, defaultColors);
//     gpuImageWrite(modelPipeline.defaultNormalMap.view, &defaultNormal);
// }
//
// void modelsDeinit()
// {
//     gpuPipelineDestroy(modelPipeline.pipeline);
//
//     gpuViewDestroy(modelPipeline.defaultNormalMap.view);
//     gpuImageDestroy(modelPipeline.defaultNormalMap.image);
//
//     gpuViewDestroy(modelPipeline.defaultColorMap.view);
//     gpuImageDestroy(modelPipeline.defaultColorMap.image);
//
//     gpuBufferDestroy(modelPipeline.defaultModel.indexBuffer);
//     gpuBufferDestroy(modelPipeline.defaultModel.vertexBuffer);
//
//     gpuBufferDestroy(modelPipeline.pointLightBuffer);
//     gpuBufferDestroy(modelPipeline.dirLightBuffer);
// }
//
// template<>
// void serialize(Serializer* s, Model* model)
// {
//     serializeObject(s,
//         &model->mesh,
//         &model->colorMap,
//         &model->normalMap);
// }
//
// Model* modelAdd(
//     Ecs* ecs,
//     Entity e,
//     Asset<Mesh>* mesh,
//     Asset<Texture>* colorMap,
//     Asset<Texture>* normalMap)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(ecsAlive(ecs, e));
//
//     Model* model = ecsAdd<Model>(ecs, e);
//     *model = {};
//     model->mesh = mesh->clone();
//     model->colorMap = colorMap->clone();
//     model->normalMap = normalMap->clone();
//
//     return model;
// }
//
// template<>
// void ecsDtor(Model* model)
// {
//     *model = {};
// }
//
// void modelsDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
// {
//     HG_ASSERT(ecs != nullptr);
//     HG_ASSERT(cmd != nullptr);
//
//     ArenaScope scratch = getScratch();
//
//     Camera* cameraC = ecsGet<Camera>(ecs, camera);
//     Transform* cameraTf = ecsGet<Transform>(ecs, camera);
//
//     Mat4 view = matModelToView(cameraTf->mat);
//
//     u32 dirLightCount = ecsCount<DirLight>(ecs);
//     u32 pointLightCount = ecsCount<PointLight>(ecs);
//
//     if (dirLightCount > modelPipeline.dirLightCapacity)
//     {
//         if (modelPipeline.dirLightCapacity == 0)
//             modelPipeline.dirLightCapacity = 1;
//         while (modelPipeline.dirLightCapacity < dirLightCount)
//         {
//             modelPipeline.dirLightCapacity *= 2;
//         }
//
//         gpuWaitIdle();
//
//         gpuBufferDestroy(modelPipeline.dirLightBuffer);
//         modelPipeline.dirLightBuffer = gpuBufferCreate(
//             sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
//             GpuBufferUsage_storageBuffer,
//             GpuMemoryUsage_frequentUpdate);
//     }
//
//     if (pointLightCount > modelPipeline.pointLightCapacity)
//     {
//         if (modelPipeline.pointLightCapacity == 0)
//             modelPipeline.pointLightCapacity = 1;
//         while (modelPipeline.pointLightCapacity < pointLightCount)
//         {
//             modelPipeline.pointLightCapacity *= 2;
//         }
//
//         gpuWaitIdle();
//
//         gpuBufferDestroy(modelPipeline.pointLightBuffer);
//         modelPipeline.pointLightBuffer = gpuBufferCreate(
//             sizeof(ModelPipelinePointLightData) * modelPipeline.pointLightCapacity,
//             GpuBufferUsage_storageBuffer,
//             GpuMemoryUsage_frequentUpdate);
//     }
//
//     ModelPipelineDirLightData* dirLights = scratch.alloc<ModelPipelineDirLightData>(dirLightCount);
//     ModelPipelinePointLightData* pointLights = scratch.alloc<ModelPipelinePointLightData>(pointLightCount);
//
//     u32 i = 0;
//     ecsForEach<DirLight>(ecs, [&](Entity, DirLight* light)
//     {
//         dirLights[i].dir = Vec4{Mat3{view} * light->dir, 0.0};
//         dirLights[i].color = light->color;
//         ++i;
//     });
//
//     i = 0;
//     ecsForEach<PointLight, Transform>(ecs, [&](Entity, PointLight* light, Transform* tf)
//     {
//         pointLights[i].pos = view * Vec4{transformWorldPos(*tf), 1.0};
//         pointLights[i].color = light->color;
//         ++i;
//     });
//
//     if (dirLightCount > 0)
//         gpuBufferWrite(modelPipeline.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);
//
//     if (pointLightCount > 0)
//         gpuBufferWrite(modelPipeline.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);
//
//     gpuBindPipeline(cmd, modelPipeline.pipeline);
//
//     ecsForEach<Model, Transform>(ecs, [&](Entity, Model* model, Transform* tf)
//     {
//         Texture* colorMap = model->colorMap == nullptr
//             ? &modelPipeline.defaultColorMap
//             : &*model->colorMap;
//
//         Texture* normalMap = model->normalMap == nullptr
//             ? &modelPipeline.defaultNormalMap
//             : &*model->normalMap;
//
//         Mesh* gpuModel = model->mesh == nullptr
//             ? &modelPipeline.defaultModel
//             : &*model->mesh;
//
//         ModelPipelinePush push{};
//         push.model = tf->mat;
//         push.vertices = gpuBufferStorageDescriptor(gpuModel->vertexBuffer);
//         push.indices = gpuBufferStorageDescriptor(gpuModel->indexBuffer);
//         push.viewProj = gpuBufferUniformDescriptor(cameraC->vpBuffer);
//         push.normalMap = gpuImageSamplerDescriptor(normalMap->view);
//         push.colorMap = gpuImageSamplerDescriptor(colorMap->view);
//         push.dirLights = gpuBufferStorageDescriptor(modelPipeline.dirLightBuffer);
//         push.dirLightCount = dirLightCount;
//         push.pointLights = gpuBufferStorageDescriptor(modelPipeline.pointLightBuffer);
//         push.pointLightCount = pointLightCount;
//
//         gpuPushConstants(cmd, modelPipeline.pipeline, &push, sizeof(push));
//
//         gpuDraw(cmd, 0, gpuModel->indexCount, 0, 1);
//     });
// }


} // namespace hg
