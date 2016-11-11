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
	frame->init();
}

Status StackFrame::endFrame() {
	delete state->compilerStack->pop();
	return Status::OK;
}

void StackFrame::init() {
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
	symbol1 = NULL;
}

BodyStackFrame::~BodyStackFrame() {
	if (symbol1 != NULL) {
		delete symbol1;
	}
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
			startFrame(new CommentStackFrame(CommentStackFrame::Type::LINE));
		} else if (c == EOF) {
			return Status::OK;
		} else {
			return Status::ERROR("unexpected character in body");
		}
	} else if (mode == Mode::SYMBOL1) {
		if (isWhitespace(c)) {
			return Status::OK;
		} else if (c == '(') {
			if (strequ(symbol1, "while")) {
				// while
				delete symbol1;

				startFrame(
						new WhileStackFrame(false,
								BIND_MEM_CB((Status(BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "dowhile")) {
				// dowhile
				delete symbol1;

				startFrame(
						new WhileStackFrame(true,
								BIND_MEM_CB((Status (BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "if")) {
				// if
				delete symbol1;

				startFrame(
						new IfStackFrame(
								BIND_MEM_CB((Status (BodyStackFrame::*) (IfBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "elseif")) {
				// elseif
				delete symbol1;

				Bytecode* lastBytecode = code->bytecodes->getLast();

				if (lastBytecode != NULL
						&& lastBytecode->type == Bytecode::Type::IF) {
					startFrame(
							new IfStackFrame(IfStackFrame::Type::ELSEIF,
									(IfBytecode*) lastBytecode));
				} else {
					return Status::ERROR(
							"elseif statement may only follow an if statement");
				}
			} else {
				// start function
				startFrame(
						new FunctionCallStackFrame(symbol1,
								BIND_MEM_CB((Status (BodyStackFrame::*)(FunctionCallBytecode*))&BodyStackFrame::bytecodeCallback,
										this)));
			}
		} else if (c == '{') {
			if (strequ(symbol1, "else")) {
				// else
				delete symbol1;

				Bytecode* lastBytecode = code->bytecodes->getLast();

				if (lastBytecode != NULL
						&& lastBytecode->type == Bytecode::Type::IF) {
					startFrame(
							new IfStackFrame(IfStackFrame::Type::ELSE,
									(IfBytecode*) lastBytecode));
				} else {
					return Status::ERROR(
							"else statement may only follow an if statement");
				}
			} else {
				return Status::ERROR(
						"previous symbol is not a valid block starting command thingie");
			}
		} else {
			return Status::ERROR("unexpected character after symbol");
		}
		symbol1 = NULL;
		mode = Mode::READY;
	} else {
		crash("unknown BodyCompilerStackFrameMode");
	}
	return Status::OK;
}

Status BodyStackFrame::bytecodeCallback(Bytecode* bytecode) {
	if (!lastWasTerminated) {
		return Status::ERROR("last statement wasn't terminated");
	}
	code->bytecodes->add(bytecode);
	lastWasTerminated = false;
	return Status::OK;
}

Status BodyStackFrame::symbolCallback(String symbol) {
	this->symbol1 = symbol;

	mode = Mode::SYMBOL1;
	return Status::OK;
}

// IfStackFrame
IfStackFrame::IfStackFrame(Type type) :
		StackFrame(StackFrame::Type::IF) {
	this->type = type;
	mode = Mode::EXPECT_BODY;
	condition = NULL;
	ifBytecode = NULL;
}

IfStackFrame::IfStackFrame(Callback<Status(IfBytecode*)> ifBytecodeCallback) :
		IfStackFrame(Type::IF) {
	this->mode = Mode::EXPECT_BODY;
	this->ifBytecodeCallback = ifBytecodeCallback;
}

IfStackFrame::IfStackFrame(Type type, IfBytecode* ifBytecode) :
		IfStackFrame(type) {
	this->type = type;
	this->ifBytecode = ifBytecode;
}

IfStackFrame::~IfStackFrame() {
	if (condition != NULL) {
		delete condition;
	}
}

void IfStackFrame::init() {
	if (type == Type::ELSE) {
		startFrame(new BodyStackFrame(
		BIND_MEM_CB(&IfStackFrame::codeCallback, this)));
		condition = new BooleanValue(true, true);
	} else {
		startFrame(new ArgumentsStackFrame(
		BIND_MEM_CB(&IfStackFrame::conditionCallback, this)));
	}
}

Status IfStackFrame::processCharacter(strchar c) {
	if (mode == Mode::EXPECT_BODY) {
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
	condition = NULL; // don't delete
	if (type == Type::IF) {
		return callbackAndEndFrame(ifBytecodeCallback,
				new IfBytecode(ifConditionCode));
	} else if (type == Type::ELSEIF || type == Type::ELSE) {
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
	this->mode = Mode::EXPECT_BODY;
	this->isDoWhile = isDoWhile;
	this->whileBytecodeCallback = whileBytecodeCallback;

	condition = NULL;
}

WhileStackFrame::~WhileStackFrame() {
	if (condition != NULL) {
		delete condition;
	}
}

void WhileStackFrame::init() {
	startFrame(new ArgumentsStackFrame(
	BIND_MEM_CB(&WhileStackFrame::conditionCallback, this)));
}

Status WhileStackFrame::processCharacter(strchar c) {
	if (mode == Mode::EXPECT_BODY) {
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

Status WhileStackFrame::codeCallback(Code * code) {
	WhileBytecode* bytecode = new WhileBytecode(condition, code, isDoWhile);
	condition = NULL;
	return callbackAndEndFrame(whileBytecodeCallback, bytecode);
}

// SymbolStackFrame
SymbolStackFrame::SymbolStackFrame(Callback<Status(String)> symbolCallback) :
		StackFrame(Type::SYMBOL) {
	symbol = new List<strchar>();
	this->symbolCallback = symbolCallback;
}

SymbolStackFrame::~SymbolStackFrame() {
	if (symbol != NULL) {
		delete symbol;
	}
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
	if (string != NULL) {
		delete string;
	}
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
FunctionCallStackFrame::FunctionCallStackFrame(String name, Type type) :
		StackFrame(StackFrame::Type::FUNCTION_CALL) {
	this->name = name;
	this->type = type;
	mode = Mode::INVALID;
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
	if (name != NULL) {
		delete name;
	}
}

void FunctionCallStackFrame::init() {
	startFrame(new ArgumentsStackFrame(
	BIND_MEM_CB(&FunctionCallStackFrame::argumentsCallback, this)));
}

Status FunctionCallStackFrame::argumentsCallback(List<Expression*>* arguments) {
	// determine if this is a syscall
	List<Function*>* functions;
	if (stringStartsWith(name, "__")) {
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
		if (strequ(function->name, name)) {
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
	symbol1 = NULL;
}

ExpressionStackFrame::~ExpressionStackFrame() {
	if (symbol1 != NULL) {
		delete symbol1;
	}
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
			if (symbol1 != NULL) {
				startFrame(new FunctionCallStackFrame(symbol1,
				BIND_MEM_CB(&ExpressionStackFrame::functionCallback, this)));
				symbol1 = NULL;
			} else {
				return Status::ERROR(
						"parenthesis in expression not implemented yet");
				startFrame(
						new ExpressionStackFrame(true,
								BIND_MEM_CB(&ExpressionStackFrame::subexpressionCallback, this)));
			}
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
	Expression* expression = (Expression*) new StringValue(string, true);
	string = NULL;
	return callbackAndEndFrame(expressionCallback, expression);
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
		symbol1 = symbol;
		return Status::OK;
	}
}

Status ExpressionStackFrame::subexpressionCallback(Expression * expression) {
	CUNUSED(expression);
	NIMPL;
	return Status::OK;
}

Status ExpressionStackFrame::functionCallback(
		FunctionCallExpression * funcCall) {
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
	if (arguments != NULL) {
		delete arguments;
	}
}

Status ArgumentsStackFrame::processCharacter(strchar c) {
	if (c == ',') {
		requireArgument = true;
	} else if (c == ')') {
		if (requireArgument) {
			return Status::ERROR("expected argument");
		}

		List<Expression*>* arguments = this->arguments;
		this->arguments = NULL;
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

Status ArgumentsStackFrame::argumentCallback(Expression * argument) {
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
			if (isWhitespace(c)) {
				if (state->compilerStack->peek()->type
						!= StackFrame::Type::SYMBOL
						&& state->compilerStack->peek()->type
								!= StackFrame::Type::STRING
						&& state->compilerStack->peek()->type
								!= StackFrame::Type::BODY) {
					continue;
				}
			}

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
