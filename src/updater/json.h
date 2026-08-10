// 极简 JSON 解析器（仅满足 update.json 解析需求，不依赖第三方库）
// 支持 object / array / string / number / bool / null，足够提取 version/url/sha256/size 等字段。
#pragma once
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <memory>
#include <stdexcept>

namespace updater {

struct JsonValue {
    enum Type { Null, Bool, Num, Str, Obj, Arr } type = Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::map<std::string, std::unique_ptr<JsonValue>> obj;
    std::vector<std::unique_ptr<JsonValue>> arr;

    const JsonValue* Find(std::string_view key) const {
        if (type != Obj) return nullptr;
        auto it = obj.find(std::string(key));
        return it == obj.end() ? nullptr : it->second.get();
    }
};

class JsonParser {
public:
    JsonParser(std::string_view s) : s_(s) {}

    std::unique_ptr<JsonValue> Parse() {
        skipWs();
        // 跳过 UTF-8 BOM（PowerShell Set-Content -Encoding utf8 等会写 BOM）
        if (i_ + 2 < s_.size() && (unsigned char)s_[i_] == 0xEF &&
            (unsigned char)s_[i_ + 1] == 0xBB && (unsigned char)s_[i_ + 2] == 0xBF) {
            i_ += 3;
            skipWs();
        }
        auto v = parseValue();
        return v;
    }

private:
    std::string_view s_;
    size_t i_ = 0;

    void skipWs() {
        while (i_ < s_.size() && (s_[i_] == ' ' || s_[i_] == '\t' || s_[i_] == '\n' || s_[i_] == '\r')) i_++;
    }

    char peek() { return i_ < s_.size() ? s_[i_] : '\0'; }

    std::unique_ptr<JsonValue> parseValue() {
        skipWs();
        if (i_ >= s_.size()) return nullptr;
        char c = s_[i_];
        if (c == '{') return parseObj();
        if (c == '[') return parseArr();
        if (c == '"') { auto v = std::make_unique<JsonValue>(); v->type = JsonValue::Str; v->str = parseStr(); return v; }
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') { i_ += 4; auto v = std::make_unique<JsonValue>(); v->type = JsonValue::Null; return v; }
        return parseNum();
    }

    std::string parseStr() {
        std::string out;
        i_++; // skip "
        while (i_ < s_.size() && s_[i_] != '"') {
            if (s_[i_] == '\\' && i_ + 1 < s_.size()) {
                char e = s_[i_ + 1];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'u': {
                        if (i_ + 5 < s_.size()) {
                            // 简化：跳过 \uXXXX，仅按 ASCII 处理（update.json 无非 ASCII）
                            unsigned cp = 0;
                            for (int k = 0; k < 4; k++) {
                                char h = s_[i_ + 2 + k];
                                cp <<= 4;
                                if (h >= '0' && h <= '9') cp |= h - '0';
                                else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                                else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                            }
                            if (cp < 0x80) out += (char)cp;
                            i_ += 4;
                        }
                        break;
                    }
                    default: out += e; break;
                }
                i_ += 2;
            } else {
                out += s_[i_++];
            }
        }
        if (i_ < s_.size()) i_++; // skip closing "
        return out;
    }

    std::unique_ptr<JsonValue> parseNum() {
        size_t start = i_;
        while (i_ < s_.size() && (s_[i_] == '-' || s_[i_] == '+' || s_[i_] == '.' ||
               (s_[i_] >= '0' && s_[i_] <= '9') || s_[i_] == 'e' || s_[i_] == 'E')) i_++;
        auto v = std::make_unique<JsonValue>();
        v->type = JsonValue::Num;
        v->num = std::stod(std::string(s_.substr(start, i_ - start)));
        return v;
    }

    std::unique_ptr<JsonValue> parseBool() {
        auto v = std::make_unique<JsonValue>();
        v->type = JsonValue::Bool;
        if (s_[i_] == 't') { v->b = true; i_ += 4; }
        else { v->b = false; i_ += 5; }
        return v;
    }

    std::unique_ptr<JsonValue> parseObj() {
        auto v = std::make_unique<JsonValue>();
        v->type = JsonValue::Obj;
        i_++; // {
        skipWs();
        if (peek() == '}') { i_++; return v; }
        while (i_ < s_.size()) {
            skipWs();
            std::string key = parseStr();
            skipWs();
            if (peek() == ':') i_++;
            auto val = parseValue();
            if (val) v->obj[key] = std::move(val);
            skipWs();
            if (peek() == ',') { i_++; continue; }
            if (peek() == '}') { i_++; break; }
            break;
        }
        return v;
    }

    std::unique_ptr<JsonValue> parseArr() {
        auto v = std::make_unique<JsonValue>();
        v->type = JsonValue::Arr;
        i_++; // [
        skipWs();
        if (peek() == ']') { i_++; return v; }
        while (i_ < s_.size()) {
            auto val = parseValue();
            if (val) v->arr.push_back(std::move(val));
            skipWs();
            if (peek() == ',') { i_++; continue; }
            if (peek() == ']') { i_++; break; }
            break;
        }
        return v;
    }
};

} // namespace updater
