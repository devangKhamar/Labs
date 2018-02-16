//============================================================================
// Name        : ROBDDs.cpp
// Author      : Devang Khamar
// Version     : 1.x
// Copyright   : N/A
// Description : ROBDDs based on the Tutorial Paper by Henrik Andersen
//============================================================================

#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <stdbool.h>

#define MAX_PROP_LENGTH 10						//maximum readable length of variable string
#define MAX_LINE_SIZE  1000						//maximum length of Input Boolean expression

#define BIG_TEST_STRING  "(IMPL (AND (OR (IMPL x1 (NOT x2)) (EQUIV (IMPL x3 x4) (AND x5 x6))) (AND (EQUIV (NOT x7) x8) (IMPL (EQUIV x9 x10) (OR x12 x13)))) (AND (OR (IMPL x14 (NOT x15)) (EQUIV (IMPL x16 x17) (AND x18 x19))) (AND (EQUIV (NOT x20) x21) (IMPL (EQUIV x22 x23) (OR x24 x25)))))";
const char *STRING_NOT   = "NOT";
const char *STRING_AND   = "AND";
const char *STRING_OR    = "OR";
const char *STRING_IMPL  = "IMPL";
const char *STRING_EQUIV = "EQUIV";
const bool FALSE = false;
const bool TRUE  = true;

//Symbol Table is useful for debugging purposes

//const char* symbolTable[] ={"NOT", "AND", "OR", "IMPL", "EQUIV", "T", "F", "var","root", "invalid"};


enum oprtr {NOT, AND, OR, IMPL, EQUIV, T, F, var,root, invalid};

class abSyntaxTree{
	/*
	 * This class defines the basic structure of the nodes used to create an Abstract Syntax Tree
	 * Each node will have 4 fields namely:
	 * 1) type     -> defines whether the current node is a constant, operator or variable.
	 * 2) data ptr -> points to the location of variable stored in an array of variables
	 * 3) lptr 	   -> pointer to Left child node.
	 * 4) rptr     -> pointer to Right child node
	 */
	public:
		int type;
		bool *data;
		abSyntaxTree *lptr, *rptr;

	abSyntaxTree(){
		type = oprtr::invalid;
		data = (bool*)&FALSE;
		lptr = NULL;
		rptr = NULL;
	}
	~abSyntaxTree(){
		lptr = NULL;
		rptr = NULL;
		delete lptr ;
		delete rptr ;
	}

	bool readData(){
		/*
		 * Since the node holds a pointer to the data variable, accessing the variable
		 * requires pointer dereferencing. This function hence return the value assigned
		 * to the variable it (is associated with/) points to.
		 */
		return *data;
	}

};

struct Token{
	/*
	 * Useful encapsulation to pass data and type to multiple functions.
	 */
	int data, type;
};


class parser{
	/*
	 * The parser used here is a basic Recursive Descent Parser. The main role
	 * of the parser is to parser a given boolean expression and construct an
	 * Abstract Syntax tree for it. The grammar used by the parser is defined
	 * below:
	 *
	 *_______Grammer:________
	 *Formula 		::= Constant | Proposition | UnaryFormula | BinaryFormula
	 *Constant 		::= "T" | "F"
	 *Proposition 	::= [a-z0-9]+
	 *UnaryFormula 	::= LeftParen UnaryOperator Formula RightParen
	 *BinaryFormula 	::= LeftParen BinaryOperator Formula Formula RightParen
	 *LeftParen 		::= "("
	 *RightParen 		::= ")"
	 *UnaryOperator 	::= "NOT"
	 *BinaryOperator 	::= "OR" | "AND" | "IMPL" | "EQUIV"
	 *
	 *The parser takes as input a string and constructs an Abstract syntax tree
	 *for the expression in it.
	*/


	char *boolstr;
	int index = 0;
	Token c_token;
	abSyntaxTree node;

	void skipWhitespaces(){
		/*
		 * Helper function that handles white spaces by skipping over them.
		 */
			while(boolstr[index] == ' ' || boolstr[index] == '\n' || boolstr[index] == '\t')
				index++;
		}

