/*
 * mish.cpp
 *
 *  Created on: May 3, 2016
 *      Author: chris
 */

#include <mish.h>
#include <log.h>
#include <string.h>
#include <array.h>
#include <functioncallreturn.h>
#include <functioncallvoid.h>
#include <stack.h>

List<Function*> mish_syscalls;

#define VALID_SYMBOL_CHARS L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"
#define WHITESPACE_CHARS L" \t\n"

enum ParseMode {
	BODY, SYMBOL, SYMBOL_READY, FUNCTION, EXPECT_ARGUMENT, STRING, PARENTHISIS
};

static bool isValidSymbolChar(wchar_t c) {
	return arrayContains<wchar_t>(VALID_SYMBOL_CHARS,
			strlen(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(wchar_t c) {
	return arrayContains<wchar_t>(WHITESPACE_CHARS, strlen(WHITESPACE_CHARS), c);
}

Code* mish_compile(String code) {
	return mish_compile(code, strlen(code));
}

Code* mish_compile(String code, size_t size) {
	Code* compiledCode = new Code();

	Stack<ParseMode> parseMode(BODY);
	Stack<List<Expression*>*> arguments;
	Stack<String> symbols;
	uint64 symbolStart = NULL;
	String symbol = NULL;
	List<wchar_t> string;
	bool escaping = false;

	for (uint64 i = 0; i < size; i++) {
		wchar_t c = code[i];

		write_serial((char) c);

		parseChar: switch (parseMode.peek()) {
		case BODY:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (isValidSymbolChar(c)) {
				symbolStart = i;
				parseMode.push(SYMBOL);
			}
			break;
		case SYMBOL:
			if (!isValidSymbolChar(c)) {
				symbol = substring(code, symbolStart, i);
				parseMode.pop();
				parseMode.push(SYMBOL_READY);
				goto parseChar;
			}
			break;
		case SYMBOL_READY:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (c == '(') {
				// begin function
				symbols.push(symbol);
				symbol = NULL;
				parseMode.pop();
				parseMode.push(FUNCTION);
				arguments.push(new List<Expression*>());
			} else {
				fault(L"unexpected character");
				return NULL;
			}
			break;
		case FUNCTION:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (c == ')') {
				String symbol = symbols.pop();

				if (stringStartsWith(symbol, L"__")) { // check if this is a syscall
					String syscallName = substring(symbol, 2, strlen(symbol));

					Iterator<Function*> syscallIterator =
							mish_syscalls.iterator();
					Function* function = NULL;
					bool found = false;
					while (syscallIterator.hasNext() && !found) {
						function = syscallIterator.next();
						if (strequ(function->name, syscallName)) {
							// check parameter sizes
							if (arguments.peek()->size()
									!= function->parameterTypes->size()) {
								continue;
							}

							// check parameter types
							Iterator<Expression*> argumentsIterator =
									arguments.peek()->iterator();
							Iterator<ValueType> parametersIterator =
									function->parameterTypes->iterator();
							while (argumentsIterator.hasNext()
									&& parametersIterator.hasNext()) {
								Expression* argument = argumentsIterator.next();
								ValueType parameter = parametersIterator.next();
								if (argument->valueType != parameter) {
									// incorrect function
									goto continueFunctionSearch;
								}
							}
							found = true;
							continueFunctionSearch: continue;
						}
					}
					delete syscallName;

					if (!found) {
						fault(L"syscall not found");
						return NULL;
					}

					if (parseMode.size() > 1
							&& (parseMode.peek(2) == EXPECT_ARGUMENT
									|| parseMode.peek(2) == FUNCTION)) {
						Expression* callExpression =
								(Expression*) new FunctionCallReturn(function,
										arguments.pop());
						arguments.peek()->add(callExpression);
					} else {
						Bytecode* callBytecode =
								(Bytecode*) new FunctionCallVoid(function,
										arguments.pop());
						compiledCode->bytecodes.add(callBytecode);
					}

					parseMode.pop();
				} else {
					// TODO regular function
				}
				delete symbol;

				break;
			} else if (c == ',') {
				parseMode.push(EXPECT_ARGUMENT);
			}
			// intentional fall-through
		case EXPECT_ARGUMENT:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (c == '\'') {
				if (parseMode.peek() == EXPECT_ARGUMENT) {
					parseMode.pop();
				}
				parseMode.push(STRING);
			} else if (isValidSymbolChar(c)) {
				symbolStart = i;
				parseMode.push(SYMBOL);
			} else {
				fault(L"unexpected argument");
				return NULL;
			}
			break;
		case STRING:
			if (escaping) {
				if (c == 'n') {
					string.add('\n');
				} else {
					string.add(c);
				}
				escaping = false;
			} else {
				if (c == '\\') {
					escaping = true;
				} else if (c == '\'') {
					wchar_t* str = (wchar_t*) create(string.size() * 2 + 1);

					Iterator<wchar_t> stringIterator = string.iterator();
					uint64 strIndex = 0;
					while (stringIterator.hasNext()) {
						str[strIndex] = stringIterator.next();
						strIndex++;
					}
					str[strIndex] = NULL; // null terminate
					string.clear();

					arguments.peek()->add((Expression*) new StringValue(str));

					parseMode.pop();
				} else {
					string.add(c);
				}
			}
			break;
		}

		/*if (parseMode.peek() == SYMBOL && !isValidSymbolChar(c)) {
		 symbol = substring(code, symbolStart, i);
		 assertPop(&parseMode, SYMBOL);
		 parseMode.push(SYMBOL_READY);
		 }

		 if (c == STRING_IDENTIFIER || parseMode.peek() == STRING) {
		 if (c == ESCAPE_CHAR) {
		 // escaping
		 escaping = true;
		 continue;
		 }

		 if (parseMode.peek() == STRING && c == STRING_IDENTIFIER
		 && !escaping) {
		 // end string
		 assertPop(&parseMode, STRING);
		 wchar_t* str = (wchar_t*) create(string.size() * 2 + 1);

		 Iterator<wchar_t> stringIterator = string.iterator();
		 uint64 strIndex = 0;
		 while (stringIterator.hasNext()) {
		 str[strIndex] = stringIterator.next();
		 strIndex++;
		 }
		 str[strIndex] = NULL; // null terminate
		 string.clear();

		 arguments.peek()->add((Expression*) new StringValue(str));
		 } else if (c == STRING_IDENTIFIER && !escaping) {
		 log(L"start string");
		 // start string
		 parseMode.push(STRING); // begin stringBuffer
		 symbolStart = i + 1;
		 } else {
		 // add to string
		 string.add(c);
		 if (escaping) {
		 // end escaping
		 escaping = false;
		 }
		 }
		 } else if (isValidSymbolChar(c)) {
		 if (parseMode.peek() != SYMBOL) {
		 parseMode.push(SYMBOL);
		 log(L"symbol mode");
		 symbolStart = i;
		 }
		 } else if (parseMode.peek() == FUNCTION && c == ARGUMENT_DELIMETER) {
		 parseMode.push(EXPECT_ARGUMENT);
		 } else if (c == '(') {
		 if (parseMode.peek() == SYMBOL_READY) {
		 // function symbolStart
		 symbols.push(symbol);
		 symbol = NULL;
		 arguments.push(new List<Expression*>());

		 assertPop(&parseMode, SYMBOL_READY);
		 parseMode.push(FUNCTION);
		 } else if (parseMode.peek() == EXPECT_ARGUMENT) { // open parentheses
		 crash(L"parenthesis not implemented yet");
		 } else {
		 crash(L"parenthesis not expected");
		 }
		 } else if (c == ')') {
		 if (parseMode.peek() == FUNCTION) {
		 assertPop(&parseMode, FUNCTION);

		 String symbol = symbols.pop();

		 Bytecode* callBytecode;

		 if (stringStartsWith(symbol, L"__")) { // check if this is a syscall
		 String syscallName = substring(symbol, 2, strlen(symbol));

		 Iterator<Function*> syscallIterator =
		 mish_syscalls.iterator();
		 Function* function;
		 bool found = false;
		 while (syscallIterator.hasNext() && !found) {
		 function = syscallIterator.next();
		 if (strequ(function->name, syscallName)) {
		 // check parameter sizes
		 if (arguments.peek()->size()
		 != function->parameterTypes->size()) {
		 continue;
		 }

		 // check parameter types
		 Iterator<Expression*> argumentsIterator =
		 arguments.peek()->iterator();
		 Iterator<ValueType> parametersIterator =
		 function->parameterTypes->iterator();
		 while (argumentsIterator.hasNext()
		 && parametersIterator.hasNext()) {
		 Expression* argument = argumentsIterator.next();
		 ValueType parameter = parametersIterator.next();
		 if (argument->valueType != parameter) {
		 // incorrect function
		 goto continueFunctionSearch;
		 }
		 }
		 found = true;
		 continueFunctionSearch: continue;
		 }
		 }
		 delete syscallName;

		 if (!found) {
		 crash(L"syscall not found");
		 }

		 callBytecode = (Bytecode*) new FunctionCallVoid(function,
		 arguments.pop());
		 } else {
		 // function
		 // TODO
		 }
		 delete symbol;
		 compiledCode->bytecodes.add(callBytecode);
		 } else if (parseMode.peek() == PARENTHISIS) {
		 crash(L"parenthesis not implemented yet");
		 } else {
		 crash(L"argument expected");
		 }
		 } else if (c != ' ' && c != '\t') {
		 crash(L"expected whitespace");
		 }*/
	}

	if (parseMode.peek() != BODY) {
		debug(L"parse mode", parseMode.pop());
		fault(L"incorrect parse mode");
		return NULL;
	}

	return compiledCode;
}
