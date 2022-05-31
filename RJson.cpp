#include "RJson.h"
#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <memory.h>

namespace json {

	static void clearValueData(ValueData &data) {
		if (data.type == ValueType::STRING) {
			if (data.str.s != nullptr && data.str.len > 0) {
				free(data.str.s);
				data.str.len = 0;
				data.str.s = nullptr;
			}
		} else if (data.type == ValueType::ARRAY) {
			for (size_t i = 0; i < data.array.size; i++) {
				clearValueData(data.array.v[i]);
			}
			if (data.array.v != nullptr && data.array.size > 0) {
				free(data.array.v);
				data.array.v = nullptr;
				data.array.size = 0;
			}
		} else if (data.type == ValueType::OBJECT) {
			for (size_t i = 0; i < data.object.size; i++) {
				if (data.object.m[i].key != nullptr && data.object.m[i].klen > 0) {
					free(data.object.m[i].key);
					data.object.m[i].key = nullptr;
					data.object.m[i].klen = 0;
				}
				clearValueData(data.object.m[i].v);
			}
			if (data.object.m != nullptr && data.object.size > 0) {
				free(data.object.m);
				data.object.m = nullptr;
				data.object.size = 0;
			}
		}
	}

	Value::Value() {
		data.type = ValueType::NIL;

		data.str.s = nullptr;
		data.str.len = 0;

		data.array.v = nullptr;
		data.array.size = 0;

		data.object.m = nullptr;
		data.object.size = 0;
	}	

	Value::Value(ValueData &v) {
		data = v;
	}

	void Value::setNull() {
		data.type = ValueType::NIL;
	}

	void Value::setFalse() {
		data.type = ValueType::FALSE;
	}

	void Value::setTrue() {
		data.type = ValueType::TRUE;
	}

	void Value::setNumber(double number) {
		data.type = ValueType::NUMBER;
		data.number = number;
	}

	void Value::setObject(Member *m, size_t size) {
		data.type = ValueType::OBJECT;
		if (m == nullptr)
			data.object.m = m;
		else
			data.object.m = (Member *)memcpy(malloc(size * sizeof(Member)), m, size * sizeof(Member));
		data.object.size = size;
	}

	void Value::setArray(ValueData *v, size_t size) {
		data.type = ValueType::ARRAY;
		if (v == nullptr) 
			data.array.v = nullptr;
		else
			data.array.v = (ValueData *)memcpy(malloc(size * sizeof(ValueData)), v, size * sizeof(ValueData));
		data.array.size = size;
	}

	void Value::setString(const char *str, size_t len) {
		assert(str != nullptr || len == 0);
		clearString();
		data.str.s = (char *)malloc(len + 1);	
		memcpy(data.str.s, str, len);
		data.str.s[len] = 0;
		data.str.len = len;
		data.type = ValueType::STRING;
	}

	void Value::clearString() {
		if (data.type == ValueType::STRING && data.str.s != nullptr && data.str.len > 0) {
			free(data.str.s);
			data.str.len = 0;
			data.str.s = nullptr;
		}
		setNull();
	}

	double Value::getNumber() {
		assert(data.type == ValueType::NUMBER);
		return data.number;
	}

	size_t Value::getObjectSize() {
		assert(data.type == ValueType::OBJECT);	
		return data.object.size;
	}

	std::string Value::getObjectKey(size_t index) {
		assert(data.type == ValueType::OBJECT && data.object.m != nullptr);	
		assert(index < data.object.size);
		std::string key = data.object.m[index].key;
		return key;
	}

	size_t Value::getObjectKeyLength(size_t index) {
		assert(data.type == ValueType::OBJECT);	
		assert(index < data.object.size);
		return data.object.m[index].klen;
	}

	Value Value::getObjectElement(std::string key) {
		Value nullValue;

		assert(data.type == ValueType::OBJECT && data.object.m != nullptr);	
		size_t size = getObjectSize();
		for (size_t i = 0; i < size; i++) {
			if (data.object.m[i].key == key) {
				return data.object.m[i].v;
			}
		}

		return nullValue;
	}