		bool match(const char *token){
			/*
			 * The heart of the parser. This helper function matches tokens.			 *
			 */
			char *s = (boolstr + index);
			if(strncmp(s,token, strlen(token)) == 0){
				index+= strlen(token);
				return true;
			}
				return false;
		}

		abSyntaxTree* createConstNode(int type, bool val){
			/*
			 * This function creates constant (T/F) nodes. Although
			 * constants are not really expected in boolean expressions for
			 * ROBDDs, they were required to verify proper functioning of
			 * the parser.
			 *
			 */
			abSyntaxTree* node = new abSyntaxTree;
			node->lptr = NULL;
			node->rptr = NULL;
			node->data = (bool*)(val ? &TRUE : &FALSE);
			node->type = type;
			return node;
		}

		abSyntaxTree* createVarNode(int type, int id){
			/*
			 * This function creates and returns variable nodes for the
			 * abstract syntax tree. It also associates each node to it's
			 * corresponding data variable in the variable array
			 */
			abSyntaxTree* node = new abSyntaxTree;
			node->lptr = NULL;
			node->rptr = NULL;
			node->data = variables + id-1;
			node->type = type;
			var_list[numVar++] = id;
			return node;
		}

		abSyntaxTree* createOptrNode(int type){
			/*
			 * This function creates Operator nodes and associates
			 * an operation with them. This association is later used
			 * to evaluate the boolean expression. By default it's *data
			 * field pints to a constant FALSE.
			 */
			abSyntaxTree* node = new abSyntaxTree;
			node->lptr = NULL;
			node->rptr = NULL;
			node->data = (bool*)&FALSE;
			node->type = type;
			return node;
		}

		bool leftParen(){
			/*
			 * Helper function that identifies the beginning of
			 * unary/binary formulas. The left parenthesis is
			 * considered to be the grammatical starting point
			 * of unary/binary formulas
			 */
			skipWhitespaces();
			int orignal = index;
			if(match("(")){
				return true;
			}
			index  =  orignal;
			return false;
		}

		bool rightParen(){
			/*
			 * Identifies the Right Parenthesis, which is the
			 * grammatical end-point of unary/binary subexpressions.
			 */
			skipWhitespaces();
			int orignal = index;
			if(match(")")){
				return true;
			}
			index  =  orignal;
			return false;
		}

		abSyntaxTree* constant (){
			/*
			 * Identifies constants and creates constant nodes.
			 * It returns the address of node created.
			 */
			skipWhitespaces();
			int orignal = index;
			if(match("T")){
				c_token.type = T;
				c_token.data = true;
				return createConstNode(T,true);
			}
			else if(match("F")){
				c_token.type = F;
				c_token.data = false;
				return createConstNode(F,false);
			}
			index = orignal;
			return NULL;
		}

	abSyntaxTree* proposition (){
		/*
		 * This function identifies propositions/ variables in
		 * the input boolean string and creates variable nodes for them.
		 * It returns the address of the node created.
		 */
		skipWhitespaces();
		int orignal = index, id;
		char *prop = new char[MAX_PROP_LENGTH];
		char *num;
		int numCharsRead;
		if(sscanf((boolstr+index),"%[a-z0-9] %n",prop, &numCharsRead) == 1){
			index += numCharsRead;
			num = (prop+1);
			id = atoi(num);
			return createVarNode(var,id);
		}
		index = orignal;
		delete(prop);
		return NULL;
	}

	abSyntaxTree* unaryOptr(){
		/*
		 * Identifies unary operator (NOT) in the boolean string
		 * and creates a node for it. It returns the address of the
		 * operator node
		 *
		 */
		int orignal = index;
		if(match(STRING_NOT)){
			return createOptrNode(NOT);
		}
		index = orignal;
		return NULL;
	}

	abSyntaxTree* binaryOptr(){
		/*
		 * Identifies binary operators (AND, OR, EQUIV, IMPL) in the
		 * boolean string and creates a node for it. It returns the
		 * address of the operator node		 *
		 */

		skipWhitespaces();
		int orignal = index;
		if(match(STRING_AND)){
			return createOptrNode(AND);
		}
		else if(match(STRING_OR)){
			return createOptrNode(OR);
		}
		else if(match(STRING_IMPL)){
			return createOptrNode(IMPL);
		}
		else if(match(STRING_EQUIV)){
			return createOptrNode(EQUIV);
		}
		else{
			index = orignal;
			return NULL;
		}
	}

