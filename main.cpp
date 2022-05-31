#include "RJson.h"
#include <iostream>
#include <string>

int main() {
	std::string test = "{\"hello\" : {\"aa\" : [1, 2, 3]}, \"gg\" : [2,2,3]}";
	json::Value value;
	json::Parser parser(test, value);
	std::cout << "json : " << parser.stringify(value) << std::endl;
	std::cout << "gg : " << value.getObjectElement("gg").getArrayElement(0).getNumber();
	value.destory();
	return 0;
}
