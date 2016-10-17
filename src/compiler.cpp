/*
 * compiler.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <array.h>
#include <compiler.h>

#define VALID_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"
#define WHITESPACE_CHARS " \t\n"

CompilerState::CompilerState() {
	codeStack = new Stack<Code*>();

	mode = new Stack<ParseMode>();

	requireExpression = false;
	argumentsStack = new Stack<List<Expression*>*>();

	symbol = new List<strchar>();
	symbolStack = new Stack<String>();

	string = new List<strchar>();
	escaping = false;

	conditionalTypeStack = new Stack<ConditionalBytecodeType>();
}

CompilerState::~CompilerState() {
	// code
	while (codeStack->size() > 0) {
		delete codeStack->pop();
	}
	delete codeStack;

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

	delete conditionalTypeStack;
}

static bool isValidSymbolChar(strchar c) {
	return arrayContains<strchar>(VALID_SYMBOL_CHARS,
			strlen(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(strchar c) {
	return arrayContains<strchar>(WHITESPACE_CHARS, strlen(WHITESPACE_CHARS), c);
}

static String processCharacter(strchar c, CompilerState* state) {
	parseChar: switch (state->mode->peek()) {
	case EXPECT_STATEMENT:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == ';') {
			break;
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			state->mode->push(SYMBOL);
			goto parseChar;
		} else if (c == '#') {
			state->mode->push(COMMENT);
			break;
		} else if (c == '}') {
			// check if we can close any blocks
			if (state->codeStack->size() == 0) {
				return "unexpected }";
			}

			state->mode->pop();
			if (state->mode->peek() == LOOP) {
				state->mode->pop();
				// close a while loop
				Code* whileCode = state->codeStack->pop();
				List<Expression*>* arguments = state->argumentsStack->pop();
				if (arguments->size() > 1) {
					crash("while loop should only have one argument");
				}
				state->codeStack->peek()->bytecodes->add(
						new ConditionalBytecode(arguments, whileCode,
								state->conditionalTypeStack->pop()));
				break;
			} else {
				crash("unexpected mode after closing block");
			}
		} else {
			return "expected statement";
		}
		break;
	case EXPECT_STATEMENT_TERMINATOR:
		if (c == ';' || c == '\n' || c == '}') {
			state->mode->pop();
			goto parseChar;
		} else if (isWhitespace(c)) {
			break;
		} else {
			return "expected statement terminator";
			break;
		}

		break;
	case SYMBOL:
		if (isValidSymbolChar(c)) {
			state->symbol->add(c);
		} else {
			// end the symbol because this isn't a valid symbol char
			strchar* sym = (strchar*) create(
					state->symbol->size() * sizeof(strchar) + 1);

			// copy the list of characters to an actual string
			Iterator<strchar> symbolIterator = state->symbol->iterator();
			uint64 symIndex = 0;
			while (symbolIterator.hasNext()) {
				sym[symIndex] = symbolIterator.next();
				symIndex++;
			}
			sym[symIndex] = NULL; // null terminate
			state->symbol->clear();

			state->mode->pop();

			if (strequ(sym, "true")) {
				state->argumentsStack->peek()->add(
						new BooleanValue(true, true));
				delete sym;
			} else if (strequ(sym, "false")) {
				state->argumentsStack->peek()->add(
						new BooleanValue(false, true));
				delete sym;
			} else {
				state->mode->push(SYMBOL_READY);
				state->symbolStack->push(sym);
			}

			// we havn't done anything with this char, so we have to re-parse it
			goto parseChar;
		}
		break;
	case SYMBOL_READY:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == '(') {
			if (strequ(state->symbolStack->peek(), "while")) {
				delete state->symbolStack->pop();

				state->argumentsStack->push(new List<Expression*>());

				state->mode->pop();
				state->mode->push(LOOP);
				state->mode->push(EXPRESSION);

				state->conditionalTypeStack->push(WHILE_CONDITIONALTYPE);
			} else if (strequ(state->symbolStack->peek(), "if")) {
				delete state->symbolStack->pop();

				state->argumentsStack->push(new List<Expression*>());

				state->mode->pop();
				state->mode->push(LOOP);
				state->mode->push(EXPRESSION);

				state->conditionalTypeStack->push(IF_CONDITIONALTYPE);
			} else {
				// begin function
				state->argumentsStack->push(new List<Expression*>());

				state->mode->pop();
				state->mode->push(FUNCTION);
				state->mode->push(EXPRESSION);
			}
		} else {
			return "unexpected character";
		}
		break;
	case EXPRESSION:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == '\'') {
			// begin string
			state->mode->push(STRING);
			state->requireExpression = false;
		} else if (isValidSymbolChar(c)) {
			state->mode->push(SYMBOL);
			state->requireExpression = false;
			goto parseChar;
		} else if (c == '(') {
			state->mode->push(REQUIRE_CLOSE_EXPRESSION);
			state->mode->push(EXPRESSION);
			// TODO open expression
		} else {
			if (state->mode->peek(1) == REQUIRE_CLOSE_EXPRESSION) {
				return "expression needs to be closed";
			} else if (state->requireExpression) {
				return "expression expected";
			}

			state->mode->pop();
			state->requireExpression = false;
			goto parseChar;
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
			if (stringStartsWith(symbol, "__")) {
				functions = &mish_syscalls;
			} else {
				// TODO regular function
				return "regular functions not implemented yet";
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
				return "syscall not found";
			}

			// add the function call to the bytecodes
			if (state->mode->size() > 1
					&& (state->mode->peek(1) == EXPRESSION
							|| state->mode->peek(1) == FUNCTION)) {
				Expression* callExpression =
						(Expression*) new FunctionCallReturn(function,
								state->argumentsStack->pop());
				state->argumentsStack->peek()->add(callExpression);
				state->mode->pop();
			} else {
				Bytecode* callBytecode = (Bytecode*) new FunctionCallVoid(
						function, state->argumentsStack->pop());
				state->codeStack->peek()->bytecodes->add(callBytecode);
				state->mode->pop();
				state->mode->push(EXPECT_STATEMENT_TERMINATOR);
			}

			break;
		} else if (c == ',') {
			// after a comma, there must be an argument
			state->requireExpression = true;
		}
		state->mode->push(EXPRESSION);
		break;
	case STRING:
		if (state->escaping) {
			if (c == 'n') {
				// new line escape sequence
				state->string->add('\n');
				state->string->add('\r');
			} else if (c == '\\') {
				// backslash escape sequence
				state->string->add('\\');
			} else if (c == '\'') {
				// quote escape sequence
				state->string->add('\'');
			} else {
				return "unrecognized escape sequence";
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
				strchar* str = (strchar*) create(
						state->string->size() * sizeof(strchar) + 1);

				// copy the list of characters to an actual string
				Iterator<strchar> stringIterator = state->string->iterator();
				uint64 strIndex = 0;
				while (stringIterator.hasNext()) {
					str[strIndex] = stringIterator.next();
					strIndex++;
				}
				str[strIndex] = NULL; // null terminate
				state->string->clear();

				// add the string to the arguments
				Value* stringConstant = new StringValue(str);
				stringConstant->isConstant = true;
				state->argumentsStack->peek()->add(
						(Expression*) stringConstant);
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
		break;
	case LOOP:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == ')') {
			int argumentsSize = state->argumentsStack->peek()->size();
			if (argumentsSize == 0) {
				return "statement needs a condition";
			} else if (argumentsSize > 1) {
				return "argument size is greater than 1";
			}

			state->mode->push(EXPECT_BLOCK);
			break;
		} else {
			return "unexpected character to end while";
		}
		break;
	case EXPECT_BLOCK:
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == '{') {
			state->mode->pop();
			state->mode->push(EXPECT_STATEMENT);
			state->codeStack->push(new Code());
		} else {
			return "expected block";
		}
	}

	// no error message, return null
	return NULL;
}

Code* mish_compile(String code) {
	return mish_compile(code, strlen(code));
}

Code* mish_compile(String sourceCode, size size) {
	CompilerState* state = new CompilerState();
	state->codeStack->push(new Code());
	state->mode->push(EXPECT_STATEMENT);

	// line stuff
	uint64 lineStart = 0;
	uint64 lineEnd = NULL;
	uint64 lineNumber = 1;

	// error message stuff
	bool hasError = false;
	uint64 errorPosition = NULL;
	String errorMessage = NULL;

	uinteger i = 0;
	for (; i < size; i++) {
		// get the next char to process
		strchar c = sourceCode[i];
		//debug("char", (uint64) c);

		if (c == '\n') {
			if (hasError) {
				lineEnd = i;
				break;
			} else {
				lineStart = i + 1;
				lineNumber++;
			}
		}

		// if we have an error, don't continue parsing the source code
		if (!hasError) {
			errorMessage = processCharacter(c, state);

			if (errorMessage != NULL) {
				hasError = true;
				errorPosition = i;
			}
		}
	}

	ParseMode endParseMode = state->mode->peek();
	if (errorMessage == NULL && endParseMode != EXPECT_STATEMENT
			&& endParseMode != COMMENT
			&& endParseMode != EXPECT_STATEMENT_TERMINATOR) {
		// something wasn't properly closed, throw a generic error for now
		debug("parse mode", (uint64) state->mode->pop());
		errorMessage = "incorrect parse mode";
		hasError = true;
	}

	if (hasError) {
		if (lineEnd == NULL) {
			lineEnd = i;
		}

		debug("lineStart", lineStart);
		debug("lineEnd", lineEnd);

		// generate the error message and display it
		String line = substring(sourceCode, lineStart, lineEnd);

		strchar* errorPositionMarker = (strchar*) create(
				(lineEnd - lineStart) * sizeof(strchar) + 1);
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
		debug("line", lineNumber, 10);
		fault(errorMessage);

		// delete error message stuff
		delete line;
		delete errorPositionMarker;
		//delete errorMessage;

		delete state;
		return NULL;
	}

	// the code has been compiled, return it
	Code* code = state->codeStack->pop();
	delete state;

	return code;
}
