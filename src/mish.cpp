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

static List<Syscall*> syscalls;

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
		write_serial(c);
		debug("parseMode", parseMode.peek());

		if (c == STRING_IDENTIFIER || parseMode.peek() == STRING) {
			if (escape) {
				escape = false;
				continue;
			} else if (c == ESCAPE) {
				escape = true;
				continue;
			} else if (parseMode.peek() == STRING) { // end stringBuffer
				write_serial('>');
				parseMode.pop();
				char* str = (char*) malloc(string.size() + 1);

				Iterator<char>* stringIterator = string.iterator();
				char strChar;
				uint64 strIndex;
				while ((strChar = stringIterator->next()) != NULL) {
					str[i] = strChar;
					string.remove((unsigned long long int) 0);
					strIndex++;
				}
				str[i] = 0; // null terminate

				arguments.peek()->add((Expression*) new StringValue(str));

				continue;
			} else if (c == STRING_IDENTIFIER) {
				write_serial('<');
				parseMode.push(STRING); // begin stringBuffer
				debug("pm", parseMode.peek());
				symbolStart = i + 1;
				continue;
			}

			string.add(c);
			write_serial(c);
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

				write_serial('s');
				parseMode.pop(); // SYMBOL
				parseMode.push(FUNCTION);
			}
		} else if (c == ')') {
			if (parseMode.peek() == FUNCTION) {
				write_serial('f');
				parseMode.pop(); // FUNCTION

				String symbol = symbols.pop();

				Bytecode* callBytecode;

				if (stringStartsWith(symbol, "__")) { // check if this is a syscall
					String syscallName = substring(symbol, 2, strlen(symbol));

					Iterator<Syscall*>* syscallIterator = syscalls.iterator();
					Syscall* syscall;
					Function* function;
					while ((syscall = syscallIterator->next()) != NULL) {
						if (strequ(syscall->name, syscallName)) {
							function = (Function*) syscall;
						}
					}
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

void mish_addSyscall(Syscall* syscall) {
	syscalls.add(syscall);
}

List<Syscall*> mish_getSyscalls() {
	return syscalls;
}
