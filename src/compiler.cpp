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

Status::Status(Type type, String message) {
	this->type = type;
	this->message = message;
}

static bool isValidSymbolChar(strchar c) {
	return arrayContains<strchar>((strchar*) VALID_SYMBOL_CHARS,
			sizeof(VALID_SYMBOL_CHARS), c);
}

static bool isWhitespace(strchar c) {
	return arrayContains<strchar>((strchar*) WHITESPACE_CHARS,
			sizeof(WHITESPACE_CHARS), c);
}

template<typename ... Args>
static Status callbackAndEndFrame(Callback<Status(Args...)> callback,
		CompilerState* state, Args ... args) {
	CompilerStackFrame* stackFrame = state->compilerStack->pop();
	callback(args...);
	delete stackFrame;
	return Status::OK;
}

static Status endFrame(CompilerState* state) {
	delete state->compilerStack->pop();
	return Status::OK;
}

// CompilerStackFrame
CompilerStackFrame::CompilerStackFrame(CompilerStackFrameType type,
		CompilerState* state) {
	this->type = type;
	this->state = state;
}

CompilerStackFrame::~CompilerStackFrame() {

}

Status CompilerStackFrame::processCharacter(strchar c) {
	// no-op
	UNUSED(c);
	return Status::OK;
}

// BodyCompilerStackFrame
BodyCompilerStackFrame::BodyCompilerStackFrame(
		Callback<Status(Code*)> codeCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::BODY, state) {
	mode = BodyCompilerStackFrameMode::READY;
	code = new Code();
	this->codeCallback = codeCallback;
	lastWasTerminated = true;
}

BodyCompilerStackFrame::~BodyCompilerStackFrame() {

}

Status BodyCompilerStackFrame::processCharacter(strchar c) {
	if (mode == BodyCompilerStackFrameMode::READY) {
		if (c == ';' || c == '\n') {
			lastWasTerminated = true;
			return Status::OK;
		} else if (isWhitespace(c)) {
			return Status::OK;
		} else if (c == '}') {
			return callbackAndEndFrame(codeCallback, state, code);
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			state->compilerStack->push(new SymbolCompilerStackFrame(
			BIND_MEM_CB(&BodyCompilerStackFrame::symbolCallback, this), state));
			return Status::REPROCESS;
		} else if (c == '#') {
			state->compilerStack->push(
					new CommentCompilerStackFrame(
							CommentCompilerStackFrameType::LINE, state));
		} else if (c == EOF) {
			return Status::OK;
		} else {
			return Status::ERROR("unexpected character");
		}
	} else {
		crash("unknown BodyCompilerStackFrameMode");
	}
	return Status::OK;
}

Status BodyCompilerStackFrame::bytecodeCallback(Bytecode* bytecode) {
	code->bytecodes->add(bytecode);
	return Status::OK;
}

Status BodyCompilerStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "while")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// while
		delete symbol;

		state->compilerStack->push(
				new WhileCompilerStackFrame(false,
						BIND_MEM_CB((Status(BodyCompilerStackFrame::*)(WhileBytecode*))&BodyCompilerStackFrame::bytecodeCallback, this),
						state));
	} else if (strequ(symbol, "dowhile")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// dowhile
		delete symbol;

		state->compilerStack->push(
				new WhileCompilerStackFrame(true,
						BIND_MEM_CB((Status (BodyCompilerStackFrame::*)(WhileBytecode*))&BodyCompilerStackFrame::bytecodeCallback, this),
						state));
	} else if (strequ(symbol, "if")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// if
		delete symbol;

		state->compilerStack->push(
				new IfCompilerStackFrame(
						BIND_MEM_CB((Status (BodyCompilerStackFrame::*) (IfBytecode*))&BodyCompilerStackFrame::bytecodeCallback, this),
						state));
	} else if (strequ(symbol, "elseif")) {
		// elseif
		delete symbol;

		Bytecode* lastBytecode = code->bytecodes->getLast();

		if (lastBytecode != NULL && lastBytecode->type == BytecodeType::IF) {
			if (lastWasTerminated) {
				return Status::ERROR(
						"may not terminate an if statement before an elseif statement");
			}

			state->compilerStack->push(
					new IfCompilerStackFrame(IfCompilerStackFrameType::ELSEIF,
							(IfBytecode*) lastBytecode, state));
		} else {
			return Status::ERROR(
					"elseif statement may only follow an if statement");
		}
	} else if (strequ(symbol, "else")) {
		// else
		delete symbol;

		Bytecode* lastBytecode = code->bytecodes->getLast();

		if (lastBytecode != NULL && lastBytecode->type == BytecodeType::IF) {
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
						BIND_MEM_CB((Status (BodyCompilerStackFrame::*)(FunctionCallVoid*))&BodyCompilerStackFrame::bytecodeCallback,
								this), state));
	}
	mode = BodyCompilerStackFrameMode::READY;
	lastWasTerminated = false;

	return Status::REPROCESS;
}