	abSyntaxTree* unaryFormula(){
		/*
		 * This function defines the structure of a unary formula,
		 * i.e. a valid subexpression containing a unary operator.
		 * It parses tokens and checks whether they satisfy the
		 * defined grammatical sequence. It also constructs the sub-tree
		 * that defines the subexpression and then returns the address
		 * of the root node of the sub tree.
		 */
		skipWhitespaces();
		abSyntaxTree *node;
		int orignal = index;
		if(!leftParen()){
			index = orignal;
			return NULL;
		}
		node = unaryOptr();
		if(node == NULL){
			index = orignal;
			return NULL;
		}

		node->rptr = NULL;
		node->lptr = formula();

		if(node->lptr == NULL){
			index = orignal;
			return NULL;
		}

		if(!rightParen()){
			index = orignal;
			return NULL;
		}

		return node;
	}

	abSyntaxTree* binaryFormula(){
		/*
		 * This function defines the structure of a binary formula,
		 * i.e. a valid subexpression containing a binary operator.
		 * It parses tokens and checks whether they satisfy the
		 * defined grammatical sequence. It also constructs the sub-tree
		 * that defines the subexpression and then returns the address
		 * of the root node of the sub tree.
		 */
		skipWhitespaces();
		abSyntaxTree *node;
		int orignal = index;
		if(!leftParen()){
			index = orignal;
			return NULL;
		}

		node = binaryOptr();

		if(node == NULL){
			index = orignal;
			return NULL;
		}

		node->lptr = formula();

		if(node->lptr == NULL){
			index = orignal;
			return NULL;
		}

		node->rptr = formula();

		if(node->rptr == NULL){
			index = orignal;
			return NULL;
		}
		if(!rightParen()){
			index = orignal;
			return NULL;
		}

		return node;
	}

	abSyntaxTree* formula(){
		/*
		 * This function defines the structure of any boolean
		 * expression that can be parsed by the parser. It constructs the
		 * abstract syntax tree and returns the address of the root node to
		 * calling function.
		 */

			int orignal = index;
			skipWhitespaces();
			abSyntaxTree* Graph = new abSyntaxTree;
			abSyntaxTree* node = constant();
			Graph->type = root;

			if(node!= NULL){
				return node;
				}

			node = proposition();
			if(node != NULL){
				return node;
			}

			node = unaryFormula();
			if(node!=NULL){
				return node;
			}

			node = 	binaryFormula();
			if(node!=NULL){
				return node;
			}

			index = orignal;
			return NULL;
		}


public:

	bool variables[300] = {false};
	unsigned var_list[300] ={0};
	int numVar = 0;

	abSyntaxTree* formulaWrapper(char *s) {
		/* This wrapper function ensures that parsing continues
		 * until the end of the boolean string is reached. This function will be called by
		 * main().
		 */
		 char *original = s;
		 boolstr = s;
		 index = 0;
		 abSyntaxTree *AST = new abSyntaxTree;
		 numVar = 0;
		 AST = formula();
		 if( AST != NULL) {
			skipWhitespaces();
			if ((int)*(s+index) == 0){
				printf("\nParsing Successful\n");
			   return AST;
			}
			else {
			   printf("\n there was junk in the string!: %d", *(boolstr+index));
			   fflush(stdout);
			   s = original;
			   delete (AST);
			   return NULL;
			}
		 }
		 delete (AST);
		 return NULL;
	  }

};

