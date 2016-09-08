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
	BODY,
	SYMBOL,
	SYMBOL_READY,
	FUNCTION,
	EXPECT_ARGUMENT,
	STRING,
	PARENTHISIS,
	COMMENT
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
	uint64 lineBeginning = 0;

	uint64 i = 0;
	String errorMessage = NULL;
	for (; i < size && errorMessage == NULL; i++) {
		wchar_t c = code[i];

		//write_serial((char) c);

		if (c == '\n') {
			lineBeginning = i;
		}

		parseChar: switch (parseMode.peek()) {
		case BODY:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (isValidSymbolChar(c)) {
				// begin a symbol
				symbolStart = i;
				parseMode.push(SYMBOL);
			}

			if (c == '#') {
				parseMode.push(COMMENT);
			}
			break;
		case SYMBOL:
			if (!isValidSymbolChar(c)) {
				// end the symbol because this isn't a valid symbol char
				symbol = substring(code, symbolStart, i);
				parseMode.pop();
				parseMode.push(SYMBOL_READY);
				goto parseChar;
				// we havn't done anything with this char, so we have to re-parse it
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
				errorMessage = L"unexpected character";
				break;
			}
			break;
		case FUNCTION:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			if (c == ')') {
				String symbol = symbols.pop();

				// determine if this is a syscall
				List<Function*> functions;
				if (stringStartsWith(symbol, L"__")) {
					functions = mish_syscalls;
				} else {
					// TODO regular function
				}

				// search for the function
				Iterator<Function*> functionsIterator = functions.iterator();
				Function* function = NULL;
				bool found = false;
				while (functionsIterator.hasNext() && !found) {
					function = functionsIterator.next();
					// check function name
					if (strequ(function->name, symbol)) {
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
								// we can't do labeled continues, this is a #DumbC workaround
							}
						}
						found = true;
						continueFunctionSearch: continue;
					}
				}
				delete symbol;

				if (!found) {
					errorMessage = L"syscall not found";
					break;
				}

				// add the function call to the bytecodes
				if (parseMode.size() > 1
						&& (parseMode.peek(2) == EXPECT_ARGUMENT
								|| parseMode.peek(2) == FUNCTION)) {
					Expression* callExpression =
							(Expression*) new FunctionCallReturn(function,
									arguments.pop());
					arguments.peek()->add(callExpression);
				} else {
					Bytecode* callBytecode = (Bytecode*) new FunctionCallVoid(
							function, arguments.pop());
					compiledCode->bytecodes.add(callBytecode);
				}

				parseMode.pop();

				break;
			} else if (c == ',') {
				// after a comma, there must be an argument
				parseMode.push(EXPECT_ARGUMENT);
				break;
			}
			// intentional fall-through
		case EXPECT_ARGUMENT:
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			// check if we are starting an expression
			if (c == '\'') {
				if (parseMode.peek() == EXPECT_ARGUMENT) {
					parseMode.pop();
				}
				// begin string
				parseMode.push(STRING);
			} else if (isValidSymbolChar(c)) {
				if (parseMode.peek() == EXPECT_ARGUMENT) {
					parseMode.pop();
				}
				// begin symbol
				symbolStart = i;
				parseMode.push(SYMBOL);
			} else {
				// wtf is this character?
				errorMessage = L"unexpected argument";
				break;
			}
			break;
		case STRING:
			if (escaping) {
				if (c == 'n') {
					// new line escape sequence
					string.add('\n');
				} else if (c == '\\') {
					// backslash escape sequence
					string.add('\\');
				} else if (c == '\'') {
					// quote escape sequence
					string.add('\'');
				} else {
					errorMessage = L"unrecognized escape sequence";
					break;
				}

				// done with escape
				escaping = false;
			} else {
				if (c == '\\') {
					// begin escape
					escaping = true;
				} else if (c == '\'') {
					// end string
					wchar_t* str = (wchar_t*) create(string.size() * 2 + 1);

					// copy the list of characters to an actual string
					Iterator<wchar_t> stringIterator = string.iterator();
					uint64 strIndex = 0;
					while (stringIterator.hasNext()) {
						str[strIndex] = stringIterator.next();
						strIndex++;
					}
					str[strIndex] = NULL; // null terminate
					string.clear();

					// add the string to the arguments
					arguments.peek()->add((Expression*) new StringValue(str));
					parseMode.pop();
				} else {
					// just another character
					string.add(c);
				}
			}
			break;
		case COMMENT:
			if (c == '\n') {
				parseMode.pop();
			}
		}
	}

	ParseMode endParseMode = parseMode.peek();
	if (errorMessage == NULL && endParseMode != BODY
			&& endParseMode != COMMENT) {
		// something wasn't properly closed, throw a generic error for now
		debug(L"parse mode", parseMode.pop());
		errorMessage = L"incorrect parse mode";
	}

	if (errorMessage) {
		// generate the error message and display it
		String line = substring(code, lineBeginning, i);
		fault(line);
		fault(errorMessage);
		return NULL;
	}

	// the code has been compiled, return it
	return compiledCode;
}

