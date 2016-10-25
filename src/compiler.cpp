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

// CompilerStackFrame
CompilerStackFrame::CompilerStackFrame(CompilerStackFrameType type) {
	this->type = type;
}

CompilerStackFrame::~CompilerStackFrame() {

}

// BodyCompilerStackFrame
BodyCompilerStackFrame::BodyCompilerStackFrame() :
		CompilerStackFrame(CompilerStackFrameType::BODY) {
	mode = BodyCompilerStackFrameMode::READY;
	symbol = NULL;
}

BodyCompilerStackFrame::~BodyCompilerStackFrame() {
	if (symbol != NULL) {
		delete symbol;
	}
}

// IfCompilerStackFrame
IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type) :
		CompilerStackFrame(CompilerStackFrameType::IF) {
	this->mode = IfCompilerStackFrameMode::EXPECT_P;
	this->type = type;
}

IfCompilerStackFrame::~IfCompilerStackFrame() {

}

// WhileCompilerStackFrame
WhileCompilerStackFrame::WhileCompilerStackFrame(bool isDoWhile) :
		CompilerStackFrame(CompilerStackFrameType::WHILE) {
	this->mode = WhileCompilerStackFrameMode::EXPECT_P;
	this->isDoWhile = isDoWhile;
}

WhileCompilerStackFrame::~WhileCompilerStackFrame() {

}

// SymbolCompilerStackFrame
SymbolCompilerStackFrame::SymbolCompilerStackFrame(
		StringCallback symbolCallback) :
		CompilerStackFrame(CompilerStackFrameType::SYMBOL) {
	symbol = new List<strchar>();
	this->symbolCallback = symbolCallback;
}

SymbolCompilerStackFrame::~SymbolCompilerStackFrame() {
	delete symbol;
}

// StringCompilerStackFrame
StringCompilerStackFrame::StringCompilerStackFrame(
		StringCallback stringCallback) :
		CompilerStackFrame(CompilerStackFrameType::STRING) {
	string = new List<strchar>();
	escaping = false;
	this->stringCallback = stringCallback;
}

StringCompilerStackFrame::~StringCompilerStackFrame() {
	delete string;
}

// CommentCompilerStackFrame
CommentCompilerStackFrame::CommentCompilerStackFrame(
		CommentCompilerStackFrameType type) :
		CompilerStackFrame(CompilerStackFrameType::COMMENT) {
	this->type = type;
}

CommentCompilerStackFrame::~CommentCompilerStackFrame() {

}

// FunctionCallCompilerStackFrame
FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name) :
		CompilerStackFrame(CompilerStackFrameType::FUNCTION_CALL) {
	this->name = name;
}

FunctionCallCompilerStackFrame::~FunctionCallCompilerStackFrame() {
	delete name;
}

// ExpressionCompilerStackFrame
ExpressionCompilerStackFrame::ExpressionCompilerStackFrame(
		ExpressionCallback expressionCallback) :
		CompilerStackFrame(CompilerStackFrameType::EXPRESSION) {
	this->expressionCallback = expressionCallback;
}

ExpressionCompilerStackFrame::~ExpressionCompilerStackFrame() {

}

// ArgumentCompilerStackFrame
ArgumentCompilerStackFrame::ArgumentCompilerStackFrame(
		ExpressionCallback argumentCallback) :
		CompilerStackFrame(CompilerStackFrameType::ARGUMENT) {
	this->argumentCallback = argumentCallback;
	requireArgument = false;
}

ArgumentCompilerStackFrame::~ArgumentCompilerStackFrame() {

}

// main compiler state

CompilerState::CompilerState() {
	compilerStack = new Stack<CompilerStackFrame*>();
}

CompilerState::~CompilerState() {
	while (compilerStack->size() > 0) {
		delete compilerStack->pop();
	}
	delete compilerStack;
}

