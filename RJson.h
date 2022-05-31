#include <string>

namespace json {
	
	enum ValueType {
		NIL,
		FALSE,
		TRUE,
		NUMBER,
		STRING,
		ARRAY,
		OBJECT
	};

	struct Member;

	struct ValueData {
		ValueType type;
		union {
			struct { Member *m; size_t size; }object;
			struct { ValueData *v; size_t size; }array;
			struct { char *s; size_t len; }str;
			double number;
		};
	};

	struct Member {
		char *key;
		size_t klen;
		ValueData v;
	};

	enum class ParseType {
		OK,	
		EXPECT_VALUE,
		INVALID_VALUE,
		ROOT_NOT_SINGULAR,
		MISS_QUOTATION_MARK,
		INVALID_STRING_ESCAPE,
		MISS_COMMA_OR_SQUARE_BRACKET,
		MISS_KEY,
		MISS_COLON,
		MISS_COMMA_OR_CURLY_BRACKET
	};

	enum class StringifyType {
		OK
	};

	class Value {
		ValueData data;
		public:
			Value();
			Value(ValueData &v);
			void setNull();
			void setTrue();
			void setFalse();
			void setNumber(double number);
			void setObject(Member *m, size_t size);
			void setArray(ValueData *v, size_t size);
			void setString(const char *str, size_t len);
			void clearString();
			void destory();
			ValueData getValueData();
			double getNumber();
			size_t getArraySize();
			size_t getObjectSize();
			std::string getObjectKey(size_t index);
			size_t getObjectKeyLength(size_t index);
			Value getObjectElement(std::string key);
			Value getArrayElement(size_t index);
			std::string getString();
			ValueType getType();
			~Value();
	};

	class Parser {
		struct Context {
			const char *jsonPtr;
			char *stack;
			size_t size;
			size_t top;
		}context;
		private:
			void *contextPush(size_t size);
			void *contextPop(size_t size);
			ParseType parse(Value &value);
			ParseType parseValue(Value &value);
			void parseWhiteSpace();
			ParseType parseNULL(Value &value);
			ParseType parseTrue(Value &value);
			ParseType parseFalse(Value &value);
			ParseType parseNumber(Value &value);
			ParseType parseStringRaw(char **str, size_t &len);
			ParseType parseString(Value &value);
			ParseType parseArray(Value &value);
			ParseType parseObject(Value &value);
			StringifyType stringifyValue(Value &value);
		public:
			Parser(std::string json, Value &value);
			std::string stringify(Value &value);
			~Parser();
	};
}