// IfCompilerStackFrame
IfCompilerStackFrame::IfCompilerStackFrame(IfCompilerStackFrameType type,
		CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::IF, state) {
	this->type = type;
	mode = IfCompilerStackFrameMode::EXPECT_P;
	condition = NULL;
	ifBytecode = NULL;
}

IfCompilerStackFrame::IfCompilerStackFrame(
		Callback<Status(IfBytecode*)> ifBytecodeCallback, CompilerState* state) :
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
			mode = IfCompilerStackFrameMode::EXPECT_BODY;
			state->compilerStack->push(
					new ArgumentsCompilerStackFrame(
					BIND_MEM_CB(&IfCompilerStackFrame::conditionCallback, this),
							state));
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == IfCompilerStackFrameMode::EXPECT_BODY) {
		if (c == '{') {
			state->compilerStack->push(new BodyCompilerStackFrame(
			BIND_MEM_CB(&IfCompilerStackFrame::codeCallback, this), state));
		} else {
			return Status::ERROR("expected {");
		}
	} else {
		crash("unknown IfCompilerStackFrameMode");
	}

	return Status::OK;
}

Status IfCompilerStackFrame::conditionCallback(List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("if expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK;
	}
}

Status IfCompilerStackFrame::codeCallback(Code* code) {
	IfConditionCode* ifConditionCode = new IfConditionCode(condition, code);
	if (type == IfCompilerStackFrameType::IF) {
		return callbackAndEndFrame(ifBytecodeCallback, state,
				new IfBytecode(ifConditionCode));
	} else if (type == IfCompilerStackFrameType::ELSEIF
			|| type == IfCompilerStackFrameType::ELSE) {
		ifBytecode->ifs->add(ifConditionCode);
	} else {
		crash("unknown IfCompilerStackFrameType");
	}

	return endFrame(state);
}

// WhileCompilerStackFrame
WhileCompilerStackFrame::WhileCompilerStackFrame(bool isDoWhile,
		Callback<Status(WhileBytecode*)> whileBytecodeCallback,
		CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::WHILE, state) {
	this->mode = WhileCompilerStackFrameMode::EXPECT_P;
	this->isDoWhile = isDoWhile;
	this->whileBytecodeCallback = whileBytecodeCallback;

	condition = NULL;
}

WhileCompilerStackFrame::~WhileCompilerStackFrame() {

}

Status WhileCompilerStackFrame::processCharacter(strchar c) {
	if (mode == WhileCompilerStackFrameMode::EXPECT_P) {
		if (c == '(') {
			mode = WhileCompilerStackFrameMode::EXPECT_BODY;
			state->compilerStack->push(
					new ArgumentsCompilerStackFrame(
							BIND_MEM_CB(&WhileCompilerStackFrame::conditionCallback, this),
							state));
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == WhileCompilerStackFrameMode::EXPECT_BODY) {
		if (c == '{') {
			state->compilerStack->push(new BodyCompilerStackFrame(
			BIND_MEM_CB(&WhileCompilerStackFrame::codeCallback, this), state));
		} else {
			return Status::ERROR("expected {");
		}
	} else {
		crash("unknown WhileCompilerStackFrameMode");
	}

	return Status::OK;
}

Status WhileCompilerStackFrame::conditionCallback(
		List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("while expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK;
	}
}

Status WhileCompilerStackFrame::codeCallback(Code* code) {
	return callbackAndEndFrame(whileBytecodeCallback, state,
			new WhileBytecode(condition, code, isDoWhile));
}

// SymbolCompilerStackFrame
SymbolCompilerStackFrame::SymbolCompilerStackFrame(
		Callback<Status(String)> symbolCallback, CompilerState* state) :
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
		Status status = callbackAndEndFrame(symbolCallback, state,
				charListToString(symbol));

		if (status != Status::OK) {
			return status;
		} else {
			// we havn't done anything with this char, so we have to re-parse it
			return Status::REPROCESS;
		}
	}

	return Status::OK;
}

// StringCompilerStackFrame
StringCompilerStackFrame::StringCompilerStackFrame(
		Callback<Status(String)> stringCallback, CompilerState* state) :
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
			return callbackAndEndFrame(stringCallback, state,
					charListToString(string));
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
			endFrame(state);
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
	this->type = type;
	mode = FunctionCallCompilerStackFrameMode::EXPECT_P;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		Callback<Status(FunctionCallVoid*)> functionCallBytecodeCallback,
		CompilerState* state) :
		FunctionCallCompilerStackFrame(name,
				FunctionCallCompilerStackFrameType::BYTECODE, state) {
	this->functionCallBytecodeCallback = functionCallBytecodeCallback;
}