	size_t Value::getArraySize() {
		assert(data.type == ValueType::ARRAY);	
		return data.array.size;
	}

	ValueData Value::getValueData() {
		return data;	
	}

	Value Value::getArrayElement(size_t index) {
		assert(data.type == ValueType::ARRAY && data.array.v != nullptr);	
		assert(index < data.array.size);
		return data.array.v[index];	
	}

	std::string Value::getString() {
		assert(data.type == ValueType::STRING);
		return std::string(data.str.s);
	}

	ValueType Value::getType() {
		return data.type;
	}

	void Value::destory() {
		clearValueData(data);
	}

	Value::~Value() {
	}

	void *Parser::contextPush(size_t size) {
		assert(size > 0);
		void *ret;
		if (context.top + size >= context.size) {
			if (context.size == 0) {
				context.size = 256;
			}
			while (context.top + size >= context.size) {
				context.size += context.size >> 1;
			}
			context.stack = (char *)realloc(context.stack, context.size);
			for (size_t i = 0; i < context.size; i++) {
				context.stack[i] = 0;
			}
		}
		ret = context.stack + context.top;
		context.top += size;
		return ret;
	}

	void *Parser::contextPop(size_t size) {
		assert(context.top >= size);
		return context.stack + (context.top -= size);
	}

	Parser::Parser(std::string json, Value &value) {
		context.jsonPtr = json.c_str();
		context.stack = nullptr;
		context.size = context.top = 0;
		parse(value);
	}

	ParseType Parser::parse(Value &value) {
		ParseType ret;
		parseWhiteSpace();	
		ret = parseValue(value);
		if (ret == ParseType::OK) {
			parseWhiteSpace();
			if (context.jsonPtr[0] != 0) {
				value.setNull();
				ret = ParseType::ROOT_NOT_SINGULAR;
			}
		}
		assert(context.top == 0);
		free(context.stack);
		context.size = 0;
		return ret;
	}

