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

static String validSymbolChars =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";

#define STRING_IDENTIFIER '\''
#define ESCAPE '\\'

static List<Function*> syscalls;

enum ParseMode {
	UNKNOWN, SYMBOL, FUNCTION, STRING
};

Code* mish_compile(String code) {
	Code* compiledCode = new Code();

	Stack<ParseMode> parseMode = Stack<ParseMode>(UNKNOWN);
	Stack<List<Expression*>*> arguments;
	Stack<String> symbols = Stack<String>();
	uint64 symbolStart;
	List<char> string;
	bool escape = false;

	for (uint64 i = 0; i < strlen(code); i++) {
		char c = code[i];

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
				parseMode.pop();
				char* str = (char*) malloc(string.size() + 1);

				Iterator<char>* stringIterator = string.iterator();
				char strChar;
				uint64 strIndex = 0;
				while ((strChar = stringIterator->next()) != NULL) {
					str[strIndex] = strChar;
					strIndex++;
				}
				str[strIndex] = NULL; // null terminate
				delete stringIterator;
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
		} else if (arrayContains<char>((char*) validSymbolChars,
				strlen(validSymbolChars), c)) {
			if (parseMode.peek() != SYMBOL) {
				parseMode.push(SYMBOL);
				symbolStart = i;
			}
		} else if (c == '(') {
			if (parseMode.peek() != SYMBOL) { // open parentheses
				// TODO
			} else { // function symbolStart
				symbols.push(substring(code, symbolStart, i));
				arguments.push(new List<Expression*>());

				parseMode.pop(); // SYMBOL
				parseMode.push(FUNCTION);
			}
		} else if (c == ')') {
			if (parseMode.peek() == FUNCTION) {
				parseMode.pop(); // FUNCTION

				String symbol = symbols.pop();

				Bytecode* callBytecode;

				if (stringStartsWith(symbol, "__")) { // check if this is a syscall
					String syscallName = substring(symbol, 2, strlen(symbol));

					Iterator<Function*>* syscallIterator = syscalls.iterator();
					Function* function;
					while ((function = syscallIterator->next()) != NULL) {
						if (strequ(function->name, syscallName)) {
							break;
						}
					}
					delete syscallIterator;
					if (function == NULL) {
						crash("syscall not found");
					}

					callBytecode = (Bytecode*) new FunctionCallVoid(function,
							arguments.pop());
				} else {
					// function
					// TODO
				}
				compiledCode->bytecodes.add(callBytecode);
			} else { // close parentheses
					 // TODO
			}
		}
	}

	return compiledCode;
}

void mish_addSyscall(Function* syscall) {
	syscalls.add(syscall);
}

List<Function*> mish_getSyscalls() {
	return syscalls;
}