FunctionCallCompilerStackFrame::FunctionCallCompilerStackFrame(String name,
		Callback<Status(FunctionCallReturn*)> functionCallExpressionCallback,
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
			state->compilerStack->push(
					new ArgumentsCompilerStackFrame(
							BIND_MEM_CB(&FunctionCallCompilerStackFrame::argumentsCallback, this),
							state));
		} else {
			return Status::ERROR("expected (");
		}
	} else {
		crash("unknown FunctionCallCompilerStackFrameMode");
	}

	return Status::OK;
}

Status FunctionCallCompilerStackFrame::argumentsCallback(
		List<Expression*>* arguments) {
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
			Iterator<Expression*> argumentsIterator = arguments->iterator();
			Iterator<ValueType> parametersIterator =
					function->parameterTypes->iterator();
			while (argumentsIterator.hasNext() && parametersIterator.hasNext()) {
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

	if (!found) {
		return Status::ERROR("syscall not found");
	}

	// create and add the function call
	if (type == FunctionCallCompilerStackFrameType::BYTECODE) {
		return callbackAndEndFrame(functionCallBytecodeCallback, state,
				new FunctionCallVoid(function, arguments));
	} else if (type == FunctionCallCompilerStackFrameType::EXPRESSION) {
		return callbackAndEndFrame(functionCallExpressionCallback, state,
				new FunctionCallReturn(function, arguments));
	} else {
		crash("unknown FunctionCallCompilerStackFrameType");
	}
	return Status::OK;
}

// ExpressionCompilerStackFrame
ExpressionCompilerStackFrame::ExpressionCompilerStackFrame(bool hasParenthesis,
		Callback<Status(Expression*)> expressionCallback, CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::EXPRESSION, state) {
	this->hasParenthesis = hasParenthesis;
	this->expressionCallback = expressionCallback;
	mode = ExpressionCompilerStackFrameMode::READY;
}

ExpressionCompilerStackFrame::~ExpressionCompilerStackFrame() {

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
					new ExpressionCompilerStackFrame(true,
							BIND_MEM_CB(&ExpressionCompilerStackFrame::subexpressionCallback, this),
							state));
		} else {
			if (c == ')') {
				endFrame(state);
				if (!hasParenthesis) {
					return Status::REPROCESS;
				}
			} else {
				return Status::ERROR(
						"unexpected character while parsing expression");
			}
		}
	} else {
		crash("unknown ExpressionCompilerStackFrameMode");
	}

	return Status::OK;
}

Status ExpressionCompilerStackFrame::stringCallback(String string) {
	return callbackAndEndFrame(expressionCallback, state,
			(Expression*) new StringValue(string, true));
}

Status ExpressionCompilerStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "true")) {
		delete symbol;
		return callbackAndEndFrame(expressionCallback, state,
				(Expression*) new BooleanValue(true, true));
	} else if (strequ(symbol, "false")) {
		delete symbol;
		return callbackAndEndFrame(expressionCallback, state,
				(Expression*) new BooleanValue(false, true));
	} else {
		state->compilerStack->push(
				new FunctionCallCompilerStackFrame(symbol,
						BIND_MEM_CB(&ExpressionCompilerStackFrame::functionCallback, this),
						state));
		return Status::OK;
	}
}

Status ExpressionCompilerStackFrame::subexpressionCallback(
		Expression* expression) {
	CUNUSED(expression);
	NIMPL;
	return Status::OK;
}

Status ExpressionCompilerStackFrame::functionCallback(
		FunctionCallReturn* funcCall) {
	return callbackAndEndFrame(expressionCallback, state,
			(Expression*) funcCall);
}

// ArgumentCompilerStackFrame
ArgumentsCompilerStackFrame::ArgumentsCompilerStackFrame(
		Callback<Status(List<Expression*>*)> argumentsCallback,
		CompilerState* state) :
		CompilerStackFrame(CompilerStackFrameType::ARGUMENT, state) {
	this->argumentsCallback = argumentsCallback;
	arguments = new List<Expression*>();
	requireArgument = false;
}

ArgumentsCompilerStackFrame::~ArgumentsCompilerStackFrame() {

}

Status ArgumentsCompilerStackFrame::processCharacter(strchar c) {
	if (c == ',') {
		requireArgument = true;
	} else if (c == ')') {
		if (requireArgument) {
			return Status::ERROR("expected argument");
		}

		return callbackAndEndFrame(argumentsCallback, state, arguments);
	} else {
		// something else, must be an argument
		requireArgument = false;
		state->compilerStack->push(
				new ExpressionCompilerStackFrame(false,
						BIND_MEM_CB(&ArgumentsCompilerStackFrame::argumentCallback, this),
						state));
		return Status::REPROCESS;
	}

	return Status::OK;
}

Status ArgumentsCompilerStackFrame::argumentCallback(Expression* argument) {
	arguments->add(argument);
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
		strchar c = (i == size ? EOF : sourceCode[i]);
		//debug(c);

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

			//debug("type", (uinteger) state->compilerStack->peek()->type);
			status = state->compilerStack->peek()->processCharacter(c);

			if (status == Status::REPROCESS) {
				i--;
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