class evaluator{
	/*
	 * This class defines an evaluator which evaluates an
	 * abstract syntax tree (containing constants).
	 */
	bool evalSubTree(abSyntaxTree* node){
		/*
		 * Evaluates a Subexpression rooted at node by performing a DFS.
		 * It uses INF forms of all operators to evaluate expressions.
		 *
		 */
		if (node == NULL){
			return false;
		}
		if(node->type == T){
			return true;
		}
		else if(node->type == F){
			return false;
		}
		else if(node->type == NOT){
			return ((node->lptr->readData()) ? false : true);
		}
		else if(node->type == var){
			//printf("var val: %d",node->readData());
			return node->readData();
		}
		else{
			bool v1 = evalSubTree(node->lptr);
			bool v2 = evalSubTree(node->rptr);
			switch(node->type){
			case OR:    return (v1 ? true : v2);
			case AND:   return (v1 ? v2 : false);
			case IMPL:  return (v1 ? true : (v2 ? false : true));
			case EQUIV: return (v1 ? v2 : (v2 ? false : true));
			case T: 	return true;
			case F:     return false;
			case var:   return (node->readData());
			}
		}

		return false;
	}

public:
	bool evaluate(abSyntaxTree* node){
		/*
		 * wrapper function that checks if node received is not NULL.
		 */
		if(node == NULL){
			printf("Incorrect abstract syntax Tree");
			fflush(stdout);
		}
		return evalSubTree(node);
	}
};

void evaluate(abSyntaxTree* node){
	/*
	 * wrapper function that calls evaluator function
	 */
	evaluator e;
	if (node != NULL){
		bool val = e.evaluate(node);
		printf(" = %c \n", val ? 'T' : 'F');
		fflush(stdout);
   }
   else {
	   printf("\nINVALID EXPRESSION :(");
	   fflush(stdout);
   }
}

class ROBDD{

	/*
	 * This class defines ROBDDs and operations necessary to construct ROBDDS.
	 * It also defines functions associated with ROBDDs such as Apply, Restrict,
	 * SatCount and Any Sat.
	 *
	 * The class uses static data structures and predefines the size of Tables T, H
	 * and G. The sizes assigned are assumed to be reasonable enough to represent
	 * boolean expressions with a large number of variables.
	 *
	 */
	enum tableT {varnum,var_index,low_node,high_node};
	enum tableHG {membership,varid};

	unsigned numVars;
	unsigned index_u;

	bool *variables;
	unsigned *var_list;
	evaluator e;
	abSyntaxTree *t;



	unsigned pair(unsigned i, unsigned j){
		/*
		 * A bijective function defined in the Andersen paper. The paper
		 * suggests the use of this function for collision free hashing.
		 */
		return (((i+j)*(i+j+1))/2 + i);
	}
	unsigned hash(unsigned i,unsigned j, unsigned k){
		/*
		 * A hash function defined in the Andersen paper. The paper
		 * suggests the use of this function for collision free hashing.
		 * It uses the prime number 523 to achieve this.
		 */
		return (pair(i,pair(j,k)) % 523);
	}
public:
	unsigned T[523][4];		 // stores adjacency list of the ROBDD
	unsigned H[523][2]= {{0}}; // used for fast lookup of existing entries
	unsigned G[523][2]= {{0}}; // used for Apply()

	//used to keep track of number entries in table T.

	//used to store satisfiability inputs and pass them to calling function
	std::vector<int> x;
	int *arr = NULL;
	unsigned setSize = 0;

	unsigned read_index(){
			return index_u;
		}

	void ROBDD_init(unsigned max_size, bool *varptr,unsigned *var_listptr, abSyntaxTree *node ){
		/*
		 * This type of constructor was chosen because every ROBDD would require come inputs from the
		 * parser to initialize construction. Hence, this pseudo-constructor accepts inputs from
		 * parser (via main()) to initialize table T, associates array of variables with a pointer,
		 * initializes variable number index (index_u), associates root node of abstract syntax tree
		 * with local node variable and reserves space needed for finding satisfiability inputs.
		 */
		T[1][varnum] = 1; T[0][varnum] = 0;
		T[1][var_index] = T[0][var_index] =  max_size+1;
		T[1][low_node] = 1; T[0][low_node] = 0;
		T[1][high_node] = 1; T[0][high_node] = 0;
		numVars = max_size;
		variables = varptr;
		var_list = var_listptr;
		index_u = 2;
		x.reserve(numVars);
		arr = new int[numVars];
		t = node;
	}

