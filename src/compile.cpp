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

Status::Status(Type type) :
		type(type), message(NULL) {
}

Status::Status(Type type, String message) :
		type(type), message(message) {
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
	scope = NULL;
}

StackFrame::~StackFrame() {

}

void StackFrame::startFrame(StackFrame* frame) {
	frame->state = state;
	state->compilerStack->push(frame);
	if (frame->type != Type::BODY) {
		frame->scope = scope;
	}
	frame->init();
}

Status StackFrame::endFrame() {
	delete state->compilerStack->pop();
	return Status::OK();
}

void StackFrame::init() {
}

Status StackFrame::processCharacter(strchar c) {
	// no-op
	UNUSED(c);
	return Status::OK();
}

VariableDefinition* StackFrame::findVariable(String name) {
	Iterator<VariableDefinition*> existingVariables =
			scope->variables->iterator();
	while (existingVariables.hasNext()) {
		VariableDefinition* variable = existingVariables.next();
		if (strequ(variable->name, name)) {
			return variable;
		}
	}

	return NULL;
}

// BodyStackFrame
BodyStackFrame::BodyStackFrame(Callback<Status(Code*)> codeCallback) :
		StackFrame(Type::BODY) {
	mode = Mode::READY;
	code = new Code();
	scope = code->scope;
	this->codeCallback = codeCallback;
	lastWasTerminated = true;
	symbol1 = NULL;
	isTop = false;
	justAddedVariable = false;
	variableToSet = NULL;
	variableIndex = 0;
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
			return Status::OK();
		} else if (isWhitespace(c)) {
			return Status::OK();
		} else if (c == '}') {
			return callbackAndEndFrame(codeCallback, code);
		} else if (isValidSymbolChar(c)) {
			// begin a symbol
			startFrame(new SymbolStackFrame(
			BIND_MEM_CB(&BodyStackFrame::symbol1Callback, this)));
			return Status::REPROCESS();
		} else if (c == '#') {
			startFrame(new CommentStackFrame(CommentStackFrame::Type::LINE));
		} else if (c == '=' && justAddedVariable) {
			variableToSet = code->scope->variables->getLast();
			startFrame(new ExpressionStackFrame(false,
			BIND_MEM_CB(&BodyStackFrame::variableSetCallback, this)));
		} else if (c == EOF) {
			if (isTop) {
				return Status::OK();
			} else {
				return Status::ERROR("block must be closed");
			}
		} else {
			return Status::ERROR("unexpected character in body");
		}
	} else if (mode == Mode::SYMBOL1) {
		if (isWhitespace(c)) {
			return Status::OK();
		} else if (isValidSymbolChar(c)) {
			startFrame(new SymbolStackFrame(
			BIND_MEM_CB(&BodyStackFrame::symbol2Callback, this)));
			return Status::REPROCESS();
		} else if (c == '=') {
			variableToSet = findVariable(symbol1);
			delete symbol1;
			symbol1 = NULL;

			if (variableToSet == NULL) {
				return Status::ERROR("could not find variable");
			}

			startFrame(new ExpressionStackFrame(false,
			BIND_MEM_CB(&BodyStackFrame::variableSetCallback, this)));
		} else if (c == '(') {
			if (strequ(symbol1, "while")) {
				// while
				delete symbol1;
				symbol1 = NULL;

				startFrame(
						new WhileStackFrame(false,
								BIND_MEM_CB((Status(BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "dowhile")) {
				// dowhile
				delete symbol1;
				symbol1 = NULL;

				startFrame(
						new WhileStackFrame(true,
								BIND_MEM_CB((Status (BodyStackFrame::*)(WhileBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "if")) {
				// if
				delete symbol1;
				symbol1 = NULL;

				startFrame(
						new IfStackFrame(
								BIND_MEM_CB((Status (BodyStackFrame::*) (IfBytecode*))&BodyStackFrame::bytecodeCallback, this)));
			} else if (strequ(symbol1, "elseif")) {
				// elseif
				delete symbol1;
				symbol1 = NULL;

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
				symbol1 = NULL;

				Bytecode* lastBytecode =
						code->bytecodes->size() == 0 ?
								NULL : code->bytecodes->getLast();

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
	return Status::OK();
}

Status BodyStackFrame::bytecodeCallback(Bytecode * bytecode) {
	if (!lastWasTerminated) {
		// termination check isn't required
		//return Status::ERROR("last statement wasn't terminated");
	}
	code->bytecodes->add(bytecode);
	lastWasTerminated = false;
	justAddedVariable = false;
	return Status::OK();
}

Status BodyStackFrame::symbol1Callback(String symbol) {
	this->symbol1 = symbol;
	mode = Mode::SYMBOL1;
	return Status::OK();
}

Status BodyStackFrame::symbol2Callback(String symbol) {
	ValueType valueType = ValueType::UNKNOWN;
	if (strequ(symbol1, "__Void")) {
		valueType = ValueType::VOID;
		delete symbol1;
		symbol1 = NULL;
	} else if (strequ(symbol1, "__Boolean")) {
		valueType = ValueType::BOOLEAN;
		delete symbol1;
		symbol1 = NULL;
	} else if (strequ(symbol1, "__String")) {
		valueType = ValueType::STRING;
		delete symbol1;
		symbol1 = NULL;
	} else {
		// search for class
		Iterator<Class*> classIterator = mish_classes.iterator();
		Class* clazz;
		bool found = false;
		while (classIterator.hasNext()) {
			clazz = classIterator.next();
			if (strequ(symbol1, clazz->name)) {
				found = true;
				break;
			}
		}

		// found class
		delete symbol1;
		symbol1 = NULL;

		// check if exists
		if (!found) {
			return Status::ERROR("type not defined");
		}

		// it exists, set value type
		valueType = ValueType::CLASS(clazz);
	}

	if (findVariable(symbol) != NULL) {
		return Status::ERROR(
				"variable with same name already exists in this scope");
	}

	code->scope->variables->add(
			new VariableDefinition(valueType, symbol, variableIndex));
	variableIndex++;
	justAddedVariable = true;
	mode = Mode::READY;
	return Status::OK();
}

Status BodyStackFrame::variableSetCallback(Expression * value) {
	if (variableToSet == NULL) {
		crash("variable to set is null");
	}
	bytecodeCallback(new SetVariableBytecode(variableToSet, value));
	return Status::OK();
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

	return Status::OK();
}

Status IfStackFrame::conditionCallback(List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("if expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK();
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

	return Status::OK();
}

Status WhileStackFrame::conditionCallback(List<Expression*>* condition) {
	if (condition->size() != 1) {
		delete condition;
		return Status::ERROR("while expects one argument");
	} else {
		this->condition = condition->get(0);
		delete condition;
		return Status::OK();
	}
}

Status WhileStackFrame::codeCallback(Code* code) {
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

		if (status != Status::OK()) {
			return status;
		} else {
			// we havn't done anything with this char, so we have to re-parse it
			return Status::REPROCESS();
		}
	}

	return Status::OK();
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

	return Status::OK();
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

	return Status::OK();
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
	return Status::OK();
}

// ExpressionStackFrame
ExpressionStackFrame::ExpressionStackFrame(bool hasParenthesis,
		Callback<Status(Expression*)> expressionCallback) :
		StackFrame(Type::EXPRESSION) {
	this->hasParenthesis = hasParenthesis;
	this->expressionCallback = expressionCallback;
	mode = Mode::READY;
	symbol1 = NULL;
	expression = NULL;
}

ExpressionStackFrame::~ExpressionStackFrame() {
	if (symbol1 != NULL) {
		delete symbol1;
	}

	if (expression != NULL) {
		delete expression;
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
			return Status::REPROCESS();
		} else if (c == '(') {
			return Status::ERROR(
					"parenthesis in expression not implemented yet");
			startFrame(new ExpressionStackFrame(true,
			BIND_MEM_CB(&ExpressionStackFrame::subexpressionCallback, this)));
		} else {
			return Status::ERROR("unexpected character in expression");
		}
	} else if (mode == Mode::EXPECT_OPERATOR) {
		if (false) {
			// TODO operators
		} else if (c == '(') {
			startFrame(new FunctionCallStackFrame(symbol1,
			BIND_MEM_CB(&ExpressionStackFrame::functionCallback, this)));
			symbol1 = NULL;
		} else { // terminate expression
			if (symbol1 != NULL) {
				// variable
				VariableDefinition* vDef = findVariable(symbol1);
				delete symbol1;
				symbol1 = NULL;

				if (vDef == NULL) {
					return Status::ERROR("could not find variable");
				}

				expression = new VariableExpression(vDef);
			}

			// callback expression
			bool hasParenthesis = this->hasParenthesis;
			Expression* expression = this->expression;
			this->expression = NULL;
			Status status = callbackAndEndFrame(expressionCallback, expression);
			if (status != Status::OK()) {
				return status;
			}
			if (hasParenthesis && c == ')') {
				return Status::OK();
			}
			return Status::REPROCESS();
		}
	} else {
		crash("unknown ExpressionStackFrameMode");
	}

	return Status::OK();
}

Status ExpressionStackFrame::stringCallback(String string) {
	expression = (Expression*) new StringValue(string, true);
	string = NULL;
	mode = Mode::EXPECT_OPERATOR;
	return Status::OK();
}

Status ExpressionStackFrame::symbolCallback(String symbol) {
	if (strequ(symbol, "true")) {
		delete symbol;
		expression = new BooleanValue(true, true);
	} else if (strequ(symbol, "false")) {
		delete symbol;
		expression = new BooleanValue(false, true);
	} else {
		symbol1 = symbol;
	}

	mode = Mode::EXPECT_OPERATOR;
	return Status::OK();
}

Status ExpressionStackFrame::subexpressionCallback(Expression * expression) {
	CUNUSED(expression);
	NIMPL;
	mode = Mode::EXPECT_OPERATOR;
	return Status::OK();
}

Status ExpressionStackFrame::functionCallback(
		FunctionCallExpression * funcCall) {
	expression = funcCall;
	mode = Mode::EXPECT_OPERATOR;
	return Status::OK();
}

// ArgumentCompilerStackFrame
ArgumentsStackFrame::ArgumentsStackFrame(
		Callback<Status(List<Expression*>*)> argumentsCallback) :
		StackFrame(Type::ARGUMENT) {
	this->argumentsCallback = argumentsCallback;
	this->scope = scope;
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
		return Status::REPROCESS();
	}

	return Status::OK();
}

Status ArgumentsStackFrame::argumentCallback(Expression * argument) {
	arguments->add(argument);
	return Status::OK();
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
	return compile(code, stringlength(code));
}

Code* compile(String sourceCode, size size) {
	State* state = new State();
	BodyStackFrame* firstFrame = new BodyStackFrame( { });
	firstFrame->state = state;
	firstFrame->isTop = true;
	state->compilerStack->push(firstFrame);

	// line stuff
	uinteger lineStart = 0;
	uinteger lineEnd = 0;
	uinteger lineNumber = 1;

	// error message stuff
	bool hasError = false;
	uinteger errorPosition = 0;
	Status status = Status::OK();

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

			if (status == Status::REPROCESS()) {
				i--;
				continue;
			} else if (status != Status::OK()) {
				hasError = true;
				errorPosition = i;
			}
		}
	}

	StackFrame::Type endFrameType = state->compilerStack->peek()->type;
	if (status == Status::OK() && endFrameType != StackFrame::Type::BODY
			&& endFrameType != StackFrame::Type::COMMENT) {
		// something wasn't properly closed, throw a generic error for now
		debug("parse mode", (uinteger) endFrameType);
		status = Status::ERROR("incorrect parse mode");
		hasError = true;
		errorPosition = i;
	}

	if (hasError) {
		if (lineEnd == 0) {
			lineEnd = i;
		}

		//debug("lineStart", lineStart);
		//debug("lineEnd", lineEnd);

		// generate the error message and display it
		String line = substring(sourceCode, lineStart, lineEnd);

		uinteger markerLength = lineEnd - lineStart
				+ (errorPosition == size ? 1 : 0);

		strchar* errorPositionMarker = (strchar*) create(
				markerLength * sizeof(strchar) + 1);
		uinteger i = 0;
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
