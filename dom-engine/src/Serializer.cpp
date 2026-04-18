#include "Serializer.h"

#include "Node.h"

#include <cctype>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {

// Escape special characters for JSON strings
std::string json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result += c;
        }
    }
    return result;
}

// Serialize node to JSON (internal recursive helper)
std::string serialize_node(const Node& node) {
    std::ostringstream oss;
    oss << "{\"type\":\"" << json_escape(node.type()) << "\"";

    // Serialize attributes
    oss << ",\"attributes\":{";
    bool first = true;
    for (const auto& [attr, val] : node.attributes()) {
        if (!first) {
            oss << ",";
        }
        oss << "\"" << json_escape(attr) << "\":\"" << json_escape(val) << "\"";
        first = false;
    }

    oss << "}";

    // Serialize children
    oss << ",\"children\":[";
    bool first_child = true;
    for (const auto& child : node.children()) {
        if (!first_child) oss << ",";
        oss << serialize_node(*child);
        first_child = false;
    }
    oss << "]}";

    return oss.str();
}

// Simplified JSON parser for internal structure
struct JsonValue {
    enum class Type { OBJECT, STRING, ARRAY };
    Type type;
    std::string value;
    std::string obj_type;
    std::map<std::string, std::string> obj_attrs;
    std::vector<JsonValue> array;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& json) : json_(json), pos_(0) {}

    JsonValue parse_value() {
        skip_whitespace();
        if (pos_ >= json_.size()) throw std::runtime_error("EOF");

        if (json_[pos_] == '{') return parse_object();
        if (json_[pos_] == '[') return parse_array();
        if (json_[pos_] == '\"') return parse_string();
        throw std::runtime_error("Unexpected token");
    }

private:
    std::string json_;
    std::size_t pos_;

    void skip_whitespace() {
        while (pos_ < json_.size() && isspace(json_[pos_])) ++pos_;
    }

    void consume(char expected) {
        skip_whitespace();
        if (pos_ >= json_.size() || json_[pos_] != expected)
            throw std::runtime_error("Expected char");
        ++pos_;
    }

    JsonValue parse_string() {
        consume('\"');
        std::string s;
        while (pos_ < json_.size() && json_[pos_] != '\"') {
            if (json_[pos_] == '\\') {
                ++pos_;
                if (pos_ >= json_.size()) break;
                if (json_[pos_] == 'n') s += '\n';
                else if (json_[pos_] == 'r') s += '\r';
                else if (json_[pos_] == 't') s += '\t';
                else s += json_[pos_];
            } else {
                s += json_[pos_];
            }
            ++pos_;
        }
        consume('\"');
        JsonValue v; v.type = JsonValue::Type::STRING; v.value = s;
        return v;
    }

    JsonValue parse_array() {
        consume('[');
        JsonValue v; v.type = JsonValue::Type::ARRAY;
        skip_whitespace();
        while (pos_ < json_.size() && json_[pos_] != ']') {
            v.array.push_back(parse_value());
            skip_whitespace();
            if (pos_ < json_.size() && json_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
            }
        }
        consume(']');
        return v;
    }

    JsonValue parse_object() {
        consume('{');
        JsonValue v; v.type = JsonValue::Type::OBJECT;
        skip_whitespace();
        while (pos_ < json_.size() && json_[pos_] != '}') {
            std::string key = parse_string().value;
            consume(':');
            if (key == "type") {
                v.obj_type = parse_string().value;
            } else if (key == "attributes") {
                consume('{');
                skip_whitespace();
                while (pos_ < json_.size() && json_[pos_] != '}') {
                    std::string attr_key = parse_string().value;
                    consume(':');
                    std::string attr_val = parse_string().value;
                    v.obj_attrs[attr_key] = attr_val;
                    skip_whitespace();
                    if (pos_ < json_.size() && json_[pos_] == ',') {
                        ++pos_;
                        skip_whitespace();
                    }
                }
                consume('}');
            } else if (key == "children") {
                v.array = parse_array().array;
            } else {
                parse_value(); // skip
            }
            skip_whitespace();
            if (pos_ < json_.size() && json_[pos_] == ',') {
                ++pos_;
                skip_whitespace();
            }
        }
        consume('}');
        return v;
    }
};

std::shared_ptr<Node> deserialize_node(const JsonValue& v) {
    auto node = std::make_shared<Node>(v.obj_type);
    for (auto const& [key, val] : v.obj_attrs) {
        node->set_attribute(key, val);
    }
    for (auto const& child_v : v.array) {
        node->add_child(deserialize_node(child_v));
    }
    return node;
}

} // namespace

std::string Serializer::serialize(const Node& root) {
    return serialize_node(root);
}

std::shared_ptr<Node> Serializer::deserialize(const std::string& json) {
    JsonParser parser(json);
    return deserialize_node(parser.parse_value());
}