	bool isMember(unsigned i, unsigned l, unsigned h){
		/*
		 * Checks if given variable is already a member of T
		 * by looking it up in Table H. If found it returns
		 * the index of the entry.
		 */
		unsigned int key =  hash(i,l,h);
		if(H[key][tableHG::membership] == 1){
			return true;
		}
	return false;
	}

	unsigned lookup(unsigned i, unsigned l, unsigned h ){
		/*
		 * lookup function to find index of queried entry
		 */
		unsigned int key =  hash(i,l,h);
		return (H[key][tableHG::varid]);
	}

	void insertInH(unsigned i, unsigned l, unsigned h, unsigned u){
		/*
		 * Inserts new entry to Table H to allow fast lookup of
		 * redundant entries.
		 */
			unsigned int key = hash(i,l,h);
			H[key][tableHG::membership] = 1;
			H[key][tableHG::varid]     = u;
		}

	unsigned addToT(unsigned i, unsigned l, unsigned h){
		/*
		 * Adds an entry with given arguments to the Table T.
		 */
		unsigned u = index_u++;
		T[u][tableT::varnum]    = u;
		T[u][tableT::var_index] = i;
		T[u][tableT::low_node]  = l;
		T[u][tableT::high_node] = h;
		return u;
	}

	unsigned Mk(unsigned i, unsigned l, unsigned h){
		/*
		 * Implementation of the Mk[T,H] algorithm defined in the Andersen Paper
		 */
		unsigned index;
		if(l == h){
			return l;
		}
		else if(isMember(i,l,h)){
			return lookup(i,l,h);
		}
		else{
			index = addToT(i,l,h);
			insertInH(i,l,h,index);
		}
		return index;
	}

	void build(){
		/*
		 * Implementation of the build algorithm from the Andersen paper.
		 * This function serves as a wrapper to the recursive operation
		 * of the build function.
		 */
		build_(e.evaluate(t),0);
	}

	unsigned build_(bool val, unsigned i){
		/*
		 * Implements the recursive operation of the build algorithm
		 */
		unsigned v0,v1;
		bool eval;
		if (i >= numVars){
			if(val){
				return 1;
			}
			else{
				return 0;
			}
		}
		else{
			variables[var_list[i]-1] = false;
			eval = e.evaluate(t);
			v0 = build_(eval , i+1);
			variables[var_list[i]-1] = true;
			eval = e.evaluate(t);
			v1 = build_(eval , i+1);
		}

		return Mk(var_list[i],v0,v1);
	}

	void Apply(ROBDD* r1, int op){
		/*
		 * Implements the Apply algorithm of the Andersen paper.
		 * It initializes the variable indices 'var(u)'of the constant nodes to the
		 * worst case maximum value (i.e. when both ROBDDs have no variables in common).
		 * This is done to ensure that the comparison operations of the Apply operation do not
		 * form a cycle. Otherwise the constant node of one ROBDD would try and operate with
		 * the constant node of another ROBDD. This would lead to a cycle and hence the program
		 * would be lost in an infinite loop.
		 *
		 * I do not want infinite loops here.
		 */
		r1->T[0][var_index]=r1->T[1][var_index]= numVars + r1->numVars;
		T[0][var_index] = T[1][var_index] = numVars + r1->numVars;
		unsigned u1, u2;
		u1 = r1->numVars;
		u2 = numVars;
		Apply_(u1,u2,op,r1);
		}