enum ExecuteMode {
	BYTECODE_MODE, ARGUMENT_MODE
};

void mish_execute(Code* code) {
	// stacks
	Stack<Iterator<Bytecode*>*> callStack;
	Stack<Iterator<Expression*>*> argumentStack;
	Stack<List<Value*>*> evaluationsStack;
	Stack<Function*> functionStack;
	Stack<ExecuteMode> modeStack;
	Value* returnValue;

	// start executing on first bytecode
	callStack.push(new Iterator<Bytecode*>(code->bytecodes.iterator()));
	modeStack.push(BYTECODE_MODE);

	while (true) {
		if (modeStack.peek() == BYTECODE_MODE) {
			if (callStack.peek()->hasNext()) {
				Bytecode* bytecode = callStack.peek()->next();
				switch (bytecode->instruction) {
				case FUNC_CALL:
					FunctionCallVoid* functionCallVoid =
							(FunctionCallVoid*) bytecode;

					argumentStack.push(
							new Iterator<Expression*>(
									functionCallVoid->arguments->iterator()));
					evaluationsStack.push(new List<Value*>());
					functionStack.push(functionCallVoid->function);
					modeStack.push(ARGUMENT_MODE);

					break;
				}
			} else {
				// TODO end of function... if non-void function, crash system, if void function, then return
				// for now, just end the execution
				break;
			}
		} else if (modeStack.peek() == ARGUMENT_MODE) {
			if (argumentStack.peek()->hasNext()) {
				Expression* expression = argumentStack.peek()->next();
				switch (expression->expressionType) {
				case VALUE_EXPRESSION:
					evaluationsStack.peek()->add((Value*) expression);
					break;
				case FUNCTION_EXPRESSION:
					log(L"function");
					FunctionCallReturn* functionCallReturn =
							(FunctionCallReturn*) expression;

					argumentStack.push(
							new Iterator<Expression*>(
									functionCallReturn->arguments->iterator()));
					evaluationsStack.push(new List<Value*>());
					functionStack.push(functionCallReturn->function);
					modeStack.push(ARGUMENT_MODE);

					break;
				}
			} else {
				// out of arguments, call the function
				delete argumentStack.pop();
				List<Value*>* arguments = evaluationsStack.pop();

				if (functionStack.peek()->native != NULL) {
					returnValue = functionStack.pop()->native(arguments);
					modeStack.pop();
					if (modeStack.peek() == ARGUMENT_MODE) {
						evaluationsStack.peek()->add(returnValue);
					}
					modeStack.push(BYTECODE_MODE);
				} else {
					// TODO push callStack
				}

				delete arguments;
			}
		} else {
			crash(L"unknown execution mode");
		}
	}

	delete callStack.pop();
}