static bool isValidSymbolChar(strchar c) {
	return arrayContains<strchar>(VALID_SYMBOL_CHARS,
			strlen(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(strchar c) {
	return arrayContains<strchar>(WHITESPACE_CHARS, strlen(WHITESPACE_CHARS), c);
}

static String processCharacter(strchar c, CompilerState* state) {
	parseChar: if (state->compilerStack->peek()->type
			== CompilerStackFrameType::BODY) {
		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		BodyCompilerStackFrame* stackFrame =
				(BodyCompilerStackFrame*) state->compilerStack->peek();
		if (stackFrame->mode == BodyCompilerStackFrameMode::READY) {
			if (c == ';') {
				break;
			} else if (isValidSymbolChar(c)) {
				// begin a symbol
				stackFrame->mode = BodyCompilerStackFrameMode::SYMBOL;
				state->compilerStack->push(
						new SymbolCompilerStackFrame(
								[stackFrame](String symbol) {
									stackFrame->symbol = symbol;
								}));
				goto parseChar;
			} else if (c == '#') {
				state->compilerStack->push(
						new CommentCompilerStackFrame(
								CommentCompilerStackFrameType::LINE));
			} else if (c == '}') {
				delete state->compilerStack->pop();

				state->mode->pop();
				if (state->mode->peek() == CONDITIONAL) {
					state->mode->pop();

					// close a conditional
					Code* code = state->codeStack->pop();
					List<Expression*>* arguments = state->argumentsStack->pop();
					if (arguments->size() > 1) {
						crash("conditional should only have one argument");
					}

					ConditionalBytecodeType type =
							state->conditionalTypeStack->pop();

					ConditionalBytecode* newBytecode = new ConditionalBytecode(
							arguments, code, type);

					if (type == IF_CONDITIONALTYPE
							|| type == WHILE_CONDITIONALTYPE
							|| type == DOWHILE_CONDITIONALTYPE) {
						state->codeStack->peek()->bytecodes->add(newBytecode);
					} else if (type == ELSEIF_CONDITIONALTYPE) {
						Bytecode* lastBytecode =
								state->codeStack->peek()->bytecodes->getLast();
						if (lastBytecode->type == CONDITIONAL_INSTRUCTION) {
							ConditionalBytecode* conditionalBytecode =
									(ConditionalBytecode*) lastBytecode;
							conditionalBytecode->elseifs->add(newBytecode);
						} else {
							return "else not expected here";
						}
					} else {
						crash("unexpected conditional type");
					}
					break;
				} else {
					crash("unexpected mode after closing block");
				}
			} else {
				return "expected statement";
			}
		} else if (stackFrame->mode
				== BodyCompilerStackFrameMode::EXPECT_TERMINATOR) {
			if (c == ';' || c == '\n' || c == '}') {
				delete state->compilerStack->pop();
				goto parseChar;
			} else if (isWhitespace(c)) {
				break;
			} else {
				return "expected statement terminator";
			}
		} else if (stackFrame->mode == BodyCompilerStackFrameMode::SYMBOL) {
			// skip any whitespace
			if (isWhitespace(c)) {
				break;
			}

			//=====
			if (strequ(stackFrame->symbol, "true")) {
				state->argumentsStack->peek()->add(
						new BooleanValue(true, true));
				delete sym;
			} else if (strequ(sym, "false")) {
				state->argumentsStack->peek()->add(
						new BooleanValue(false, true));
				delete sym;
			}

			// add the string to the arguments
			Value* stringConstant = new StringValue(str);
			stringConstant->isConstant = true;
			state->argumentsStack->peek()->add((Expression*) stringConstant);
			state->mode->pop();
			//====== TODO this doesn't belong here, rather in the argument parsing stack frame

			if (c == '(') {
				if (strequ(stackFrame->symbol, "while")) {
					delete stackFrame->symbol;
					delete stackFrame->symbol;

					state->compilerStack->push(
							new WhileCompilerStackFrame(false));
				} else if (strequ(stackFrame->symbol, "dowhile")) {
					delete stackFrame->symbol;
					delete stackFrame->symbol;

					state->compilerStack->push(
							new WhileCompilerStackFrame(true));
				} else if (strequ(stackFrame->symbol, "if")) {
					delete stackFrame->symbol;

					state->compilerStack->push(
							new IfCompilerStackFrame(
									IfCompilerStackFrameType::IF));
				} else if (strequ(stackFrame->symbol, "elseif")) {
					delete stackFrame->symbol;

					state->compilerStack->push(
							new IfCompilerStackFrame(
									IfCompilerStackFrameType::ELSEIF));
				} else {
					state->compilerStack->push(
							new FunctionCallCompilerStackFrame(
									stackFrame->symbol));
				}
			} else if (c == '{') {
				if (strequ(stackFrame->symbol, "else")) {
					delete stackFrame->symbol;

					state->compilerStack->push(
							new IfCompilerStackFrame(
									IfCompilerStackFrameType::ELSE));
				} else {
					return "unexpected character";
				}
			} else {
				return "unexpected character";
			}
		} else {
			crash("unknown BodyCompilerStackFrameType");
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::IF) {
		IfCompilerStackFrame* stackFrame =
				(IfCompilerStackFrame*) state->compilerStack->peek();

	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::WHILE) {
		WhileCompilerStackFrame* stackFrame =
				(WhileCompilerStackFrame*) state->compilerStack->peek();

	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::SYMBOL) {
		SymbolCompilerStackFrame* stackFrame =
				(SymbolCompilerStackFrame*) state->compilerStack->peek();

		if (isValidSymbolChar(c)) {
			stackFrame->symbol->add(c);
		} else {
			// end the symbol because this isn't a valid symbol char
			strchar* sym = (strchar*) create(
					stackFrame->symbol->size() * sizeof(strchar) + 1);

			// copy the list of characters to an actual string
			Iterator<strchar> symbolIterator = stackFrame->symbol->iterator();
			uint64 symIndex = 0;
			while (symbolIterator.hasNext()) {
				sym[symIndex] = symbolIterator.next();
				symIndex++;
			}
			sym[symIndex] = NULL; // null terminated

			delete state->compilerStack->pop();
			stackFrame->symbolCallback(sym);

			// we havn't done anything with this char, so we have to re-parse it
			goto parseChar;
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::STRING) {
		StringCompilerStackFrame* stackFrame =
				(StringCompilerStackFrame*) state->compilerStack->peek();

		if (stackFrame->escaping) {
			if (c == 'n') {
				// new line escape sequence
				stackFrame->string->add('\n');
				stackFrame->string->add('\r');
			} else if (c == '\\') {
				// backslash escape sequence
				stackFrame->string->add('\\');
			} else if (c == '\'') {
				// quote escape sequence
				stackFrame->string->add('\'');
			} else {
				return "unrecognized escape sequence";
			}

			// done with escape
			stackFrame->escaping = false;
		} else {
			if (c == '\\') {
				// begin escape
				stackFrame->escaping = true;
			} else if (c == '\'') {
				// end string
				strchar* str = (strchar*) create(
						stackFrame->string->size() * sizeof(strchar) + 1);

				// copy the list of characters to an actual string
				Iterator<strchar> stringIterator =
						stackFrame->string->iterator();
				uint64 strIndex = 0;
				while (stringIterator.hasNext()) {
					str[strIndex] = stringIterator.next();
					strIndex++;
				}
				str[strIndex] = NULL; // null terminate

				delete state->compilerStack->pop();
				stackFrame->stringCallback(str);
			} else {
				// just another character
				stackFrame->string->add(c);
			}
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::COMMENT) {
		CommentCompilerStackFrame* stackFrame =
				(CommentCompilerStackFrame*) state->compilerStack->peek();

		if (stackFrame->type == CommentCompilerStackFrameType::LINE) {
			if (c == '\n') {
				delete state->compilerStack->pop();
			}
			// ignore character
		} else {
			crash("unknown CommentCompilerStackFrameType");
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::EXPRESSION) {
		ExpressionCompilerStackFrame* stackFrame =
				(ExpressionCompilerStackFrame*) state->compilerStack->peek();

		// skip any whitespace
		if (isWhitespace(c)) {
			break;
		}

		if (c == '\'') {
			// begin string
			state->compilerStack->push(
					new StringCompilerStackFrame([stackFrame](String string) {
						stackFrame->expressionCallback(new StringValue(string));
					}));
		} else if (isValidSymbolChar(c)) {
			state->mode->push(SYMBOL);
			state->requireExpression = false;
			goto parseChar;
		} else if (c == '(') {
			state->compilerStack->push(
					new ExpressionCompilerStackFrame(
							[stackFrame](Expression* expression) {
								// TODO do something with expression
								// TODO maybe expression stack frame has to have close parenthesis flag?
							}));
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
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::ARGUMENT) {
		ArgumentCompilerStackFrame* stackFrame =
				(ArgumentCompilerStackFrame*) state->compilerStack->peek();
		if (c == ',') {
			stackFrame->requireArgument = true;
		} else if (c == ')') {
			if (stackFrame->requireArgument) {
				return "expected argument";
			}

			delete state->compilerStack->pop();
		} else {
			stackFrame->requireArgument = false;
			state->compilerStack->push(
					new ExpressionCompilerStackFrame(
							[stackFrame](Expression* expression) {
								stackFrame->argumentCallback(expression);
							}));
		}
	} else {
		crash("unknown CompilerStackFrameType");
	}

	// ====================================================================================================================================

	parseChar: switch (state->mode->peek()) {
	case EXPRESSION:

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

		break;
	case COMMENT:
		if (c == '\n') {
			state->mode->pop();
		}
		break;
	case CONDITIONAL:
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