	void Parser::parseWhiteSpace() {
		const char *p = context.jsonPtr;	
		while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') {
			p++;
		}
		context.jsonPtr = p;
	}

	ParseType Parser::parseValue(Value &value) {
		switch (context.jsonPtr[0]) {
			case '{': return parseObject(value);	
			case '[': return parseArray(value);	
			case '\"': return parseString(value);	
			case 'n': return parseNULL(value);	
			case 't': return parseTrue(value);	
			case 'f': return parseFalse(value);	
			case 0: return ParseType::EXPECT_VALUE;
			default: return parseNumber(value);
		}
	}

	ParseType Parser::parseNULL(Value &value) {
		if (context.jsonPtr[1] != 'u' || context.jsonPtr[2] != 'l' || context.jsonPtr[3] != 'l') {
			return ParseType::INVALID_VALUE;
		}
		value.setNull();
		context.jsonPtr += 4;
		return ParseType::OK;
	}

	ParseType Parser::parseTrue(Value &value) {
		if (context.jsonPtr[1] != 'r' || context.jsonPtr[2] != 'u' || context.jsonPtr[3] != 'e') {
			return ParseType::INVALID_VALUE;
		}
		value.setTrue();
		context.jsonPtr += 4;
		return ParseType::OK;
	}

	ParseType Parser::parseFalse(Value &value) {
		if (context.jsonPtr[1] != 'a' || context.jsonPtr[2] != 'l' || context.jsonPtr[3] != 's' || context.jsonPtr[4] != 'e') {
			return ParseType::INVALID_VALUE;
		}
		value.setFalse();
		context.jsonPtr += 5;
		return ParseType::OK;
	}

	ParseType Parser::parseNumber(Value &value) {
		const char *p = context.jsonPtr;
		if (*p == '-') p++;
		if (*p == '0') p++;
		else {
			if (!(*p >= '1' && *p <= '9')) return ParseType::INVALID_VALUE;
			for (p++; (*p >= '0' && *p <= '9'); p++);
		}
		if (*p == '.') {
			p++;
			if (!(*p >= '0' && *p <= '9')) return ParseType::INVALID_VALUE;
			for (p++; (*p >= '0' && *p <= '9'); p++);
		}
		if (*p == 'e' || *p == 'E') {
			p++;
			if (*p == '+' || *p == '-') p++;
			if (!(*p >= '0' && *p <= '9')) return ParseType::INVALID_VALUE;
			for (p++; (*p >= '0' && *p <= '9'); p++);
		}
		value.setNumber(strtod(context.jsonPtr, nullptr));
		context.jsonPtr = p;

		return ParseType::OK;
	}

	ParseType Parser::parseStringRaw(char **str, size_t &len) {
		size_t head = context.top;	
		context.jsonPtr++;
		const char *p = context.jsonPtr;
		char *stack = nullptr;
		while (true) {
			char ch = *(p++);
			switch (ch) {
				case '\\':
					switch (*(p++)) {
						case '\"':
							*(char *)contextPush(sizeof(char)) = '\"';
							break;
						case '\\':
							*(char *)contextPush(sizeof(char)) = '\\';
							break;
						case '/':
							*(char *)contextPush(sizeof(char)) = '/';
							break;
						case 'b':
							*(char *)contextPush(sizeof(char)) = '\b';
							break;
						case 'f':
							*(char *)contextPush(sizeof(char)) = '\f';
							break;
						case 'n':
							*(char *)contextPush(sizeof(char)) = '\n';
							break;
						case 'r':
							*(char *)contextPush(sizeof(char)) = '\r';
							break;
						case 't':
							*(char *)contextPush(sizeof(char)) = '\t';
							break;
						default:
							context.top = head;
							return ParseType::INVALID_STRING_ESCAPE;
					}
					break;
				case '\"':
					len = context.top - head;
					*str = (char *)malloc(sizeof(char) * len);
					stack = (char *)contextPop(len);
					memcpy(*str, stack, len * sizeof(char));
					context.jsonPtr = p;
					return ParseType::OK;
				case 0:
					context.top = head;
					return ParseType::MISS_QUOTATION_MARK;
				default:
					*(char *)contextPush(sizeof(char)) = ch;
					break;
			}	
		}

	}

	ParseType Parser::parseString(Value &value) {
		ParseType ret;
		size_t len = 0;
		char *s;

		if ((ret = parseStringRaw(&s, len)) == ParseType::OK) {
			value.setString(s, len);
		}
		free(s);
		return ret;
	}

	ParseType Parser::parseArray(Value &value) {
		size_t size = 0;
		ParseType ret;
		context.jsonPtr++;
		parseWhiteSpace();
		if (context.jsonPtr[0] == ']') {
			context.jsonPtr++;
			value.setArray(nullptr, 0);
			return ParseType::OK;
		}
		while (true) {
			Value tempValue;
			ValueData tempValueData;
			if ((ret = parseValue(tempValue)) != ParseType::OK) {
				break;
			}
			tempValueData = tempValue.getValueData();
			memcpy(contextPush(sizeof(ValueData)), &(tempValueData), sizeof(ValueData));
			size++;
			parseWhiteSpace();
			if (context.jsonPtr[0] == ',') {
				context.jsonPtr++;
				parseWhiteSpace();
			} else if (context.jsonPtr[0] == ']') {
				context.jsonPtr++;
				ValueData *temp = (ValueData *)contextPop(size * sizeof(ValueData));
				value.setArray(temp, size);
				ret = ParseType::OK;
				break;
			} else {
				ret = ParseType::MISS_COMMA_OR_SQUARE_BRACKET;
				break;
			}
		}
		return ret;
	}

	ParseType Parser::parseObject(Value &value) {
		Member member;		
		size_t size = 0;
		ParseType ret;
		context.jsonPtr++;
		parseWhiteSpace();
		if (context.jsonPtr[0] == '}') {
			context.jsonPtr++;
			value.setObject(nullptr, 0);
			return ParseType::OK;
		}

		member.key = nullptr;
		size = 0;

		while (true) {
			char *str;
			Value tempValue;
			if (context.jsonPtr[0] != '\"') {
				ret = ParseType::MISS_KEY;
				break;
			}
			if ((ret = parseStringRaw(&str, member.klen)) != ParseType::OK) {
				break;
			}
			memcpy(member.key = (char *)malloc(member.klen + 1), str, member.klen);
			member.key[member.klen] = 0;
			free(str);
			parseWhiteSpace();
			if (context.jsonPtr[0] != ':') {
				ret = ParseType::MISS_COLON;	
				break;
			}
			context.jsonPtr++;
			parseWhiteSpace();
			if ((ret = parseValue(tempValue)) != ParseType::OK) {
				break;
			}
			member.v = tempValue.getValueData();
			memcpy((Member *)contextPush(sizeof(Member)), &member, sizeof(Member));
			size++;
			member.key = nullptr;
			parseWhiteSpace();
			if (context.jsonPtr[0] == ',') {
				context.jsonPtr++;
				parseWhiteSpace();
			} else if (context.jsonPtr[0] == '}') {
				context.jsonPtr++;
				Member *temp = (Member *)malloc(sizeof(Member) * size);
				Member *stack = (Member *)contextPop(sizeof(member) * size);
				memcpy(temp, stack, sizeof(Member) * size);
				value.setObject(temp, size);
				free(temp);
				ret = ParseType::OK;
				break;
			} else {
				ret = ParseType::MISS_COMMA_OR_CURLY_BRACKET;
				break;
			}
		}
		return ret;
	}

	std::string Parser::stringify(Value &value) {
		std::string ret = "";
		assert(context.jsonPtr != nullptr);
		context.stack = nullptr;
		context.top = context.size = 0;
		if (stringifyValue(value) != StringifyType::OK) {
			return "";
		}
		ret.append(context.stack);
		free(context.stack);
		context.size = 0;
		return ret;
	}

	StringifyType Parser::stringifyValue(Value &value) {
		switch (value.getType()) {
			case ValueType::NIL:
				memcpy((char *)contextPush(4), "null", 4);
				break;
			case ValueType::TRUE:
				memcpy((char *)contextPush(4), "true", 4);
				break;
			case ValueType::FALSE:
				memcpy((char *)contextPush(5), "false", 5);
				break;
			case ValueType::NUMBER:
				{
					char buffer[32] = { 0 };
					int length = sprintf(buffer, "%.17g", value.getNumber());
					memcpy((char *)contextPush(length), buffer, length);
				}
				break;
			case ValueType::STRING:
				memcpy((char *)contextPush(value.getString().length()), value.getString().c_str(), value.getString().length());
				break;
			case ValueType::ARRAY:
				{
					memcpy((char *)contextPush(1), "[", 1);
					size_t size = value.getArraySize();
					for (size_t i = 0; i < size; i++) {
						Value temp = value.getArrayElement(i);
						stringifyValue(temp);
						if (i + 1 < size)
							memcpy((char *)contextPush(1), ",", 1);
					}
					memcpy((char *)contextPush(1), "]", 1);
				}
				break;
			case ValueType::OBJECT:
				{
					*(char *)contextPush(1) = '{';
					size_t size = value.getObjectSize();
					for (size_t i = 0; i < size; i++) {
						std::string key = value.getObjectKey(i);
						Value temp = value.getObjectElement(key);
						memcpy((char *)contextPush(1), "\"", 1);
						memcpy((char *)contextPush(key.length()), key.c_str(), key.length());
						memcpy((char *)contextPush(2), "\":", 2);
						stringifyValue(temp);
						if (i + 1 < size)
							*(char *)contextPush(1) = ',';
					}
					*(char *)contextPush(1) = '}';
				}
				break;
			default:
				break;
		}

		return StringifyType::OK;
	}

	Parser::~Parser() {
	}
};
