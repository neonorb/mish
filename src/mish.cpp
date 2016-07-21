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

#define VALID_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"

#define STRING_IDENTIFIER '\''
#define ESCAPE '\\'

static List<Function*> syscalls;

enum ParseMode {
	READY, SYMBOL, SYMBOL_READY, FUNCTION, STRING
};

static void assertPop(Stack<ParseMode>* stack, ParseMode expect) {
	ParseMode popped = stack->pop();
	if (popped != expect) {
		debug("expected", expect);
		debug("got", popped);
		crash("incorrect parse mode pop");
	}
}

static bool isValidSymbolChar(char c) {
	return arrayContains<char>(VALID_SYMBOL_CHARS,
			strlen(VALID_SYMBOL_CHARS), c);
}

Code* mish_compile(String code) {
	return mish_compile(code, (void*) strlen(code));
}

Code* mish_compile(String code, void* end) {
	Code* compiledCode = new Code();

	Stack<ParseMode> parseMode(READY);
	Stack<List<Expression*>*> arguments;
	Stack<String> symbols;
	uint64 symbolStart;
	String symbol;
	List<char> string;
	bool escape = false;

	for (uint64 i = 0; i < (uint64) end - (uint64) code; i++) {
		char c = code[i];
		write_serial(c);

		if (parseMode.peek() == SYMBOL && !isValidSymbolChar(c)) {
			symbol = substring(code, symbolStart, i);
			assertPop(&parseMode, SYMBOL);
			parseMode.push(SYMBOL_READY);
		}

		if (c == STRING_IDENTIFIER || parseMode.peek() == STRING) {
			if (escape) {
				// not escaping
				escape = false;
			} else if (c == ESCAPE) {
				// escaping
				escape = true;
				continue;
			}

			if (parseMode.peek() == STRING && c == STRING_IDENTIFIER) {
				// end string
				assertPop(&parseMode, STRING);
				char* str = (char*) create(string.size() + 1);

				Iterator<char> stringIterator = string.iterator();
				char strChar;
				uint64 strIndex = 0;
				while ((strChar = stringIterator.next()) != NULL) {
					str[strIndex] = strChar;
					strIndex++;
				}
				str[strIndex] = NULL; // null terminate
				string.clear();

				arguments.peek()->add((Expression*) new StringValue(str));
			} else if (c == STRING_IDENTIFIER) {
				// start string
				parseMode.push(STRING); // begin stringBuffer
				symbolStart = i + 1;
			} else {
				// add to string
				string.add(c);
			}
		} else if (isValidSymbolChar(c)) {
			if (parseMode.peek() != SYMBOL) {
				parseMode.push(SYMBOL);
				symbolStart = i;
			}
		} else if (c == '(') {
			if (parseMode.peek() == SYMBOL_READY) {
				// function symbolStart
				symbols.push(symbol);
				symbol = NULL;
				arguments.push(new List<Expression*>());

				assertPop(&parseMode, SYMBOL_READY);
				parseMode.push(FUNCTION);
			} else { // open parentheses
				// TODO
			}
		} else if (c == ')') {
			if (parseMode.peek() == FUNCTION) {
				assertPop(&parseMode, FUNCTION);

				String symbol = symbols.pop();

				Bytecode* callBytecode;

				if (stringStartsWith(symbol, "__")) { // check if this is a syscall
					String syscallName = substring(symbol, 2, strlen(symbol));

					Iterator<Function*> syscallIterator = syscalls.iterator();
					Function* function;
					while ((function = syscallIterator.next()) != NULL) {
						if (strequ(function->name, syscallName)) {
							break;
						}
					}
					delete syscallName;

					if (function == NULL) {
						crash("syscall not found");
					}

					callBytecode = (Bytecode*) new FunctionCallVoid(function,
							arguments.pop());
				} else {
					// function
					// TODO
				}
				delete symbol;
				compiledCode->bytecodes.add(callBytecode);
			} else { // close parentheses
					 // TODO
			}
		}
	}

	assertPop(&parseMode, READY);

	return compiledCode;
}

void mish_addSyscall(Function* syscall) {
	syscalls.add(syscall);
}

List<Function*> mish_getSyscalls() {
	return syscalls;
}