	unsigned Apply_(unsigned u1, unsigned u2, int op, ROBDD* R1){
		/*
		 * Performs the recursive operation of Apply Algorithm.
		 */
		unsigned ind, u;
		ind = pair(u1,u2);
		if(G[ind][0] != 0){
			return G[ind][1];
		}
		else if ((R1->T[u1][varnum] == 0 || R1->T[u1][varnum] == 1) &&
				(T[u2][varnum] == 0 || T[u2][varnum] == 1)){
			u = operate(op, R1->T[u1][varnum], T[u2][varnum]);
		}
		else if(R1->T[u1][var_index] == T[u2][var_index]){
			u = Mk(R1->T[u1][var_index],Apply_(R1->T[u1][low_node],T[u2][low_node],op, R1),
					Apply_(R1->T[u1][high_node],T[u2][high_node],op,R1));
		}
		else if(T[u2][var_index] > R1->T[u1][var_index] ){
			u = Mk(R1->T[u1][var_index],Apply_(R1->T[u1][low_node],T[u2][var_index],op,R1),
								Apply_(R1->T[u1][high_node],T[u2][var_index],op,R1));
		}
		else /*R1->[u1][var_index] > T[u2][var_index]*/{
			u = Mk(T[u2][var_index],Apply_(R1->T[u1][var_index],T[u2][low_node],op, R1),
								Apply_(R1->T[u1][var_index],T[u2][high_node],op,R1));
		}
		G[ind][0] = 1; G[ind][1] = u;
		return u;
	}

	bool operate(unsigned op, unsigned v1, unsigned v2){
		/*
		 * Called by the Apply function to operate on terminal nodes.
		 */

		switch(op){
			case OR:    return (v1 ? true : v2);
			case AND:   return (v1 ? v2 : false);
			case IMPL:  return (v1 ? true : (v2 ? false : true));
			case EQUIV: return (v1 ? v2 : (v2 ? false : true));
			default:
				printf("\nINVALID OPERATOR. %d", op);
				exit(0);
			}
	}

	void Restrict(unsigned u, int j, bool b, ROBDD *Reduced){
		/*
		 * Implements the Restrict Algorithm of the Andersen Paper. This
		 * function serves as a wrapper to the recursive operation of the
		 * algorithm. It also initializes a new ROBDD variable to store the
		 * restricted ROBDD.
		 */
		Reduced->T[0][var_index] = Reduced->T[1][var_index] = numVars;
		Reduced->T[1][varnum] = 1; Reduced->T[0][varnum] = 0;
		Reduced->T[1][low_node] = 1; Reduced->T[0][low_node] = 0;
		Reduced->T[1][high_node] = 1; Reduced->T[0][high_node] = 0;
		Reduced->index_u = 2;
		res(u,j,b,Reduced);
	}

	unsigned res(unsigned u, unsigned j, bool b, ROBDD* R){
		/*
		 * This function performs the recursive operation of the Restrict algorithm.
		 * It returns constant values when the function is called with terminal nodes indices.
		 * This is because on this implementation, the terminal nodes have their low and high
		 * nodes mapped back to themselves.
		 */
		if(u == 0){
			return 0;
		}
		else if(u==1){
			return 1;
		}
		if(T[u][var_index] > j){
			R->Mk(T[u][var_index],T[u][low_node],T[u][high_node]);
			return T[u][varnum];
		}
		else if(T[u][var_index] < j){
				return (R->Mk(T[u][var_index], res(T[u][low_node],j,b,R),res(T[u][high_node],j,b,R)));
		}
		else if(b == 0){
			return res(T[u][low_node],j,b,R);
		}
		else{
			return res(T[u][high_node],j,b,R);
		}
	}

	int SatCount(unsigned u){
		/*
		 * Implements the SatCount algorithm from the Andersen paper.
		 */
		return pow(2,((T[u][var_index]-1)))*count(T[u][varnum]) ;
	}

	int count(unsigned u){
		/*
		 * Implements the count() function that is called recursively in the SatCount
		 * Algorithm. It computes the number of possible true assignments by performing
		 * a DFS through the ROBDD.
		 */
		unsigned val,index_low,index_high;
		if(u == 0){
			return 0;
		}
		else if(u==1){
			return 1;
		}
		else{
			index_low = T[u][low_node];
			val = pow(2,((T[index_low][var_index] - T[u][var_index] - 1)))*count(T[u][low_node]);
			index_high = T[u][high_node];
			val += pow(2,((T[index_high][var_index] - T[u][var_index] - 1)))*count(T[u][high_node]);

			return val;
		}
	}

