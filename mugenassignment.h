#pragma once

#include <prism/geometry.h>

#include <prism/mugendefreader.h>
#include <prism/memorystack.h>
#include <prism/stlutil.h>

using namespace prism;

typedef enum {
	MUGEN_ASSIGNMENT_RETURN_TYPE_STRING,
	MUGEN_ASSIGNMENT_RETURN_TYPE_NUMBER,
	MUGEN_ASSIGNMENT_RETURN_TYPE_FLOAT,
	MUGEN_ASSIGNMENT_RETURN_TYPE_BOOLEAN,
	MUGEN_ASSIGNMENT_RETURN_TYPE_VECTOR,
	MUGEN_ASSIGNMENT_RETURN_TYPE_RANGE,
	MUGEN_ASSIGNMENT_RETURN_TYPE_BOTTOM,

} AssignmentReturnType;

typedef struct {
	AssignmentReturnType mType;
	char mData[200];
} AssignmentReturnValue;

enum DreamMugenAssignmentType : uint8_t {
	MUGEN_ASSIGNMENT_TYPE_FIXED_BOOLEAN,
	MUGEN_ASSIGNMENT_TYPE_AND,
	MUGEN_ASSIGNMENT_TYPE_XOR,
	MUGEN_ASSIGNMENT_TYPE_OR,
	MUGEN_ASSIGNMENT_TYPE_COMPARISON,
	MUGEN_ASSIGNMENT_TYPE_INEQUALITY,
	MUGEN_ASSIGNMENT_TYPE_LESS_OR_EQUAL,
	MUGEN_ASSIGNMENT_TYPE_GREATER_OR_EQUAL,
	MUGEN_ASSIGNMENT_TYPE_VECTOR,
	MUGEN_ASSIGNMENT_TYPE_RANGE,
	MUGEN_ASSIGNMENT_TYPE_NULL,
	MUGEN_ASSIGNMENT_TYPE_NEGATION,
	MUGEN_ASSIGNMENT_TYPE_VARIABLE,
	MUGEN_ASSIGNMENT_TYPE_RAW_VARIABLE,
	MUGEN_ASSIGNMENT_TYPE_NUMBER,
	MUGEN_ASSIGNMENT_TYPE_FLOAT,
	MUGEN_ASSIGNMENT_TYPE_STRING,
	MUGEN_ASSIGNMENT_TYPE_ARRAY,
	MUGEN_ASSIGNMENT_TYPE_LESS,
	MUGEN_ASSIGNMENT_TYPE_GREATER,
	MUGEN_ASSIGNMENT_TYPE_ADDITION,
	MUGEN_ASSIGNMENT_TYPE_MULTIPLICATION,
	MUGEN_ASSIGNMENT_TYPE_MODULO,
	MUGEN_ASSIGNMENT_TYPE_SUBTRACTION,
	MUGEN_ASSIGNMENT_TYPE_SET_VARIABLE,
	MUGEN_ASSIGNMENT_TYPE_DIVISION,
	MUGEN_ASSIGNMENT_TYPE_EXPONENTIATION,
	MUGEN_ASSIGNMENT_TYPE_BITWISE_INVERSION,
	MUGEN_ASSIGNMENT_TYPE_UNARY_MINUS,
	MUGEN_ASSIGNMENT_TYPE_OPERATOR_ARGUMENT,
	MUGEN_ASSIGNMENT_TYPE_BITWISE_AND,
	MUGEN_ASSIGNMENT_TYPE_BITWISE_XOR,
	MUGEN_ASSIGNMENT_TYPE_BITWISE_OR,
	MUGEN_ASSIGNMENT_TYPE_STATIC,
	MUGEN_ASSIGNMENT_TYPE_AMOUNT
};

typedef struct {
	uint8_t mType;
} DreamMugenAssignment;

typedef struct {
	uint8_t mType;
	int mValue;
} DreamMugenNumberAssignment;

typedef struct {
	uint8_t mType;
	double mValue;
} DreamMugenFloatAssignment;

typedef struct {
	uint8_t mType;
	char* mValue;
} DreamMugenStringAssignment;

typedef struct {
	uint8_t mType;
	void* mFunc;
} DreamMugenVariableAssignment;

typedef struct {
	uint8_t mType;
	char* mName;
} DreamMugenRawVariableAssignment;


typedef struct {
	uint8_t mType;
	uint8_t mValue;
} DreamMugenFixedBooleanAssignment;

typedef struct {
	uint8_t mType;
	uint8_t mExcludeLeft;
	uint8_t mExcludeRight;
	DreamMugenAssignment* a;
} DreamMugenRangeAssignment;

typedef struct {
	uint8_t mType;
	DreamMugenAssignment* a;
	DreamMugenAssignment* b;
} DreamMugenDependOnTwoAssignment;

typedef struct {
	uint8_t mType;
	DreamMugenAssignment* a;
} DreamMugenDependOnOneAssignment;

typedef struct {
	uint8_t mType;
	void* mFunc;
	DreamMugenAssignment* mIndex;
} DreamMugenArrayAssignment;

void setupDreamAssignmentReader(MemoryStack* tMemoryStack);
void shutdownDreamAssignmentReader();
void setDreamAssignmentCommandLookupID(int tID);
void resetDreamAssignmentCommandLookupID();

DreamMugenAssignment* makeDreamTrueMugenAssignment();

DreamMugenAssignment* makeDreamFalseMugenAssignment();
void destroyDreamFalseMugenAssignment(DreamMugenAssignment* tAssignment);
void destroyDreamMugenAssignment(DreamMugenAssignment* tAssignment);

DreamMugenAssignment* makeDreamNumberMugenAssignment(int tVal);
DreamMugenAssignment * makeDreamFloatMugenAssignment(double tVal);
DreamMugenAssignment * makeDreamStringMugenAssignment(const char* tVal);
DreamMugenAssignment* makeDream2DVectorMugenAssignment(const Vector2D& tVal);
DreamMugenAssignment* makeDreamAndMugenAssignment(DreamMugenAssignment* a, DreamMugenAssignment* b);
DreamMugenAssignment* makeDreamOrMugenAssignment(DreamMugenAssignment* a, DreamMugenAssignment* b);

DreamMugenAssignment*  parseDreamMugenAssignmentFromString(char* tText);
DreamMugenAssignment*  parseDreamMugenAssignmentFromString(const char* tText);

uint8_t fetchDreamAssignmentFromGroupAndReturnWhetherItExists(const char* tName, MugenDefScriptGroup* tGroup, DreamMugenAssignment** tOutput);
void fetchDreamAssignmentFromGroupAsElement(MugenDefScriptGroupElement* tElement, DreamMugenAssignment** tOutput);

int doDreamAssignmentStringsBeginsWithPattern(char* tPattern, char* tText);