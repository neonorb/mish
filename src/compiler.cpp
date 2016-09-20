/*
 * compiler.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <array.h>
#include <compiler.h>

#define VALID_SYMBOL_CHARS L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"
#define WHITESPACE_CHARS L" \t\n"

CompilerState::CompilerState() {
	code = new Code();

	mode = new Stack<ParseMode>();

	argumentsStack = new Stack<List<Expression*>*>();

	symbol = new List<wchar_t>();
	symbolStack = new Stack<String>();

	string = new List<wchar_t>();
	escaping = false;
}

CompilerState::~CompilerState() {
	// mode
	delete mode;

	// arguments
	while (argumentsStack->size() > 0) {
		List<Expression*>* arguments = argumentsStack->pop();

		Iterator<Expression*> argumentsIterator = arguments->iterator();
		while (argumentsIterator.hasNext()) {
			delete argumentsIterator.next();
		}

		delete arguments;
	}
	delete argumentsStack;

	// symbol
	delete symbol;
	while (symbolStack->size() > 0) {
		delete symbolStack->pop();
	}
	delete symbolStack;

	// string
	delete string;
}

static bool isValidSymbolChar(wchar_t c) {
	return arrayContains<wchar_t>(VALID_SYMBOL_CHARS,
			strlen(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(wchar_t c) {
	return arrayContains<wchar_t>(WHITESPACE_CHARS, strlen(WHITESPACE_CHARS), c);
}

static String processCharacter(char c, CompilerState* state) {
	parseChar: switch (state->mode->peek()) {
	case EXPECT_STATEMENT:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		} else if (c == ';') {
			break;
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			state->mode->push(SYMBOL);
			goto parseChar;
		} else if (c == '#') {
			state->mode->push(COMMENT);
		} else {
			return L"expected statement";
		}
		break;
	case EXPECT_STATEMENT_TERMINATOR:
		if (c == ';' || c == '\n') {
			state->mode->pop();
		} else if (isWhitespace(c)) {
			break;
		} else {
			return L"expected statement terminator";
			break;
		}

		break;
	case SYMBOL:
		if (isValidSymbolChar(c)) {
			state->symbol->add(c);
		} else {
			// end the symbol because this isn't a valid symbol char
			wchar_t* sym = (wchar_t*) create(state->symbol->size() * 2 + 1);

			// copy the list of characters to an actual string
			Iterator<wchar_t> symbolIterator = state->symbol->iterator();
			uint64 symIndex = 0;
			while (symbolIterator.hasNext()) {
				sym[symIndex] = symbolIterator.next();
				symIndex++;
			}
			sym[symIndex] = NULL; // null terminate
			state->symbol->clear();

			state->symbolStack->push(sym);

			state->mode->pop();
			state->mode->push(SYMBOL_READY);
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
			state->argumentsStack->push(new List<Expression*>());

			state->mode->pop();
			state->mode->push(FUNCTION);
		} else {
			return L"unexpected character";
		}
		break;
	case FUNCTION:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == ')') {
			String symbol = state->symbolStack->pop();

			// determine if this is a syscall
			List<Function*>* functions;
			if (stringStartsWith(symbol, L"__")) {
				functions = &mish_syscalls;
			} else {
				// TODO regular function
			}

			// search for the function
			Iterator<Function*> functionsIterator = functions->iterator();
			Function* function = NULL;
			bool found = false;
			while (functionsIterator.hasNext() && !found) {
				function = functionsIterator.next();
				// check function name
				if (strequ(function->name, symbol)) {
					// check parameter sizes
					if (state->argumentsStack->peek()->size()
							!= function->parameterTypes->size()) {
						continue;
					}

					// check parameter types
					Iterator<Expression*> argumentsIterator =
							state->argumentsStack->peek()->iterator();
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
				return L"syscall not found";
				break;
			}

			// add the function call to the bytecodes
			if (state->mode->size() > 1
					&& (state->mode->peek(2) == EXPECT_ARGUMENT
							|| state->mode->peek(2) == FUNCTION)) {
				Expression* callExpression =
						(Expression*) new FunctionCallReturn(function,
								state->argumentsStack->pop());
				state->argumentsStack->peek()->add(callExpression);
				state->mode->pop();
			} else {
				Bytecode* callBytecode = (Bytecode*) new FunctionCallVoid(
						function, state->argumentsStack->pop());
				state->code->bytecodes->add(callBytecode);
				state->mode->pop();
				state->mode->push(EXPECT_STATEMENT_TERMINATOR);
			}

			break;
		} else if (c == ',') {
			// after a comma, there must be an argument
			state->mode->push(EXPECT_ARGUMENT);
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
			if (state->mode->peek() == EXPECT_ARGUMENT) {
				state->mode->pop();
			}
			// begin string
			state->mode->push(STRING);
		} else if (isValidSymbolChar(c)) {
			if (state->mode->peek() == EXPECT_ARGUMENT) {
				state->mode->pop();
			}
			// begin symbol
			state->mode->push(SYMBOL);
			goto parseChar;
		} else {
			// wtf is this character?
			return L"unexpected argument";
			break;
		}
		break;
	case STRING:
		if (state->escaping) {
			if (c == 'n') {
				// new line escape sequence
				state->string->add('\n');
			} else if (c == '\\') {
				// backslash escape sequence
				state->string->add('\\');
			} else if (c == '\'') {
				// quote escape sequence
				state->string->add('\'');
			} else {
				return L"unrecognized escape sequence";
				break;
			}

			// done with escape
			state->escaping = false;
		} else {
			if (c == '\\') {
				// begin escape
				state->escaping = true;
			} else if (c == '\'') {
				// end string
				wchar_t* str = (wchar_t*) create(state->string->size() * 2 + 1);

				// copy the list of characters to an actual string
				Iterator<wchar_t> stringIterator = state->string->iterator();
				uint64 strIndex = 0;
				while (stringIterator.hasNext()) {
					str[strIndex] = stringIterator.next();
					strIndex++;
				}
				str[strIndex] = NULL; // null terminate
				state->string->clear();

				// add the string to the arguments
				state->argumentsStack->peek()->add(
						(Expression*) new StringValue(str));
				state->mode->pop();
			} else {
				// just another character
				state->string->add(c);
			}
		}
		break;
	case COMMENT:
		if (c == '\n') {
			state->mode->pop();
		}
	}

	// no error message, return null
	return NULL;
}

Code* mish_compile(String code) {
	return mish_compile(code, strlen(code));
}

Code* mish_compile(String sourceCode, size_t size) {
	CompilerState* state = new CompilerState();
	state->mode->push(EXPECT_STATEMENT);

	uint64 lineStart = 0;
	uint64 lineEnd = NULL;
	uint64 lineNumber = 1;

	uint64 errorPosition = NULL;
	String errorMessage = NULL;
	for (uint64 i = 0; i < size; i++) {
		// get the next char to process
		wchar_t c = sourceCode[i];
		//write_serial(c);

		if (c == '\n') {
			if (errorMessage != NULL) {
				lineEnd = i;
				break;
			} else {
				lineStart = i + 1;
				lineNumber++;
			}
		}

		// if we have a previous error message, don't continue parsing
		if (errorMessage == NULL) {
			errorMessage = processCharacter(c, state);

			if (errorMessage != NULL) {
				errorPosition = i;
			}
		}
	}

	ParseMode endParseMode = state->mode->peek();
	if (errorMessage == NULL && endParseMode != EXPECT_STATEMENT
			&& endParseMode != COMMENT
			&& endParseMode != EXPECT_STATEMENT_TERMINATOR) {
		// something wasn't properly closed, throw a generic error for now
		debug(L"parse mode", state->mode->pop());
		errorMessage = L"incorrect parse mode";
	}

	if (errorMessage) {
		// generate the error message and display it
		String line = substring(sourceCode, lineStart, lineEnd);

		wchar_t* errorPositionMarker = (wchar_t*) create(
				(lineEnd - lineStart) * 2 + 1);
		uint64 i = 0;
		for (; i < (lineEnd - lineStart); i++) {
			if (lineStart + i == errorPosition) {
				errorPositionMarker[i] = '^';
			} else {
				errorPositionMarker[i] = '_';
			}
		}
		errorPositionMarker[i] = 0; // null terminate

		// print the error
		fault(line);
		fault(errorPositionMarker);
		debug(L"line", lineNumber, 10);
		fault(errorMessage);

		// delete error message stuff
		delete line;
		delete errorPositionMarker;
		delete errorMessage;

		delete state->code;
		delete state;
		return NULL;
	}

	// the code has been compiled, return it
	Code* code = state->code;
	delete state;

	return code;
}