	int AnySat(unsigned u){
		/*
		 * Implements the AnySat() algorithm of the Andersen paper that finds
		 * an input assignment that satisfies the boolean expression represented by the
		 * ROBDD
		 */
		setSize = 0;
		return AnySat_(u);
	}
	int AnySat_(unsigned u){
		/*
		 * Implements the recursive operation of AnySat algorithm
		 */
		int val = -1;
		if(T[u][varnum] == 0){
			return 0;
		}
		else if(T[u][varnum] == 1){
			return -1;
		}
		else if(T[u][low_node] == 0){
			arr[setSize] = 1;
			setSize++;
			val = AnySat_(T[u][high_node]);
			if(val!= -1){
				arr[setSize] = val;
			}
		}
		else {

			arr[setSize] = 0;
			setSize++;
			val = AnySat_(T[u][low_node]);
			if( val != -1){
				arr[setSize] = val;
				}
			}
			return setSize;
		}
};

void Apply_ROBDD(ROBDD k, unsigned numVar_k){
	/*
	 * Wrapper function to Apply(). It accepts and parses another
	 * boolean expression, creates it's ROBDDD and calls the Apply operation
	 * on both the ROBDDs.
	 */

	char input[1000];	unsigned optr,c;
	parser p2;		abSyntaxTree* exp_2;
	ROBDD m;

	printf("\nEnter next expression: ");
	fflush(stdout);
 	while ( (c = getchar()) != '\n' && c != EOF );
	fgets(input,MAX_LINE_SIZE,stdin);
		if( input == NULL){
			printf("\nIncorrect input");
			fflush(stdout);
		}
	printf("%s",input);
	fflush(stdout);
	exp_2 = p2.formulaWrapper(input);
	m.ROBDD_init(p2.numVar, p2.variables,p2.var_list, exp_2);
	m.build();
	ROBDD r;
scan:
/*
 * Presents choice of Operations on ROBDD
 */
	printf("\n Enter Operand key: \n[1]AND\n[2]OR\n[3]IMPL\n[4]EQUIV");
	fflush(stdout);
	scanf("%d",&optr);
	printf(" %d",optr);
	if(optr>0 && optr<5){
		m.Apply(&k,optr);
	}

	else{
		printf("invalid choice\n\n");
		fflush(stdout);
		goto scan;
	}
	// prints out the adjacency list of the new ROBDD
	for(unsigned i = 0; i< (m.read_index() +1); i++){
		printf("\n%d %d %d %d", m.T[i][0],m.T[i][1],m.T[i][2],m.T[i][3]);
		fflush(stdout);
	}

}

void Restrict_ROBDD(ROBDD k, unsigned NumVars){
	/*
	 * Wrapper to the Restrict function. It accepts the variable and it's
	 * assignemnt with which to restrict the given ROBDD.
	 */
	ROBDD temp;
	unsigned index,val;
	fflush(stdout);
	printf("\n Enter the variable which you wish to Restrict with: "); fflush(stdout);
	scanf(" x%d",&index);
	printf("\nIndex: %d\n Enter value to co-factor (0/1): ",index); fflush(stdout);
	scanf(" %d",&val);
	k.build();
    k.Restrict(NumVars+2,(unsigned)index,val, &temp);

	printf("\nRestricted Adjacency List:");
	fflush(stdout);

	//prints the Adjacency List of the  restricted ROBDD
    for(unsigned i = 0; i<temp.read_index() + 1; i++){
    			printf("\n%d %d %d %d", temp.T[i][0],temp.T[i][1],temp.T[i][2],temp.T[i][3]);
    			fflush(stdout);
    		}
}

void SatCount_ROBDD(ROBDD k, int numVar){
	/*
	 * Wrapper to SatCount function. Allows the user to choose between
	 * computing for all nodes or for a specific node. It accepts the variable
	 * number as input.
	 */
	int val, option,u, index;
	printf("\nprint for all nodes? (Yes = 1 / No = 0)"); fflush(stdout);
	scanf("%d",&option);
	index= k.read_index();
	if(!option){
		printf("\n Enter Node number (u) : "); fflush(stdout);
		scanf("%d",&u);
		val = k.SatCount(u);
		printf("\nNo. of values that satisfy node %d = %d ", u, val); fflush(stdout);

	}
	else{
		for (int i = 0; i < index ; i++){
			val = k.SatCount(i);
			printf("\nNo. of values that satisfy node %d = %d ", k.T[i][0], val); fflush(stdout);
		}
	}
}

