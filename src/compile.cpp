/*
 * compiler.cpp
 *
 *  Created on: Sep 15, 2016
 *      Author: chris13524
 */

#include <array.h>
#include <compile.h>
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

// StackFrame
StackFrame::StackFrame(Type type) {
	this->type = type;
	state = NULL;
}

StackFrame::~StackFrame() {

}

void StackFrame::startFrame(StackFrame* frame) {
	frame->state = state;
	state->compilerStack->push(frame);
}

Status StackFrame::endFrame() {
	delete state->compilerStack->pop();
	return Status::OK;
}

Status StackFrame::processCharacter(strchar c) {
	// no-op
	UNUSED(c);
	return Status::OK;
}

// BodyStackFrame
BodyStackFrame::BodyStackFrame(Callback<Status(Code*)> codeCallback) :
		StackFrame(Type::BODY) {
	mode = Mode::READY;
	code = new Code();
	this->codeCallback = codeCallback;
	lastWasTerminated = true;
}

BodyStackFrame::~BodyStackFrame() {

}

Status BodyStackFrame::processCharacter(strchar c) {
	if (mode == Mode::READY) {
		if (c == ';' || c == '\n') {
			lastWasTerminated = true;
			return Status::OK;
		} else if (isWhitespace(c)) {
			return Status::OK;
		} else if (c == '}') {
			return callbackAndEndFrame(codeCallback, code);
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			startFrame(new SymbolStackFrame(
			BIND_MEM_CB(&BodyStackFrame::symbolCallback, this)));
			return Status::REPROCESS;
		} else if (c == '#') {
			startFrame(
					new CommentStackFrame(
							CommentStackFrame::Type::LINE));
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

Status BodyStackFrame::bytecodeCallback(Bytecode* bytecode) {
	code->bytecodes->add(bytecode);
	return Status::OK;
}

Status BodyStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "while")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// while
		delete symbol;

		startFrame(
				new WhileStackFrame(false,
						BIND_MEM_CB((Status(BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
	} else if (strequ(symbol, "dowhile")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// dowhile
		delete symbol;

		startFrame(
				new WhileStackFrame(true,
						BIND_MEM_CB((Status (BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
	} else if (strequ(symbol, "if")) {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// if
		delete symbol;

		startFrame(
				new IfStackFrame(
						BIND_MEM_CB((Status (BodyStackFrame::*) (IfBytecode*))&BodyStackFrame::bytecodeCallback, this)));
	} else if (strequ(symbol, "elseif")) {
		// elseif
		delete symbol;

		Bytecode* lastBytecode = code->bytecodes->getLast();

		if (lastBytecode != NULL && lastBytecode->type == Bytecode::Type::IF) {
			if (lastWasTerminated) {
				return Status::ERROR(
						"may not terminate an if statement before an elseif statement");
			}

			startFrame(
					new IfStackFrame(IfStackFrame::Type::ELSEIF,
							(IfBytecode*) lastBytecode));
		} else {
			return Status::ERROR(
					"elseif statement may only follow an if statement");
		}
	} else if (strequ(symbol, "else")) {
		// else
		delete symbol;

		Bytecode* lastBytecode = code->bytecodes->getLast();

		if (lastBytecode != NULL && lastBytecode->type == Bytecode::Type::IF) {
			if (lastWasTerminated) {
				return Status::ERROR(
						"may not terminate an if statement before and else statement");
			}

			startFrame(
					new IfStackFrame(IfStackFrame::Type::ELSE,
							(IfBytecode*) lastBytecode));
		} else {
			return Status::ERROR(
					"else statement may only follow an if statement");
		}
	} else {
		if (!lastWasTerminated) {
			return Status::ERROR("last statement wasn't terminated");
		}

		// function
		startFrame(
				new FunctionCallStackFrame(symbol,
						BIND_MEM_CB((Status (BodyStackFrame::*)(FunctionCallBytecode*))&BodyStackFrame::bytecodeCallback,
								this)));
	}
	mode = Mode::READY;
	lastWasTerminated = false;

	return Status::REPROCESS;
}

// IfStackFrame
IfStackFrame::IfStackFrame(Type type) :
		StackFrame(StackFrame::Type::IF) {
	this->type = type;
	mode = Mode::EXPECT_P;
	condition = NULL;
	ifBytecode = NULL;
}

IfStackFrame::IfStackFrame(Callback<Status(IfBytecode*)> ifBytecodeCallback) :
		IfStackFrame(Type::IF) {
	this->mode = Mode::EXPECT_P;
	this->ifBytecodeCallback = ifBytecodeCallback;
}

IfStackFrame::IfStackFrame(Type type, IfBytecode* ifBytecode) :
		IfStackFrame(type) {
	if (type == Type::ELSEIF) {
		mode = Mode::EXPECT_P;
	} else if (type == Type::ELSE) {
		mode = Mode::EXPECT_BODY;
		condition = new BooleanValue(true, true);
	} else {
		crash("use IfStackFrame(IfBytecodeCallback)");
	}
	this->ifBytecode = ifBytecode;
}

IfStackFrame::~IfStackFrame() {

}

Status IfStackFrame::processCharacter(strchar c) {
	if (mode == Mode::EXPECT_P) {
		if (c == '(') {
			mode = Mode::EXPECT_BODY;
			startFrame(new ArgumentsStackFrame(
			BIND_MEM_CB(&IfStackFrame::conditionCallback, this)));
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == Mode::EXPECT_BODY) {
		if (c == '{') {
			startFrame(new BodyStackFrame(
			BIND_MEM_CB(&IfStackFrame::codeCallback, this)));
		} else {
			return Status::ERROR("expected {");
		}
	} else {
		crash("unknown IfCompilerStackFrameMode");
	}

	return Status::OK;
}

Status IfStackFrame::conditionCallback(List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("if expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK;
	}
}

Status IfStackFrame::codeCallback(Code* code) {
	IfConditionCode* ifConditionCode = new IfConditionCode(condition, code);
	if (type == Type::IF) {
		return callbackAndEndFrame(ifBytecodeCallback,
				new IfBytecode(ifConditionCode));
	} else if (type == Type::ELSEIF
			|| type == Type::ELSE) {
		ifBytecode->ifs->add(ifConditionCode);
	} else {
		crash("unknown IfStackFrameType");
	}

	return endFrame();
}

// WhileStackFrame
WhileStackFrame::WhileStackFrame(bool isDoWhile,
		Callback<Status(WhileBytecode*)> whileBytecodeCallback) :
		StackFrame(Type::WHILE) {
	this->mode = Mode::EXPECT_P;
	this->isDoWhile = isDoWhile;
	this->whileBytecodeCallback = whileBytecodeCallback;

	condition = NULL;
}

WhileStackFrame::~WhileStackFrame() {

}

Status WhileStackFrame::processCharacter(strchar c) {
	if (mode == Mode::EXPECT_P) {
		if (c == '(') {
			mode = Mode::EXPECT_BODY;
			startFrame(new ArgumentsStackFrame(
			BIND_MEM_CB(&WhileStackFrame::conditionCallback, this)));
		} else {
			return Status::ERROR("expected (");
		}
	} else if (mode == Mode::EXPECT_BODY) {
		if (c == '{') {
			startFrame(new BodyStackFrame(
			BIND_MEM_CB(&WhileStackFrame::codeCallback, this)));
		} else {
			return Status::ERROR("expected {");
		}
	} else {
		crash("unknown WhileStackFrameMode");
	}

	return Status::OK;
}

Status WhileStackFrame::conditionCallback(List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("while expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK;
	}
}

Status WhileStackFrame::codeCallback(Code* code) {
	return callbackAndEndFrame(whileBytecodeCallback,
			new WhileBytecode(condition, code, isDoWhile));
}

// SymbolStackFrame
SymbolStackFrame::SymbolStackFrame(Callback<Status(String)> symbolCallback) :
		StackFrame(Type::SYMBOL) {
	symbol = new List<strchar>();
	this->symbolCallback = symbolCallback;
}

SymbolStackFrame::~SymbolStackFrame() {
	delete symbol;
}

Status SymbolStackFrame::processCharacter(strchar c) {
	if (isValidSymbolChar(c)) {
		symbol->add(c);
	} else {
		Status status = callbackAndEndFrame(symbolCallback,
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

// StringStackFrame
StringStackFrame::StringStackFrame(Callback<Status(String)> stringCallback) :
		StackFrame(Type::STRING) {
	string = new List<strchar>();
	escaping = false;
	this->stringCallback = stringCallback;
}

StringStackFrame::~StringStackFrame() {
	delete string;
}

Status StringStackFrame::processCharacter(strchar c) {
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
			return callbackAndEndFrame(stringCallback, charListToString(string));
		} else if (c == EOF) {
			return Status::ERROR("unexpected EOF");
		} else {
			// just another character
			string->add(c);
		}
	}

	return Status::OK;
}

// CommentStackFrame
CommentStackFrame::CommentStackFrame(Type type) :
		StackFrame(StackFrame::Type::COMMENT) {
	this->type = type;
}

CommentStackFrame::~CommentStackFrame() {

}

Status CommentStackFrame::processCharacter(strchar c) {
	if (type == Type::LINE) {
		if (c == '\n') {
			endFrame();
		}
		// ignore character
	} else {
		crash("unknown CommentStackFrameType");
	}

	return Status::OK;
}

// FunctionCallStackFrame
FunctionCallStackFrame::FunctionCallStackFrame(String name,
		Type type) :
		StackFrame(StackFrame::Type::FUNCTION_CALL) {
	this->name = name;
	this->type = type;
	mode = Mode::EXPECT_P;
}

FunctionCallStackFrame::FunctionCallStackFrame(String name,
		Callback<Status(FunctionCallBytecode*)> functionCallBytecodeCallback) :
		FunctionCallStackFrame(name, Type::BYTECODE) {
	this->functionCallBytecodeCallback = functionCallBytecodeCallback;
}

FunctionCallStackFrame::FunctionCallStackFrame(String name,
		Callback<Status(FunctionCallExpression*)> functionCallExpressionCallback) :
		FunctionCallStackFrame(name, Type::EXPRESSION) {
	this->functionCallExpressionCallback = functionCallExpressionCallback;
}

FunctionCallStackFrame::~FunctionCallStackFrame() {
	delete name;
}

Status FunctionCallStackFrame::processCharacter(strchar c) {
	if (mode == Mode::EXPECT_P) {
		if (c == '(') {
			startFrame(new ArgumentsStackFrame(
			BIND_MEM_CB(&FunctionCallStackFrame::argumentsCallback, this)));
		} else {
			return Status::ERROR("expected (");
		}
	} else {
		crash("unknown FunctionCallStackFrameMode");
	}

	return Status::OK;
}

Status FunctionCallStackFrame::argumentsCallback(List<Expression*>* arguments) {
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
	if (type == Type::BYTECODE) {
		return callbackAndEndFrame(functionCallBytecodeCallback,
				new FunctionCallBytecode(function, arguments));
	} else if (type == Type::EXPRESSION) {
		return callbackAndEndFrame(functionCallExpressionCallback,
				new FunctionCallExpression(function, arguments));
	} else {
		crash("unknown FunctionCallStackFrameType");
	}
	return Status::OK;
}

// ExpressionStackFrame
ExpressionStackFrame::ExpressionStackFrame(bool hasParenthesis,
		Callback<Status(Expression*)> expressionCallback) :
		StackFrame(Type::EXPRESSION) {
	this->hasParenthesis = hasParenthesis;
	this->expressionCallback = expressionCallback;
	mode = Mode::READY;
}

ExpressionStackFrame::~ExpressionStackFrame() {

}

Status ExpressionStackFrame::processCharacter(strchar c) {
	if (mode == Mode::READY) {
		if (c == '\'') {
			// begin string
			startFrame(new StringStackFrame(
			BIND_MEM_CB(&ExpressionStackFrame::stringCallback, this)));
		} else if (isValidSymbolChar(c)) {
			startFrame(new SymbolStackFrame(
			BIND_MEM_CB(&ExpressionStackFrame::symbolCallback, this)));
			return Status::REPROCESS;
		} else if (c == '(') {
			return Status::ERROR(
					"parenthesis in expression not implemented yet");
			startFrame(new ExpressionStackFrame(true,
			BIND_MEM_CB(&ExpressionStackFrame::subexpressionCallback, this)));
		} else {
			if (c == ')') {
				endFrame();
				if (!hasParenthesis) {
					return Status::REPROCESS;
				}
			} else {
				return Status::ERROR(
						"unexpected character while parsing expression");
			}
		}
	} else {
		crash("unknown ExpressionStackFrameMode");
	}

	return Status::OK;
}

Status ExpressionStackFrame::stringCallback(String string) {
	return callbackAndEndFrame(expressionCallback,
			(Expression*) new StringValue(string, true));
}

Status ExpressionStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "true")) {
		delete symbol;
		return callbackAndEndFrame(expressionCallback,
				(Expression*) new BooleanValue(true, true));
	} else if (strequ(symbol, "false")) {
		delete symbol;
		return callbackAndEndFrame(expressionCallback,
				(Expression*) new BooleanValue(false, true));
	} else {
		startFrame(new FunctionCallStackFrame(symbol,
		BIND_MEM_CB(&ExpressionStackFrame::functionCallback, this)));
		return Status::OK;
	}
}

Status ExpressionStackFrame::subexpressionCallback(Expression* expression) {
	CUNUSED(expression);
	NIMPL;
	return Status::OK;
}

Status ExpressionStackFrame::functionCallback(FunctionCallExpression* funcCall) {
	return callbackAndEndFrame(expressionCallback, (Expression*) funcCall);
}

// ArgumentCompilerStackFrame
ArgumentsStackFrame::ArgumentsStackFrame(
		Callback<Status(List<Expression*>*)> argumentsCallback) :
		StackFrame(Type::ARGUMENT) {
	this->argumentsCallback = argumentsCallback;
	arguments = new List<Expression*>();
	requireArgument = false;
}

ArgumentsStackFrame::~ArgumentsStackFrame() {

}

Status ArgumentsStackFrame::processCharacter(strchar c) {
	if (c == ',') {
		requireArgument = true;
	} else if (c == ')') {
		if (requireArgument) {
			return Status::ERROR("expected argument");
		}

		return callbackAndEndFrame(argumentsCallback, arguments);
	} else {
		// something else, must be an argument
		requireArgument = false;
		startFrame(new ExpressionStackFrame(false,
		BIND_MEM_CB(&ArgumentsStackFrame::argumentCallback, this)));
		return Status::REPROCESS;
	}

	return Status::OK;
}

Status ArgumentsStackFrame::argumentCallback(Expression* argument) {
	arguments->add(argument);
	return Status::OK;
}

// main compiler state

State::State() {
	compilerStack = new Stack<StackFrame*>();
}

State::~State() {
	while (compilerStack->size() > 0) {
		delete compilerStack->pop();
	}
	delete compilerStack;
}

Code* compile(String code) {
	return compile(code, strlen(code));
}

Code* compile(String sourceCode, size size) {
	State* state = new State();
	BodyStackFrame* firstFrame = new BodyStackFrame( { });
	firstFrame->state = state;
	state->compilerStack->push(firstFrame);

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
			if (state->compilerStack->peek()->type != StackFrame::Type::SYMBOL
					&& state->compilerStack->peek()->type
							!= StackFrame::Type::STRING
					&& state->compilerStack->peek()->type
							!= StackFrame::Type::BODY) {

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

	StackFrame::Type endFrameType = state->compilerStack->peek()->type;
	if (status == Status::OK && endFrameType != StackFrame::Type::BODY
			&& endFrameType != StackFrame::Type::COMMENT) {
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
	Code* code = ((BodyStackFrame*) state->compilerStack->peek())->code;
	delete state;

	return code;
}

}

}
