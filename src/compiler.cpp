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

namespace mish {

namespace compile {

const Status Status::OK(Type::OK);
const Status Status::REPROCESS(Type::REPROCESS);

Status::Status(Type type) {
	this->type = type;
	this->message = NULL;
}

static bool isValidSymbolChar(strchar c) {
	return arrayContains<strchar>((strchar*) VALID_SYMBOL_CHARS,
			sizeof(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(strchar c) {
	return arrayContains<strchar>((strchar*) WHITESPACE_CHARS,
			sizeof(WHITESPACE_CHARS), c);
}

// CompilerStackFrame
CompilerStackFrame::CompilerStackFrame(CompilerStackFrameType type,
		CompilerState* state) {
	this->type = type;
	this->state = state;
}

CompilerStackFrame::~CompilerStackFrame() {

}

// BodyCompilerStackFrame
BodyCompilerStackFrame::BodyCompilerStackFrame(
		Callback<void(Code*)> codeCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::BODY, state) {
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

Status BodyCompilerStackFrame::processCharacter(strchar c) {
	if (mode == BodyCompilerStackFrameMode::READY) {
		if (c == ';' || c == '\n') {
			lastWasTerminated = true;
			return Status::OK;
		} else if (isWhitespace(c)) {
			return Status::OK;
		} else if (c == '}') {
			codeCallback(code);
			delete state->compilerStack->pop();
			// the statement that created this body needs to be closed, run its closing code
			goto parseChar;
			// TODO potential issue with closing multiple bodys...maybe
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			mode = BodyCompilerStackFrameMode::SYMBOL;
			state->compilerStack->push(new SymbolCompilerStackFrame(
			BIND_MEM_CB(&BodyCompilerStackFrame::setSymbol, this), state));
			goto parseChar;
		} else if (c == '#') {
			state->compilerStack->push(
					new CommentCompilerStackFrame(
							CommentCompilerStackFrameType::LINE, state));
		} else if (c == EOF) {
			return Status::OK;
		} else {
			return Status::ERROR("unexpected character");
		}
	} else if (mode == BodyCompilerStackFrameMode::SYMBOL) {
		// skip any whitespace
		if (isWhitespace(c)) {
			return Status::OK;
		}

		if (strequ(symbol, "while")) {
			if (!lastWasTerminated) {
				return Status::ERROR("last statement wasn't terminated");
			}

			// while
			delete symbol;

			state->compilerStack->push(
					new WhileCompilerStackFrame(false,
							BIND_MEM_CB((void (BodyCompilerStackFrame::*)(WhileBytecode*))&BodyCompilerStackFrame::addBytecode, this),
							state));
		} else if (strequ(symbol, "dowhile")) {
			if (!lastWasTerminated) {
				return Status::ERROR("last statement wasn't terminated");
			}

			// dowhile
			delete symbol;

			state->compilerStack->push(
					new WhileCompilerStackFrame(true,
							BIND_MEM_CB((void (BodyCompilerStackFrame::*)(WhileBytecode*))&BodyCompilerStackFrame::addBytecode, this),
							state));
		} else if (strequ(symbol, "if")) {
			if (!lastWasTerminated) {
				return Status::ERROR("last statement wasn't terminated");
			}

			// if
			delete symbol;

			state->compilerStack->push(
					new IfCompilerStackFrame(
							BIND_MEM_CB((void (BodyCompilerStackFrame::*) (IfBytecode*))&BodyCompilerStackFrame::addBytecode, this),
							state));
		} else if (strequ(symbol, "elseif")) {
			// elseif
			delete symbol;

			Bytecode* lastBytecode = code->bytecodes->getLast();

			if (lastBytecode != NULL
					&& lastBytecode->type == BytecodeType::IF) {
				if (lastWasTerminated) {
					return Status::ERROR(
							"may not terminate an if statement before an elseif statement");
				}

				state->compilerStack->push(
						new IfCompilerStackFrame(
								IfCompilerStackFrameType::ELSEIF,
								(IfBytecode*) lastBytecode, state));
			} else {
				return Status::ERROR(
						"elseif statement may only follow an if statement");
			}
		} else if (strequ(symbol, "else")) {
			// else
			delete symbol;

			Bytecode* lastBytecode = code->bytecodes->getLast();

			if (lastBytecode != NULL
					&& lastBytecode->type == BytecodeType::IF) {
				if (lastWasTerminated) {
					return Status::ERROR(
							"may not terminate an if statement before and else statement");
				}

				state->compilerStack->push(
						new IfCompilerStackFrame(IfCompilerStackFrameType::ELSE,
								(IfBytecode*) lastBytecode, state));
			} else {
				return Status::ERROR(
						"else statement may only follow an if statement");
			}
		} else {
			if (!lastWasTerminated) {
				return Status::ERROR("last statement wasn't terminated");
			}

			// function
			state->compilerStack->push(
					new FunctionCallCompilerStackFrame(symbol,
							BIND_MEM_CB((void (BodyCompilerStackFrame::*)(FunctionCallVoid*))&BodyCompilerStackFrame::addBytecode,
									this), state));
		}
		symbol = NULL;
		mode = BodyCompilerStackFrameMode::READY;
		lastWasTerminated = false;

		goto parseChar;
	} else {
		crash("unknown BodyCompilerStackFrameMode");
	}
	return Status::OK;
}

void BodyCompilerStackFrame::addBytecode(Bytecode* bytecode) {
	code->bytecodes->add(bytecode);
}

void BodyCompilerStackFrame::setSymbol(String symbol) {
	this->symbol = symbol;
}

// IfCompilerStackFrame
IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type,
		CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::IF, state) {
	this->type = type;
	mode = IfCompilerStackFrameMode::EXPECT_P;
	condition = NULL;
	code = NULL;
	ifBytecode = NULL;
}

IfCompilerStackFrame::IfCompilerStackFrame(
		Callback<void(IfBytecode*)> ifBytecodeCallback, CompilerState* state) :
		IfCompilerStackFrame(IfCompilerStackFrameType::IF, state) {
	this->mode = IfCompilerStackFrameMode::EXPECT_P;
	this->ifBytecodeCallback = ifBytecodeCallback;
}

IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type,
		IfBytecode* ifBytecode, CompilerState* state) :
		IfCompilerStackFrame(type, state) {
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

Status IfCompilerStackFrame::processCharacter(strchar c) {
	if (mode == IfCompilerStackFrameMode::EXPECT_P) {
		if (c == '(') {
			mode = IfCompilerStackFrameMode::EXPECT_CONDITION;
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == IfCompilerStackFrameMode::EXPECT_CONDITION) {
		if (c == ')') {
			return Status::ERROR("expected condition");
		} else {
			state->compilerStack->push(new ExpressionCompilerStackFrame(
			BIND_MEM_CB(&IfCompilerStackFrame::setCondition, this), state));
			mode = IfCompilerStackFrameMode::EXPECT_CP;
			goto parseChar;
		}
	} else if (mode == IfCompilerStackFrameMode::EXPECT_CP) {
		if (c == ')') {
			mode = IfCompilerStackFrameMode::EXPECT_BODY;
		} else {
			return Status::ERROR("expected )");
		}
	} else if (mode == IfCompilerStackFrameMode::EXPECT_BODY) {
		if (c == '{') {
			mode = IfCompilerStackFrameMode::DONE;
			state->compilerStack->push(new BodyCompilerStackFrame(
			BIND_MEM_CB(&IfCompilerStackFrame::setCode, this), state));
		} else {
			return Status::ERROR("expected {");
		}
	} else if (mode == IfCompilerStackFrameMode::DONE) {
		IfConditionCode* ifConditionCode = new IfConditionCode(condition, code);
		if (type == IfCompilerStackFrameType::IF) {
			ifBytecodeCallback(new IfBytecode(ifConditionCode));
		} else if (type == IfCompilerStackFrameType::ELSEIF
				|| type == IfCompilerStackFrameType::ELSE) {
			ifBytecode->ifs->add(ifConditionCode);
		} else {
			crash("unknown IfCompilerStackFrameType");
		}
		delete state->compilerStack->pop();
	} else {
		crash("unknown IfCompilerStackFrameMode");
	}

	return Status::OK;
}

void IfCompilerStackFrame::setCondition(Expression* condition) {
	this->condition = condition;
}

void IfCompilerStackFrame::setCode(Code* code) {
	this->code = code;
}

// WhileCompilerStackFrame
WhileCompilerStackFrame::WhileCompilerStackFrame(bool isDoWhile,
		Callback<void(WhileBytecode*)> whileBytecodeCallback,
		CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::WHILE, state) {
	this->mode = WhileCompilerStackFrameMode::EXPECT_P;
	this->isDoWhile = isDoWhile;
	this->whileBytecodeCallback = whileBytecodeCallback;

	condition = NULL;
	code = NULL;
}

WhileCompilerStackFrame::~WhileCompilerStackFrame() {

}

Status WhileCompilerStackFrame::processCharacter(strchar c) {
	if (mode == WhileCompilerStackFrameMode::EXPECT_P) {
		if (c == '(') {
			mode = WhileCompilerStackFrameMode::EXPECT_CONDITION;
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == WhileCompilerStackFrameMode::EXPECT_CONDITION) {
		if (c == ')') {
			return Status::ERROR("expected condition");
		} else {
			state->compilerStack->push(new ExpressionCompilerStackFrame(
			BIND_MEM_CB(&WhileCompilerStackFrame::setCondition, this), state));
			mode = WhileCompilerStackFrameMode::EXPECT_CP;
			goto parseChar;
		}
	} else if (mode == WhileCompilerStackFrameMode::EXPECT_CP) {
		if (c == ')') {
			mode = WhileCompilerStackFrameMode::EXPECT_BODY;
		} else {
			return Status::ERROR("expected ) while parsing while");
		}
	} else if (mode == WhileCompilerStackFrameMode::EXPECT_BODY) {
		if (c == '{') {
			mode = WhileCompilerStackFrameMode::DONE;
			state->compilerStack->push(new BodyCompilerStackFrame(
			BIND_MEM_CB(&WhileCompilerStackFrame::setCode, this), state));
		} else {
			return Status::ERROR("expected {");
		}
	} else if (mode == WhileCompilerStackFrameMode::DONE) {
		whileBytecodeCallback(new WhileBytecode(condition, code, isDoWhile));
		delete state->compilerStack->pop();
	} else {
		crash("unknown WhileCompilerStackFrameMode");
	}

	return Status::OK;
}

void WhileCompilerStackFrame::setCondition(Expression* condition) {
	this->condition = condition;
}

void WhileCompilerStackFrame::setCode(Code* code) {
	this->code = code;
}

// SymbolCompilerStackFrame
SymbolCompilerStackFrame::SymbolCompilerStackFrame(
		Callback<void(String)> symbolCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::SYMBOL, state) {
	symbol = new List<strchar>();
	this->symbolCallback = symbolCallback;
}

SymbolCompilerStackFrame::~SymbolCompilerStackFrame() {
	delete symbol;
}

Status SymbolCompilerStackFrame::processCharacter(strchar c) {
	if (isValidSymbolChar(c)) {
		symbol->add(c);
	} else {
		delete state->compilerStack->pop();
		symbolCallback(charListToString(symbol));

		// we havn't done anything with this char, so we have to re-parse it
		return Status::REPROCESS;
	}

	return Status::OK;
}

// StringCompilerStackFrame
StringCompilerStackFrame::StringCompilerStackFrame(
		Callback<void(String)> stringCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::STRING, state) {
	string = new List<strchar>();
	escaping = false;
	this->stringCallback = stringCallback;
}

StringCompilerStackFrame::~StringCompilerStackFrame() {
	delete string;
}

Status StringCompilerStackFrame::processCharacter(strchar c) {
	if (escaping) {
		if (c == 'n') {
			// new line escape sequence
			string->add('\n');
			string->add('\r');
		} else if (c == '\\') {
			// backslash escape sequence
			string->add('\\');
		} else if (c == '\'') {
			// quote escape sequence
			string->add('\'');
		} else if (c == EOF) {
			return Status::ERROR("unexpected EOF");
		} else {
			return Status::ERROR("unrecognized escape sequence");
		}

		// done with escape
		escaping = false;
	} else {
		if (c == '\\') {
			// begin escape
			escaping = true;
		} else if (c == '\'') {
			stringCallback(charListToString(string));
			delete state->compilerStack->pop();
		} else if (c == EOF) {
			return Status::ERROR("unexpected EOF");
		} else {
			// just another character
			string->add(c);
		}
	}

	return Status::OK;
}

// CommentCompilerStackFrame
CommentCompilerStackFrame::CommentCompilerStackFrame(
		CommentCompilerStackFrameType type, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::COMMENT, state) {
	this->type = type;
}

CommentCompilerStackFrame::~CommentCompilerStackFrame() {

}

Status CommentCompilerStackFrame::processCharacter(strchar c) {
	if (type == CommentCompilerStackFrameType::LINE) {
		if (c == '\n') {
			delete state->compilerStack->pop();
		}
		// ignore character
	} else {
		crash("unknown CommentCompilerStackFrameType");
	}

	return Status::OK;
}

// FunctionCallCompilerStackFrame
FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		FunctionCallCompilerStackFrameType type, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::FUNCTION_CALL, state) {
	this->name = name;
	arguments = new List<Expression*>();
	this->type = type;
	mode = FunctionCallCompilerStackFrameMode::EXPECT_P;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		Callback<void(FunctionCallVoid*)> functionCallBytecodeCallback,
		CompilerState* state) :
		FunctionCallCompilerStackFrame(name,
				FunctionCallCompilerStackFrameType::BYTECODE, state) {
	this->functionCallBytecodeCallback = functionCallBytecodeCallback;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		Callback<void(Expression*)> functionCallExpressionCallback,
		CompilerState* state) :
		FunctionCallCompilerStackFrame(name,
				FunctionCallCompilerStackFrameType::EXPRESSION, state) {
	this->functionCallExpressionCallback = functionCallExpressionCallback;
}

FunctionCallCompilerStackFrame::~FunctionCallCompilerStackFrame() {
	delete name;
}

Status FunctionCallCompilerStackFrame::processCharacter(strchar c) {
	if (mode == FunctionCallCompilerStackFrameMode::EXPECT_P) {
		if (c == '(') {
			mode = FunctionCallCompilerStackFrameMode::ARGUMENTS;
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == FunctionCallCompilerStackFrameMode::ARGUMENTS) {
		if (c == ')') {
			// search for the function
			String functionName = name;

			// determine if this is a syscall
			List<Function*>* functions;
			if (stringStartsWith(functionName, "__")) {
				functions = &mish_syscalls;
			} else {
				// TODO regular function
				return Status::ERROR("regular functions not implemented yet");
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
					if (arguments->size() != function->parameterTypes->size()) {
						continue;
					}

					// check parameter types
					Iterator<Expression*> argumentsIterator =
							arguments->iterator();
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
				return Status::ERROR("syscall not found");
			}

			delete state->compilerStack->pop();

			// create and add the function call
			if (type == FunctionCallCompilerStackFrameType::BYTECODE) {
				functionCallBytecodeCallback(
						new FunctionCallVoid(function, arguments));
			} else if (type == FunctionCallCompilerStackFrameType::EXPRESSION) {
				functionCallExpressionCallback(
						new FunctionCallReturn(function, arguments));
			} else {
				crash("unknown FunctionCallCompilerStackFrameType");
			}
		} else {
			state->compilerStack->push(
					new ExpressionCompilerStackFrame(
							BIND_MEM_CB(&FunctionCallCompilerStackFrame::argumentCallback, this),
							state));
			goto parseChar;
		}
	} else {
		crash("unknown FunctionCallCompilerStackFrameMode");
	}

	return Status::OK;
}

void FunctionCallCompilerStackFrame::argumentCallback(Expression* argument) {
	arguments->add(argument);
}

// ExpressionCompilerStackFrame
ExpressionCompilerStackFrame::ExpressionCompilerStackFrame(
		Callback<void(Expression*)> expressionCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::EXPRESSION, state) {
	this->expressionCallback = expressionCallback;
	symbol = NULL;
	mode = ExpressionCompilerStackFrameMode::READY;
}

ExpressionCompilerStackFrame::~ExpressionCompilerStackFrame() {
	if (symbol != NULL) {
		delete symbol;
	}
}

Status ExpressionCompilerStackFrame::processCharacter(strchar c) {
	if (mode == ExpressionCompilerStackFrameMode::READY) {
		if (c == '\'') {
			// begin string
			state->compilerStack->push(
					new StringCompilerStackFrame(
							BIND_MEM_CB(&ExpressionCompilerStackFrame::stringCallback, this),
							state));
		} else if (isValidSymbolChar(c)) {
			state->compilerStack->push(
					new SymbolCompilerStackFrame(
							BIND_MEM_CB(&ExpressionCompilerStackFrame::symbolCallback, this),
							state));
			return Status::REPROCESS;
		} else if (c == '(') {
			return Status::ERROR(
					"parenthesis in expression not implemented yet");
			state->compilerStack->push(
					new ExpressionCompilerStackFrame(
							BIND_MEM_CB(&ExpressionCompilerStackFrame::subexpressionCallback, this),
							state));
		} else if (c == ')') {
			if (symbol != NULL) {
				return Status::ERROR("expression symbols not implemented yet");
			}

			delete state->compilerStack->pop();
			goto parseChar;
		} else {
			return Status::ERROR(
					"unexpected character while parsing expression");
		}
	} else if (mode == ExpressionCompilerStackFrameMode::SYMBOL) {
		// function
		state->compilerStack->push(
				new FunctionCallCompilerStackFrame(symbol,
						BIND_MEM_CB(&ExpressionCompilerStackFrame::functionCallback, this),
						state));
		symbol = NULL;
		return Status::REPROCESS;
		// TODO variables
	} else {
		crash("unknown ExpressionCompilerStackFrameMode");
	}

	return Status::OK;
}

void ExpressionCompilerStackFrame::stringCallback(String string) {
	expressionCallback(new StringValue(string, true));
}

void ExpressionCompilerStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "true")) {
		expressionCallback(new BooleanValue(true, true));
		delete symbol;
	} else if (strequ(symbol, "false")) {
		expressionCallback(new BooleanValue(false, true));
		delete symbol;
	} else {
		mode = ExpressionCompilerStackFrameMode::SYMBOL;
		this->symbol = symbol;
	}
}

void ExpressionCompilerStackFrame::subexpressionCallback(
		Expression* expression) {
	CUNUSED(expression);
	NIMPL;
}

void ExpressionCompilerStackFrame::functionCallback(
		FunctionCallReturn* funcCall) {
	delete state->compilerStack->pop();
	expressionCallback(funcCall);
}

// ArgumentCompilerStackFrame
ArgumentCompilerStackFrame::ArgumentCompilerStackFrame(
		Callback<void(Expression*)> argumentCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::ARGUMENT, state) {
	this->argumentCallback = argumentCallback;
	requireArgument = false;
}

ArgumentCompilerStackFrame::~ArgumentCompilerStackFrame() {

}

Status ArgumentCompilerStackFrame::processCharacter(strchar c) {
	if (c == ',') {
		requireArgument = true;
	} else if (c == ')') {
		if (requireArgument) {
			return Status::ERROR("expected argument");
		}

		delete state->compilerStack->pop();
	} else {
		requireArgument = false;
		state->compilerStack->push(
				new ExpressionCompilerStackFrame(argumentCallback, state));
		goto parseChar;
	}

	return Status::OK;
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

Code* mish_compile(String code) {
	return mish_compile(code, strlen(code));
}

Code* mish_compile(String sourceCode, size size) {
	CompilerState* state = new CompilerState();
	state->compilerStack->push(new BodyCompilerStackFrame( { }, state));

// line stuff
	uint64 lineStart = 0;
	uint64 lineEnd = NULL;
	uint64 lineNumber = 1;

// error message stuff
	bool hasError = false;
	uint64 errorPosition = NULL;
	Status status = Status::OK;

	uinteger i = 0;
	for (; i < size + 1; i++) {
		// get the next char to process
		strchar c = ((uint8) c == size ? EOF : sourceCode[i]);
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
			if (state->compilerStack->peek()->type
					!= CompilerStackFrameType::SYMBOL
					&& state->compilerStack->peek()->type
							!= CompilerStackFrameType::STRING
					&& state->compilerStack->peek()->type
							!= CompilerStackFrameType::BODY) {

				if (isWhitespace(c)) {
					continue;
				}
			}

			status = state->compilerStack->peek()->processCharacter(c);

			if (status == Status::REPROCESS) {
				continue;
			} else if (status != Status::OK) {
				hasError = true;
				errorPosition = i;
			}
		}
	}

	CompilerStackFrameType endFrameType = state->compilerStack->peek()->type;
	if (status == Status::OK && endFrameType != CompilerStackFrameType::BODY
			&& endFrameType != CompilerStackFrameType::COMMENT) {
		// something wasn't properly closed, throw a generic error for now
		debug("parse mode", (uint64) endFrameType);
		status = Status::ERROR("incorrect parse mode");
		hasError = true;
		errorPosition = i;
	}

	if (hasError) {
		if (lineEnd == NULL) {
			lineEnd = i;
		}

		debug("lineStart", lineStart);
		debug("lineEnd", lineEnd);

		// generate the error message and display it
		String line = substring(sourceCode, lineStart, lineEnd);

		uinteger markerLength = lineEnd - lineStart
				+ (errorPosition == size ? 1 : 0);

		strchar* errorPositionMarker = (strchar*) create(
				markerLength * sizeof(strchar) + 1);
		uint64 i = 0;
		for (; i < markerLength; i++) {
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
		fault(status.message);

		// delete error message stuff
		delete line;
		delete errorPositionMarker;
		//delete status;

		delete state;
		return NULL;
	}

	// the code has been compiled, return it
	Code* code = ((BodyCompilerStackFrame*) state->compilerStack->peek())->code;
	delete state;

	return code;
}

}

}