void AnySat_ROBDD(ROBDD k, int numVar){
	/*
	 * Wrapper to SatCount function. Allows the user to choose between
	 * computing for all nodes or for a specific node. It accepts the variable
	 * number as input.
	 */
	int val, option,u, index;
	index = k.read_index();
	printf("\nprint for all nodes?(Yes = 1 / No = 0)"); fflush(stdout);
	scanf("%d",&option);
	if(!option){
		printf("\n Enter Node number (u) : "); fflush(stdout);
		scanf("%d",&u);
		val = k.SatCount(u);
		printf("\nNo. of values that satisfy node %d = %d ", u, val); fflush(stdout);

	}
	else{
			for (int i = 0; i < index ; i++){
				k.AnySat(i);
				printf("\nSatisfiability set for node%d : [", k.T[i][0]); fflush(stdout);
				for (unsigned j = 0; j < k.setSize ; j++){
					printf(" %d", k.arr[j]);
				}
				printf(" ]"); fflush(stdout);

			}
	}
}

int main(){
	/*
	 * Where the Magic Happens!!
	 *
	 * The main function is designed to serve as a UI to the whole application.
	 * It has a Menu system that allows the user to choose which operation must be performed
	 * on the ROBDD.
	 */


	abSyntaxTree *exp_1;
	ROBDD k; parser p1;


	char input[MAX_LINE_SIZE];
	char *copy;
	int option,c;

new_exp:	// prompt user for boolean expression. Example expression given to explain grammar

printf(" Enter formulae of proposition logic, one per line (max %d characters)", MAX_LINE_SIZE);fflush(stdout);
	printf("\n Syntax is prefix. \nEg: ((!F OR F) IMPL ((F EQUIV T) AND T) in infix becomes");fflush(stdout);
	printf("\n (IMPL (OR F (NOT F)) (AND (EQUIV F T) T)) in prefix \n");
	fflush(stdout);
	//fgets(input,MAX_LINE_SIZE,stdin);
	if( input == NULL){
		printf("\nIncorrect input");
		goto new_exp;
	}
	//printf("\n%s",input);
	//copy = input;
	copy = BIG_TEST_STRING; 	
	exp_1 = p1.formulaWrapper(copy);
	k.ROBDD_init(p1.numVar,p1.variables,p1.var_list, exp_1);
	k.build();

	printf("\nPrinting Adjacency List of ROBDD.\nNote: Constant nodes 0/1 will be mapped to themselves\n"); fflush(stdout);
	printf("\nu i l h"); fflush(stdout);
// print adjacency list of Boolean expression received from user
	for(unsigned i = 0; i<=k.read_index(); i++){
			printf("\n%d %d %d %d", k.T[i][0],k.T[i][1],k.T[i][2],k.T[i][3]);
			fflush(stdout);
		}
menu:
// Menu of Choices for operations on ROBDD
	printf("\n\n\tMENU\n \n1.Apply\n2.Restrict\n3.Satisfiability Count\n4.AnySat check\n5.Enter new expression\n6.QUIT");
	fflush(stdout);
	printf("\nEnter Operation you wish to perform:");
	fflush(stdout);
	//scanf("%d",&option);
	option = 6;

	if(option < 0 && option > 7){
		printf("INVALID option. Program Terminated.");
		fflush(stdout);
		exit(0);
	}
	switch(option){
	case 1: Apply_ROBDD(k, p1.numVar);
		goto menu; break;
	case 2: Restrict_ROBDD(k,p1.numVar);
		goto menu; break;
	case 3: SatCount_ROBDD(k,p1.numVar);
		goto menu; break;
	case 4: AnySat_ROBDD(k,p1.numVar);
		goto menu; break;
	case 5:  while ( (c = getchar()) != '\n' && c != EOF );
		goto new_exp;


	case 6: printf("\n\nThank you for your time! Program Terminated."); fflush(stdout);break;

	// for the keyboard slips...
	default: printf("Invalid Choice!"); fflush(stdout); break;
	}
	return 0;
}


