/*
 * compiler.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <array.h>
#include <compiler.h>
#include <feta.h>

#define VALID_SYMBOL_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"
#define WHITESPACE_CHARS " \t\n"

// CompilerStackFrame
CompilerStackFrame::CompilerStackFrame(CompilerStackFrameType type) {
	this->type = type;
}

CompilerStackFrame::~CompilerStackFrame() {

}

// BodyCompilerStackFrame
BodyCompilerStackFrame::BodyCompilerStackFrame(CodeCallback codeCallback) :
		CompilerStackFrame(CompilerStackFrameType::BODY) {
	mode = BodyCompilerStackFrameMode::READY;
	symbol = NULL;
	code = new Code();
	this->codeCallback = codeCallback;
	lastWasTerminated = true;
}

BodyCompilerStackFrame::~BodyCompilerStackFrame() {
	if (symbol != NULL) {
		delete symbol;
	}
}

// IfCompilerStackFrame
IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type) :
		CompilerStackFrame(CompilerStackFrameType::IF) {
	this->type = type;
	mode = IfCompilerStackFrameMode::EXPECT_P;
	condition = NULL;
	code = NULL;
	ifBytecode = NULL;
}

IfCompilerStackFrame::IfCompilerStackFrame(
		IfBytecodeCallback ifBytecodeCallback) :
		IfCompilerStackFrame(IfCompilerStackFrameType::IF) {
	this->mode = IfCompilerStackFrameMode::EXPECT_P;
	this->ifBytecodeCallback = ifBytecodeCallback;
}

IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type,
		IfBytecode* ifBytecode) :
		IfCompilerStackFrame(type) {
	if (type == IfCompilerStackFrameType::ELSEIF) {
		mode = IfCompilerStackFrameMode::EXPECT_P;
	} else if (type == IfCompilerStackFrameType::ELSE) {
		mode = IfCompilerStackFrameMode::EXPECT_BODY;
		condition = new BooleanValue(true, true);
	} else {
		crash("use IfCompilerStackFrame(IfBytecodeCallback)");
	}
	this->ifBytecode = ifBytecode;
}

IfCompilerStackFrame::~IfCompilerStackFrame() {

}

// WhileCompilerStackFrame
WhileCompilerStackFrame::WhileCompilerStackFrame(bool isDoWhile,
		WhileBytecodeCallback whileBytecodeCallback) :
		CompilerStackFrame(CompilerStackFrameType::WHILE) {
	this->mode = WhileCompilerStackFrameMode::EXPECT_P;
	this->isDoWhile = isDoWhile;
	this->whileBytecodeCallback = whileBytecodeCallback;

	condition = NULL;
	code = NULL;
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
FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		FunctionCallCompilerStackFrameType type) :
		CompilerStackFrame(CompilerStackFrameType::FUNCTION_CALL) {
	this->name = name;
	arguments = new List<Expression*>();
	this->type = type;
	mode = FunctionCallCompilerStackFrameMode::EXPECT_P;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		FunctionCallBytecodeCallback functionCallBytecodeCallback) :
		FunctionCallCompilerStackFrame(name,
				FunctionCallCompilerStackFrameType::BYTECODE) {
	this->functionCallBytecodeCallback = functionCallBytecodeCallback;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		FunctionCallExpressionCallback functionCallExpressionCallback) :
		FunctionCallCompilerStackFrame(name,
				FunctionCallCompilerStackFrameType::EXPRESSION) {
	this->functionCallExpressionCallback = functionCallExpressionCallback;
}

FunctionCallCompilerStackFrame::~FunctionCallCompilerStackFrame() {
	delete name;
}

// ExpressionCompilerStackFrame
ExpressionCompilerStackFrameStringCallbackStruct::ExpressionCompilerStackFrameStringCallbackStruct(
		ExpressionCompilerStackFrame* stackFrame, CompilerState* state) {
	this->stackFrame = stackFrame;
	this->state = state;
}

ExpressionCompilerStackFrame::ExpressionCompilerStackFrame(
		ExpressionCallback expressionCallback) :
		CompilerStackFrame(CompilerStackFrameType::EXPRESSION) {
	this->expressionCallback = expressionCallback;
	symbol = NULL;
	mode = ExpressionCompilerStackFrameMode::READY;
}

ExpressionCompilerStackFrame::~ExpressionCompilerStackFrame() {
	if (symbol != NULL) {
		delete symbol;
	}
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
	return arrayContains<strchar>((strchar*) VALID_SYMBOL_CHARS,
			sizeof(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(strchar c) {
	return arrayContains<strchar>((strchar*) WHITESPACE_CHARS,
			sizeof(WHITESPACE_CHARS), c);
}

static String processCharacter(strchar c, CompilerState* state) {
	// TODO process EOF
	parseChar:

	// skip any whitespace
	if (state->compilerStack->peek()->type != CompilerStackFrameType::SYMBOL
			&& state->compilerStack->peek()->type
					!= CompilerStackFrameType::STRING
			&& state->compilerStack->peek()->type
					!= CompilerStackFrameType::BODY) {

		if (isWhitespace(c)) {
			return NULL;
		}
	}

	if (state->compilerStack->peek()->type == CompilerStackFrameType::BODY) {
		BodyCompilerStackFrame* stackFrame =
				(BodyCompilerStackFrame*) state->compilerStack->peek();

		if (stackFrame->mode == BodyCompilerStackFrameMode::READY) {
			if (c == ';' || c == '\n') {
				stackFrame->lastWasTerminated = true;
				return NULL;
			} else if (isWhitespace(c)) {
				return NULL;
			} else if (c == '}') {
				stackFrame->codeCallback(stackFrame->code);
				delete state->compilerStack->pop();
				// the statement that created this body needs to be closed, run its closing code
				goto parseChar;
				// TODO potential issue with closing multiple bodys...maybe
			} else if (isValidSymbolChar(c)) {
				// begin a symbol
				stackFrame->mode = BodyCompilerStackFrameMode::SYMBOL;
				state->compilerStack->push(
						new SymbolCompilerStackFrame(
								StringCallback(stackFrame,
										[](void* stackFrame, String symbol) -> void* {
											((BodyCompilerStackFrame*) stackFrame)->symbol = symbol;
											return VALUE_NOT_USED;
										})));
				goto parseChar;
			} else if (c == '#') {
				state->compilerStack->push(
						new CommentCompilerStackFrame(
								CommentCompilerStackFrameType::LINE));
			} else {
				return "expected character";
			}
		} else if (stackFrame->mode == BodyCompilerStackFrameMode::SYMBOL) {
			// skip any whitespace
			if (isWhitespace(c)) {
				return NULL;
			}

			if (strequ(stackFrame->symbol, "while")) {
				if (!stackFrame->lastWasTerminated) {
					return "last statement wasn't terminated";
				}

				// while
				delete stackFrame->symbol;

				state->compilerStack->push(
						new WhileCompilerStackFrame(false,
								WhileBytecodeCallback(stackFrame,
										[](void* stackFrame, WhileBytecode* whileBytecode) -> void* {
											((BodyCompilerStackFrame*)stackFrame)->code->bytecodes->add(whileBytecode);
											return VALUE_NOT_USED;
										})));
			} else if (strequ(stackFrame->symbol, "dowhile")) {
				if (!stackFrame->lastWasTerminated) {
					return "last statement wasn't terminated";
				}

				// dowhile
				delete stackFrame->symbol;

				state->compilerStack->push(
						new WhileCompilerStackFrame(true,
								WhileBytecodeCallback(stackFrame,
										[](void* stackFrame, WhileBytecode* whileBytecode) -> void* {
											((BodyCompilerStackFrame*)stackFrame)->code->bytecodes->add(whileBytecode);
											return VALUE_NOT_USED;
										})));
			} else if (strequ(stackFrame->symbol, "if")) {
				if (!stackFrame->lastWasTerminated) {
					return "last statement wasn't terminated";
				}

				// if
				delete stackFrame->symbol;

				state->compilerStack->push(
						new IfCompilerStackFrame(
								IfBytecodeCallback(stackFrame,
										[](void* stackFrame, IfBytecode* ifBytecode) -> void* {
											((BodyCompilerStackFrame*)stackFrame)->code->bytecodes->add(ifBytecode);
											return VALUE_NOT_USED;
										})));
			} else if (strequ(stackFrame->symbol, "elseif")) {
				// elseif
				delete stackFrame->symbol;

				Bytecode* lastBytecode = stackFrame->code->bytecodes->getLast();

				if (lastBytecode != NULL
						&& lastBytecode->type == BytecodeType::IF) {
					if (stackFrame->lastWasTerminated) {
						return "may not terminate an if statement before an elseif statement";
					}

					state->compilerStack->push(
							new IfCompilerStackFrame(
									IfCompilerStackFrameType::ELSEIF,
									(IfBytecode*) lastBytecode));
				} else {
					return "elseif statement may only follow an if statement";
				}
			} else if (strequ(stackFrame->symbol, "else")) {
				// else
				delete stackFrame->symbol;

				Bytecode* lastBytecode = stackFrame->code->bytecodes->getLast();

				if (lastBytecode != NULL
						&& lastBytecode->type == BytecodeType::IF) {
					if (stackFrame->lastWasTerminated) {
						return "may not terminate an if statement before and else statement";
					}

					state->compilerStack->push(
							new IfCompilerStackFrame(
									IfCompilerStackFrameType::ELSE,
									(IfBytecode*) lastBytecode));
				} else {
					return "else statement may only follow an if statement";
				}
			} else {
				if (!stackFrame->lastWasTerminated) {
					return "last statement wasn't terminated";
				}

				// function
				state->compilerStack->push(
						new FunctionCallCompilerStackFrame(stackFrame->symbol,
								FunctionCallBytecodeCallback(stackFrame,
										[](void* stackFrame, FunctionCallVoid* functionBytecode) -> void* {
											((BodyCompilerStackFrame*)stackFrame)->code->bytecodes->add(functionBytecode);
											return VALUE_NOT_USED;
										})));
			}
			stackFrame->symbol = NULL;
			stackFrame->mode = BodyCompilerStackFrameMode::READY;
			stackFrame->lastWasTerminated = false;

			goto parseChar;
		} else {
			crash("unknown BodyCompilerStackFrameMode");
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::IF) {
		IfCompilerStackFrame* stackFrame =
				(IfCompilerStackFrame*) state->compilerStack->peek();
		if (stackFrame->mode == IfCompilerStackFrameMode::EXPECT_P) {
			if (c == '(') {
				stackFrame->mode = IfCompilerStackFrameMode::EXPECT_CONDITION;
			} else {
				return "expected (";
			}
		} else if (stackFrame->mode
				== IfCompilerStackFrameMode::EXPECT_CONDITION) {
			if (c == ')') {
				return "expected condition";
			} else {
				state->compilerStack->push(
						new ExpressionCompilerStackFrame(
								ExpressionCallback(stackFrame,
										[](void* stackFrame, Expression* condition) -> void* {
											((IfCompilerStackFrame*)stackFrame)->condition = condition;
											return VALUE_NOT_USED;
										})));
				stackFrame->mode = IfCompilerStackFrameMode::EXPECT_CP;
				goto parseChar;
			}
		} else if (stackFrame->mode == IfCompilerStackFrameMode::EXPECT_CP) {
			if (c == ')') {
				stackFrame->mode = IfCompilerStackFrameMode::EXPECT_BODY;
			} else {
				return "expected )";
			}
		} else if (stackFrame->mode == IfCompilerStackFrameMode::EXPECT_BODY) {
			if (c == '{') {
				stackFrame->mode = IfCompilerStackFrameMode::DONE;
				state->compilerStack->push(
						new BodyCompilerStackFrame(
								CodeCallback(stackFrame,
										[](void* stackFrame, Code* code) -> void* {
											((IfCompilerStackFrame*)stackFrame)->code = code;
											return VALUE_NOT_USED;
										})));
			} else {
				return "expected {";
			}
		} else if (stackFrame->mode == IfCompilerStackFrameMode::DONE) {
			IfConditionCode* ifConditionCode = new IfConditionCode(
					stackFrame->condition, stackFrame->code);
			if (stackFrame->type == IfCompilerStackFrameType::IF) {
				stackFrame->ifBytecodeCallback(new IfBytecode(ifConditionCode));
			} else if (stackFrame->type == IfCompilerStackFrameType::ELSEIF
					|| stackFrame->type == IfCompilerStackFrameType::ELSE) {
				stackFrame->ifBytecode->ifs->add(ifConditionCode);
			} else {
				crash("unknown IfCompilerStackFrameType");
			}
			delete state->compilerStack->pop();
		} else {
			crash("unknown IfCompilerStackFrameMode");
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::WHILE) {
		WhileCompilerStackFrame* stackFrame =
				(WhileCompilerStackFrame*) state->compilerStack->peek();
		if (stackFrame->mode == WhileCompilerStackFrameMode::EXPECT_P) {
			if (c == '(') {
				stackFrame->mode =
						WhileCompilerStackFrameMode::EXPECT_CONDITION;
			} else {
				return "expected (";
			}
		} else if (stackFrame->mode
				== WhileCompilerStackFrameMode::EXPECT_CONDITION) {
			if (c == ')') {
				return "expected condition";
			} else {
				state->compilerStack->push(
						new ExpressionCompilerStackFrame(
								ExpressionCallback(stackFrame,
										[](void* stackFrame, Expression* condition) -> void* {
											((WhileCompilerStackFrame*)stackFrame)->condition = condition;
											return VALUE_NOT_USED;
										})));
				stackFrame->mode = WhileCompilerStackFrameMode::EXPECT_CP;
				goto parseChar;
			}
		} else if (stackFrame->mode == WhileCompilerStackFrameMode::EXPECT_CP) {
			if (c == ')') {
				stackFrame->mode = WhileCompilerStackFrameMode::EXPECT_BODY;
			} else {
				return "expected ) while parsing while";
			}
		} else if (stackFrame->mode
				== WhileCompilerStackFrameMode::EXPECT_BODY) {
			if (c == '{') {
				stackFrame->mode = WhileCompilerStackFrameMode::DONE;
				state->compilerStack->push(
						new BodyCompilerStackFrame(
								CodeCallback(stackFrame,
										[](void* stackFrame, Code* code) -> void* {
											((WhileCompilerStackFrame*)stackFrame)->code = code;
											return VALUE_NOT_USED;
										})));
			} else {
				return "expected {";
			}
		} else if (stackFrame->mode == WhileCompilerStackFrameMode::DONE) {
			stackFrame->whileBytecodeCallback(
					new WhileBytecode(stackFrame->condition, stackFrame->code,
							stackFrame->isDoWhile));
			delete state->compilerStack->pop();
		} else {
			crash("unknown WhileCompilerStackFrameMode");
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::SYMBOL) {
		SymbolCompilerStackFrame* stackFrame =
				(SymbolCompilerStackFrame*) state->compilerStack->peek();

		if (isValidSymbolChar(c)) {
			stackFrame->symbol->add(c);
		} else {
			stackFrame->symbolCallback(charListToString(stackFrame->symbol));
			delete state->compilerStack->pop();

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
				stackFrame->stringCallback(
						charListToString(stackFrame->string));
				delete state->compilerStack->pop();
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

		if (stackFrame->mode == ExpressionCompilerStackFrameMode::READY) {
			if (c == '\'') {
				// begin string
				state->compilerStack->push(
						new StringCompilerStackFrame(
								StringCallback(stackFrame,
										[](void* stackFrame, String string) -> void* {
											((ExpressionCompilerStackFrame*)stackFrame)->expressionCallback(new StringValue(string, true));
											return VALUE_NOT_USED;
										})));
			} else if (isValidSymbolChar(c)) {
				state->compilerStack->push(
						new SymbolCompilerStackFrame(
								StringCallback(
										new ExpressionCompilerStackFrameStringCallbackStruct(
												stackFrame, state),
										[](void* stateStructArgument, String symbol) -> void* {
											ExpressionCompilerStackFrameStringCallbackStruct* stateStruct = (ExpressionCompilerStackFrameStringCallbackStruct*) stateStructArgument;
											if (strequ(symbol, "true")) {
												stateStruct->stackFrame->expressionCallback(new BooleanValue(true, true));
												delete symbol;
											} else if (strequ(symbol, "false")) {
												stateStruct->stackFrame->expressionCallback(new BooleanValue(false, true));
												delete symbol;
											} else {
												stateStruct->stackFrame->mode = ExpressionCompilerStackFrameMode::SYMBOL;
												stateStruct->stackFrame->symbol = symbol;
											}
											//delete stateStruct->state->compilerStack->pop();
											delete stateStruct;
											return VALUE_NOT_USED;
										})));
				goto parseChar;
			} else if (c == '(') {
				return "parenthesis in expression not implemented yet";
				state->compilerStack->push(
						new ExpressionCompilerStackFrame(
								ExpressionCallback(stackFrame,
										[](void* stackFrame, Expression* expression) -> void* {
											// TODO do something with expression
											// TODO maybe expression stack frame has to have close parenthesis flag?
											CUNUSED(stackFrame);
											CUNUSED(expression);
											NIMPL;
											return VALUE_NOT_USED;
										})));
			} else if (c == ')') {
				if (stackFrame->symbol != NULL) {
					return "expression symbols not implemented yet";
				}

				delete state->compilerStack->pop();
				goto parseChar;
			} else {
				return "unexpected character while parsing expression";
			}
		} else if (stackFrame->mode
				== ExpressionCompilerStackFrameMode::SYMBOL) {
			// function
			state->compilerStack->push(
					new FunctionCallCompilerStackFrame(stackFrame->symbol,
							FunctionCallExpressionCallback(stackFrame,
									[](void* stackFrame, FunctionCallReturn* functionCallExpression) -> void* {
										((ExpressionCompilerStackFrame*)stackFrame)->expressionCallback(functionCallExpression);
										return VALUE_NOT_USED;
									})));
			stackFrame->symbol = NULL;
			stackFrame->mode = ExpressionCompilerStackFrameMode::DONE;
			goto parseChar;
			// TODO variables
		} else if (stackFrame->mode == ExpressionCompilerStackFrameMode::DONE) {
			delete state->compilerStack->pop();
			goto parseChar;
		} else {
			crash("unknown ExpressionCompilerStackFrameMode");
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
							ExpressionCallback(stackFrame,
									[](void* stackFrame, Expression* expression) -> void* {
										((ArgumentCompilerStackFrame*)stackFrame)->argumentCallback(expression);
										return VALUE_NOT_USED;
									})));
			goto parseChar;
		}
	} else if (state->compilerStack->peek()->type
			== CompilerStackFrameType::FUNCTION_CALL) {
		FunctionCallCompilerStackFrame* stackFrame =
				(FunctionCallCompilerStackFrame*) state->compilerStack->peek();

		if (stackFrame->mode == FunctionCallCompilerStackFrameMode::EXPECT_P) {
			if (c == '(') {
				stackFrame->mode =
						FunctionCallCompilerStackFrameMode::ARGUMENTS;
			}
		} else if (stackFrame->mode
				== FunctionCallCompilerStackFrameMode::ARGUMENTS) {
			if (c == ')') {
				// search for the function
				String functionName = stackFrame->name;

				// determine if this is a syscall
				List<Function*>* functions;
				if (stringStartsWith(functionName, "__")) {
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
					if (strequ(function->name, functionName)) {
						// check parameter sizes
						if (stackFrame->arguments->size()
								!= function->parameterTypes->size()) {
							continue;
						}

						// check parameter types
						Iterator<Expression*> argumentsIterator =
								stackFrame->arguments->iterator();
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
				//delete functionName;

				if (!found) {
					return "syscall not found";
				}

				// create and add the function call
				if (stackFrame->type
						== FunctionCallCompilerStackFrameType::BYTECODE) {
					stackFrame->functionCallBytecodeCallback(
							new FunctionCallVoid(function,
									stackFrame->arguments));
				} else if (stackFrame->type
						== FunctionCallCompilerStackFrameType::EXPRESSION) {
					stackFrame->functionCallExpressionCallback(
							new FunctionCallReturn(function,
									stackFrame->arguments));
				} else {
					crash("unknown FunctionCallCompilerStackFrameType");
				}

				delete state->compilerStack->pop();
			} else {
				state->compilerStack->push(
						new ExpressionCompilerStackFrame(
								ExpressionCallback(stackFrame,
										[](void* stackFrame, Expression* argument) -> void* {
											((FunctionCallCompilerStackFrame*)stackFrame)->arguments->add(argument);
											return VALUE_NOT_USED;
										})));
				goto parseChar;
			}
		} else {
			crash("unknown FunctionCallCompilerStackFrameMode");
		}
	} else {
		crash("unknown CompilerStackFrameType");
	}

// no error message, return null
	return NULL;
}

Code* mish_compile(String code) {
	return mish_compile(code, strlen(code));
}

Code* mish_compile(String sourceCode, size size) {
	CompilerState* state = new CompilerState();
	Code* compiledCode;
	state->compilerStack->push(
			new BodyCompilerStackFrame(
					CodeCallback(&compiledCode,
							[](void* compiledCode, Code* code) -> void* {
								*((Code**)compiledCode) = code;
								return NULL;
							})));

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

	CompilerStackFrameType endFrameType = state->compilerStack->peek()->type;
	if (errorMessage == NULL && endFrameType != CompilerStackFrameType::BODY
			&& endFrameType != CompilerStackFrameType::COMMENT) {
// something wasn't properly closed, throw a generic error for now
		debug("parse mode", (uint64) endFrameType);
		errorMessage = "incorrect parse mode";
		hasError = true;
	}

	if (hasError) {
		if (lineEnd == NULL) {
			lineEnd = i;
		}

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
	Code* code = ((BodyCompilerStackFrame*) state->compilerStack->peek())->code;
	delete state;

	return code;
}
