/*
	TODO:
		IMPLEMENT A METHOD TO CHECK FOR HAZARDS WITH THE RS RT RD STUFF
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>	// For find
#include <cctype>


// Size of each things in RTYPE or ITYPE
#define OPCODE_BIT_SIZE 6
#define RS_BIT_SIZE 5
#define RT_BIT_SIZE 5
#define RD_BIT_SIZE 5
#define SHIFT_BIT_SIZE 5
#define FUNCT_BIT_SIZE 6
#define IMMEDIATE_BIT_SIZE 16
#define ADDRESS_BIT_SIZE 26

int branchToPC = 0;
bool doBranch = false;
bool complete = false;



/*
*/
class Instructions
{
public:
	Instructions()
	{
		rs = rt = rd = immediate = shamt = 0;
		type = function = "";
	}
	Instructions(std::string _instruct, int _instructionNum)
	{
		// Initialize this objects instance's values
		this->rs = this->rt = this->rd = this->immediate = this->shamt = NULL;
		this->function = "";
		type = "Default";
		this->instruction = _instruct;

		this->instructionNum = _instructionNum;
		this->PC = (instructionNum - 1) * 4;
	}
	std::string get_Instruction() { return instruction; };


	int rs, rt, rd;	// Holding values for source, destinatin1, destination2
	int immediate;	// Holding value for if the Instruction is I-type
	int shamt;	// Holding value for the shift amount
	std::string type;	// Holding value for what type this is
	std::string function;	// Holding value for what function code represents
	int instructionNum = 0;	// Holding value for order of instruction
	int PC;
	bool regWrite = false;	// Indicates if instruction performs reg access
	bool memWrite = false;	// Indicates if instruction performs memory access

	int newRegisterValue;	// Holding value for value to save to register
	int newMemoryValue;		// Holding value for value to save to memory

	std::string state = "NONE";	// Holding value for which stage instruction is in, i.e. IF -> ID -> ....

	bool done = false;	// Tell if this instruction has completed all stages

	// Flags used to know if stage is complete
	bool IF = false;
	bool ID = false;
	bool EX = false;
	bool MEM = false;
	bool WB = false;

	Instructions *next;	// Holding value for pointing to instruction after
	Instructions *prev;	// Holding value for pointing to instruction before
private:
	std::string instruction;
};

struct IF_ID
{
	void setValues(Instructions &_instruction)
	{
		
		rS = _instruction.rs;
		rT = _instruction.rt;
		rD = _instruction.rd;

		n = &_instruction;
	}
	void reset()
	{
		rS = NULL;
		rT = NULL;
		rD = NULL;
		n = NULL;
	}
	int rS = NULL;
	int rT = NULL;
	int rD = NULL;

	Instructions *n = NULL;
};

struct ID_EX
{
	void setWriteRegister(Instructions &_instruction)
	{
		if (_instruction.type == "RTYPE")
		{
			rD = _instruction.rd;
		}
		else rD = _instruction.rt;


		rS = _instruction.rs;
		rT = _instruction.rt;
		n = &_instruction;
	}
	void reset()
	{
		rS = NULL;
		rT = NULL;
		rD = NULL;
		n = NULL;
	}
	int rS = NULL;
	int rT = NULL;
	int rD = NULL;
	Instructions *n = NULL;
};

struct EX_MEM
{
	void setWriteRegister(Instructions &_instruction)
	{
		if (_instruction.type == "RTYPE")
		{
			rD = _instruction.rd;
		}
		else rD = _instruction.rt;

		n = &_instruction;
	}
	void reset()
	{
		rD = NULL;
		n = NULL;
		regWrite = false;
		n = NULL;
	}
	int rD;
	bool regWrite = false;
	Instructions *n = NULL;

};

struct MEM_WB
{
	void setWriteRegister(Instructions &_instruction)
	{
		if (_instruction.type == "RTYPE")
		{
			rD = _instruction.rd;
		}
		else rD = _instruction.rt;

		n = &_instruction;
	}
	
	void reset()
	{
		rD = NULL;
		n = NULL;
		regWrite = false;
		n = NULL;
	}

	int rD = NULL;
	bool regWrite = false;
	Instructions *n = NULL;

};

// List of instructions as doubly linked list
class list_Instructions
{
public:
	Instructions *top = NULL, *tail = NULL;
	void display_List();
	void insert_Instruction(std::string _instruction, int _instructionNum);
	int Get_Size();
	int size = 0;
};
// Make fuction that returns size
int list_Instructions::Get_Size()
{
	return size;
}

void list_Instructions::display_List()
{
	Instructions *n = top;
	while (n != NULL)
	{

		std::cout << n->instructionNum << ": " << n->get_Instruction() << std::endl;
		std::cout << "\t PC: " << n->PC << std::endl;
		std::cout << "\t Type: " << n->type << std::endl;
		std::cout << "\t rs: " << n->rs << std::endl;
		std::cout << "\t rt: " << n->rt << std::endl;
		std::cout << "\t rd: " << n->rd << std::endl;
		std::cout << "\t immediate: " << n->immediate << std::endl;
		std::cout << "\t shamt: " << n->shamt << std::endl;
		std::cout << "\t function: " << n->function << std::endl;
		std::cout << "\t state: " << n->state << std::endl;
		n = n->next;
	}
}

// Add new instruction to end of instruction list
void list_Instructions::insert_Instruction(std::string _instruction, int _instructionNum)
{
	// If there is nothing inside list
	if (top == NULL)
	{
		top = new Instructions(_instruction, _instructionNum);
		tail = top;
		top->next = NULL;
		tail->next = NULL;
		top->prev = NULL;
		tail->prev = NULL;
		size++;
	}
	// If the list is already occupied
	else if(top != NULL)
	{
		// Create a new Instruction node
		Instructions *n = new Instructions(_instruction, _instructionNum);
		tail->next = n;	// Make old node point to new node
		n->prev = tail;	// Make new node point to old node
		tail = n;	// Make tail become new node
		tail->next = NULL;	// Tail should point to nothing after it
		size++;	// Increase thhe number of Instruction/nodes inside the list
	}
}

struct Register
{
	int R[32];
	std::vector<int> usedRegisters;
	Register();
};

// Initialize instance with all zeroes
Register::Register()
{
	std::fill(R, R + 32, 0);
}

struct Mem
{
	int M[250];
	std::vector<int> usedMemory;
	Mem();
};

Mem::Mem()
{
	std::fill(M, M + 32, 0);
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> _tokens;

	while (std::getline(ss, item, delim))
	{
		_tokens.push_back(item);
	}

	return _tokens;
}

void initFile(const char *fileName, Mem *_memory, Register *_registers, list_Instructions &_List_Instructions)
{
	std::string inString = "";	// Holding value for line of text
	std::string codeString = "";	// Holding value for line of 32bit Instruction
	std::vector<std::string> tokens;	// Holding vector for splitting the inString
	std::vector<std::string> instructions;	// 
	std::ifstream inFile;
	int regLocation = NULL;
	int memLocation = NULL;
	inFile.open(fileName);

	// Check if the file is opened
	if (inFile.is_open())
	{
		std::cout << "File opened" << std::endl;

		// Go through each line
		while (std::getline(inFile, inString))
		{
			//std::cout << inString << std::endl;
			// Separate line by white space
			tokens = split(inString, ' ');

			// Only if there are two values in tokens ex. " R1 and 23"
			if (tokens.size() > 1)
			{
				//std::cout << "These are the separations" << std::endl;

				// Checking for parsing
				for (int i = 0; i < tokens.size(); i++)
				{
					//std::cout << i << ": " << tokens[i] << " ";
				}
				std::cout << std::endl;

				// Check if the first character of the first element of tokens string is a letter
				if (isalpha(tokens.front().front()))
				{

					// Erase the first element
					tokens.front().erase(0, 1);
					// Convert tokens' first element to int
					regLocation = atoi(tokens.front().c_str());
					

					// If a regLocation is found
					if (regLocation > 0)
					{
						//std::cout << "Reg Location" << std::endl;
						//std::cout << regLocation << std::endl;

						// Store the second token, which is value, into that register location
						_registers->R[regLocation] = atoi(tokens.at(1).c_str());
						_registers->usedRegisters.push_back(regLocation);
					}

				}
				// The first character of the first element of vector tokens is a numeric value therefore memory
				else if (isdigit(tokens.front().front()))
				{
					// Convert the first token to an int value
					// Divide by 4 because 0, 4, 8,... 996 memory locations
					memLocation = atoi(tokens.front().c_str()) / 4;
					// Save value at memLocation
					_memory->M[memLocation] = atoi(tokens.back().c_str());
					_memory->usedMemory.push_back(memLocation);
					//std::cout << "Mem[" << memLocation << "]" << " = " << _memory->M[memLocation] << std::endl;
				}

			}

			// Check if the 1 token is CODE
			else if (!strcmp("CODE", inString.c_str()))
			{
				//std::cout << "CODE IS SEEN" << std::endl;
				int _instructionCount = 1;
				// Go through here
				while (std::getline(inFile, codeString))
				{
					//std::cout << "while " << codeString << std::endl;
					// Make a new instance of Instructions
					//_list_Instructions.push_back(Instructions(codeString));
					_List_Instructions.insert_Instruction(codeString, _instructionCount);
					_instructionCount++;
					
				}
			}

			//std::cout << std::endl;


		}

	}
	else
	{
		std::cout << "File cannot be opened" << std::endl;
	}
}

// What operation the instruction needs to take
void get_Function(Instructions &_instruction)
{
	if (_instruction.type == "RTYPE")
	{
		_instruction.regWrite = true;
		// Switch based on Function of RTYPE
		switch (strtoul(_instruction.function.c_str(), NULL, 2))
		{
			// Add
		case 32:
			_instruction.function = "ADD";
			break;
			// Subtract
		case 34:
			_instruction.function = "SUB";
			break;
			// SLT
		case 42:
			_instruction.function = "SLT";
			break;

		default:
			break;

		}
	}
	else if (_instruction.type == "ITYPE")
	{
		// Switch based on OPCODE of ITYPE
		switch (strtoul(_instruction.get_Instruction().substr(0, OPCODE_BIT_SIZE).c_str(), NULL, 2))
		{
			// SW
		case 43:
			_instruction.function = "SW";
			_instruction.memWrite = true;
			break;
			// LW
		case 35:
			_instruction.function = "LW";
			_instruction.regWrite = true;
			break;
			// ADDi
		case 8:
			_instruction.function = "ADDi";
			break;
			// BNE
		case 5:
			_instruction.function = "BNE";
			break;
			// BEQ
		case 4:
			_instruction.function = "BEQ";
			break;
		default:
			break;
		}
	}

}

// Perform the fetching aspect
void performIF(Instructions &_instruction, Register &_registers, FILE *_fp)
{

	// Get the opcode string
	std::string opcode = _instruction.get_Instruction().substr(0, OPCODE_BIT_SIZE);
	// Convert from Binary to Decimal
	if (std::strtoul(opcode.c_str(), NULL, 2) == 0)
	{
		_instruction.type = "RTYPE";
		// Get the RT int value
		_instruction.rs = strtoul(_instruction.get_Instruction().substr(OPCODE_BIT_SIZE, RS_BIT_SIZE).c_str(), NULL, 2);
		// Get RT
		_instruction.rt = strtoul(_instruction.get_Instruction().substr(RS_BIT_SIZE + OPCODE_BIT_SIZE, RT_BIT_SIZE).c_str(), NULL, 2);
		// Get RD
		_instruction.rd = strtoul(_instruction.get_Instruction().substr(RT_BIT_SIZE + OPCODE_BIT_SIZE + RS_BIT_SIZE, RD_BIT_SIZE).c_str(), NULL, 2);
		// Get SHAMT
		_instruction.shamt = strtoul(_instruction.get_Instruction().substr(OPCODE_BIT_SIZE + RS_BIT_SIZE + RT_BIT_SIZE + RD_BIT_SIZE, SHIFT_BIT_SIZE).c_str(), NULL, 2);
		// Get FUNCT
		_instruction.function = _instruction.get_Instruction().substr(OPCODE_BIT_SIZE + RS_BIT_SIZE + RT_BIT_SIZE + RD_BIT_SIZE + SHIFT_BIT_SIZE, FUNCT_BIT_SIZE).c_str();

		// Find out if new register has been modified, so add it
		std::vector<int>::iterator it;
		it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), _instruction.rs);
		if (it == _registers.usedRegisters.end())
		{
			_registers.usedRegisters.push_back(_instruction.rs);
		}
		it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), _instruction.rt);
		if (it == _registers.usedRegisters.end())
		{
			_registers.usedRegisters.push_back(_instruction.rt);
		}
		it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), _instruction.rd);
		if (it == _registers.usedRegisters.end())
		{
			_registers.usedRegisters.push_back(_instruction.rd);
		}

		// Sort the list of accessed registers in ascending order
		std::sort(_registers.usedRegisters.begin(), _registers.usedRegisters.end());
		//std::cout << _instruction.type << std::endl;
	}
	else
	{
		// Get TYPE
		_instruction.type = "ITYPE";
		// Get RS
		_instruction.rs = strtoul(_instruction.get_Instruction().substr(OPCODE_BIT_SIZE, RS_BIT_SIZE).c_str(), NULL, 2);
		// Get RT
		_instruction.rt = strtoul(_instruction.get_Instruction().substr(OPCODE_BIT_SIZE + RS_BIT_SIZE, RT_BIT_SIZE).c_str(), NULL, 2);
		// Get Immediate Address
		_instruction.immediate = (int16_t)strtoul(_instruction.get_Instruction().substr(OPCODE_BIT_SIZE + RS_BIT_SIZE + RT_BIT_SIZE, IMMEDIATE_BIT_SIZE).c_str(), NULL, 2);

		// Find out if new register has been modified, so add it
		std::vector<int>::iterator it;
		it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), _instruction.rs);
		if (it == _registers.usedRegisters.end())
		{
			_registers.usedRegisters.push_back(_instruction.rs);
		}
		it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), _instruction.rt);
		if (it == _registers.usedRegisters.end())
		{
			_registers.usedRegisters.push_back(_instruction.rt);
		}

		// Sort the list of accessed registers in ascending order
		std::sort(_registers.usedRegisters.begin(), _registers.usedRegisters.end());


		//std::cout << _instruction.type << std::endl;
	}
	get_Function(_instruction);
	_instruction.IF = true;
	std::cout << "\t I" << _instruction.instructionNum << "-IF";
	fprintf(_fp, "\t I%d -IF", _instruction.instructionNum);
	_instruction.state = "IF";
}




// Perform the decoding aspect
void performID(Instructions &_instruction, Register &_registers, FILE *_fp)
{
	_instruction.ID = true;
	std::cout << "\t I" << _instruction.instructionNum << "-ID";
	fprintf(_fp, "\t I%d -ID", _instruction.instructionNum);
	_instruction.state = "ID";
}



// Perform the Execution aspect
void performEX(Instructions &_instruction, Register &_registers, Mem &_memory, FILE *_fp)
{
	_instruction.EX = true;
	std::cout << "\t I" << _instruction.instructionNum << "-EX";
	fprintf(_fp, "\t I%d -EX", _instruction.instructionNum);


	if (_instruction.function == "ADD")
	{
		//std::cout << "\t\tPerform ADD" << std::endl;
		// Perform ADD function
 		_instruction.newRegisterValue = _registers.R[_instruction.rs] + _registers.R[_instruction.rt];
		_registers.R[_instruction.rd] = _instruction.newRegisterValue;
	}
	else if (_instruction.function == "ADDi")
	{
		//std::cout << "\t\tPerform ADDi" << std::endl;

		// Perform ADD immediate function
		_instruction.newRegisterValue = _registers.R[_instruction.rs] + _instruction.immediate;
		_registers.R[_instruction.rt] = _instruction.newRegisterValue;
	}
	else if (_instruction.function == "SUB")
	{
		//std::cout << "\t\terform SUB" << std::endl;

		// Perform SUB function
		_instruction.newRegisterValue = _registers.R[_instruction.rs] - _registers.R[_instruction.rt];
		_registers.R[_instruction.rd] = _instruction.newRegisterValue;
	}
	else if (_instruction.function == "SLT")
	{
		//std::cout << "\t\tPerform SLT" << std::endl;

		// Perform Shift if less than function
		if (_registers.R[_instruction.rs] < _registers.R[_instruction.rt])
		{
			_registers.R[_instruction.rd] = 1;
		}
		else
		{
			_registers.R[_instruction.rd] = 0;
		}
	}
	else if (_instruction.function == "BEQ")
	{
		//std::cout << "\t\tPerform BEQ" << std::endl;

		//std::cout <<"\t\tRS: " << _registers.R[_instruction.rs] << " " << "RT: " << _registers.R[_instruction.rt] << std::endl;

		// Perform Branch If Equal
		if (_registers.R[_instruction.rs] == _registers.R[_instruction.rt])
		{
			std::cout << "\t\t\t Do branch" << std::endl;
			 
			doBranch = true;	// skips to other instruction
		}

	}
	else if (_instruction.function == "BNE")
	{
		//std::cout << "\t\tPerform BNE" << std::endl;

		//std::cout << "\t\tRS: " << _registers.R[_instruction.rs] << " " << "RT: " << _registers.R[_instruction.rt] << std::endl;
		// Perform Branch If Not Equal
		if (_registers.R[_instruction.rs] != _registers.R[_instruction.rt])
		{
			//std::cout << "\t\t\t Do branch" << std::endl;
			doBranch = true;	// skips to new instruction
		}
	}
	else if (_instruction.function == "LW")
	{
		//std::cout << "\t\t Perform LW" << std::endl;

		// Perform Load Word function
		_instruction.newRegisterValue = _memory.M[_registers.R[_instruction.rs]/4];
		_registers.R[_instruction.rt] = _instruction.newRegisterValue;
		// Find out if new memory has been modified, so add it
		std::vector<int>::iterator it;
		it = find(_memory.usedMemory.begin(), _memory.usedMemory.end(), _registers.R[_instruction.rs] / 4);
		if (it == _memory.usedMemory.end())
		{
			_memory.usedMemory.push_back(_registers.R[_instruction.rs] / 4);
		}

		// Sort the list of accessed registers in ascending order
		std::sort(_memory.usedMemory.begin(), _memory.usedMemory.end());
	}
	else if (_instruction.function == "SW")
	{
		//std::cout << "\t\tPerform SW" << std::endl;

		// Perform Store Word
		_instruction.newRegisterValue = _registers.R[_instruction.rt];
		_memory.M[_registers.R[_instruction.rs]/4] = _instruction.newRegisterValue;
		// Find out if new memory has been modified, so add it
		std::vector<int>::iterator it;
		it = find(_memory.usedMemory.begin(), _memory.usedMemory.end(), _registers.R[_instruction.rs] / 4);
		if (it == _memory.usedMemory.end())
		{
			_memory.usedMemory.push_back(_registers.R[_instruction.rs] / 4);
		}

		// Sort the list of accessed registers in ascending order
		std::sort(_memory.usedMemory.begin(), _memory.usedMemory.end());

	}

	_instruction.state = "EX";
	_instruction.EX = true;	// completed execution
}

// Perform the Mem access aspect
void performMEM(Instructions &_instruction, FILE *_fp)
{
	_instruction.MEM = true;
	_instruction.state = "MEM";
	std::cout << "\t I" << _instruction.instructionNum << "-MEM";
	fprintf(_fp, "\t I%d -MEM", _instruction.instructionNum);

}

// Perform the Write Back aspect
void performWB(Instructions &_instruction, FILE *_fp)
{
	_instruction.WB = true;
	_instruction.state = "WB";
	std::cout << "\t I" << _instruction.instructionNum << "-WB";
	fprintf(_fp, "\t I%d -WB", _instruction.instructionNum);
	// Instruction is now done
	_instruction.done = true;
}
void Perform_Simulation(list_Instructions &_list, Register &_registers, Mem &_memory, const char* _outputFileName)
{
	// Open file for appending
	FILE *fp = fopen(_outputFileName, "a+");
	list_Instructions instructionsLeft = _list;
	IF_ID output_regIF_ID;
	ID_EX output_regID_EX;
	EX_MEM output_regEX_MEM;
	MEM_WB output_regMEM_WB;
	//Instructions *n = _list.top;	// Use Instructions *n = _list.tail; for pipeline
	// For every Instruction....
	int currPC = 0;
	int cycleNum = 1;
	while (!complete)
	{
		//Instructions *n = _list.top;	// Use Instructions *n = _list.tail; for pipeline
		Instructions *n = _list.top;
		std::cout << "Cycle: " << cycleNum << " ";
		fprintf(fp, "C# %d", cycleNum);
		while (n != NULL)	// use _list.tail->WB != false for pipeline
		{

			//std::cout << " Cycle #" << cycleNum << " ";

			//std::cout << "Instruction:" << cycleNum << std::endl;

			//performIF(*n);


			//performID(*n, _registers);
			//performEX(*n, _registers, _memory);
			//std::cout << doBranch << std::endl;

			// Separate test for the first instruction
			// Also check if it is done
			if (n->prev == NULL && n->WB == false && n->done == false)
			{
				// If it hasn't done fetch yet
				if (n->IF == false)
				{
					// Do the fetch
					performIF(*n, _registers, fp);

					// Update transition registers
					output_regIF_ID.setValues(*n);
					// PC increments after fetching
					currPC == n->PC;
				}
				// If it did
				else if (n->IF == true)
				{
					// Check if it needs to do the decode stage
					if (n->ID == false)
					{
						// If not, do it
						performID(*n, _registers, fp);

						// Update transition registers
						
						output_regID_EX.setWriteRegister(*n);
						//output_regIF_ID.reset();
						

					}
					// If done, go to next stage
					else if (n->ID == true)
					{
						// Check if EX stage is done
						if (n->EX == false)
						{
							// If not, do it
							performEX(*n, _registers, _memory, fp);

							// Update transition registers
							output_regEX_MEM.setWriteRegister(*n);
							//output_regIF_ID.reset();

							//output_regID_EX.reset();


							
						}
						// If done, go to next stage
						else if (n->EX == true)
						{
							// Check if MEM stage is done
							if (n->MEM == false)
							{
								// If not, do it
								performMEM(*n, fp);
								// Update transition registers
								output_regMEM_WB.setWriteRegister(*n);

								//output_regID_EX.reset();


							}
							// If done, go to next stage
							else if (n->MEM == true)
							{
								// Check if WB stage is done
								if (n->WB == false)
								{
									// If not, do it
									performWB(*n, fp);
									//output_regEX_MEM.reset();
									output_regMEM_WB.reset();


									//// Update transition registers
									//output_regMEM_WB.n = n;
									//output_regMEM_WB.setWriteRegister(*n);
								}
							}
						}
					}
				}

			}
			// Current instruction is not the first instruction
			// Used for subsequent instructions
			else if (n->prev != NULL && n->done != true)
			{
				// Check if current instruction did IF
				// And if previous instruction is in ID stage
				if (n->IF == true && n->prev->ID == true )
				{
					// Check for hazards
					if ((output_regIF_ID.rS == output_regEX_MEM.rD || 
						output_regIF_ID.rS == output_regMEM_WB.rD ||
						output_regIF_ID.rT == output_regEX_MEM.rD ||
						output_regIF_ID.rT == output_regMEM_WB.rD) &&
						n->prev->WB == false &&
						(n->regWrite == true || n->memWrite == true))
					{
						std::cout << "\t I" << n->instructionNum << "-STALL";
						fprintf(fp, "\t I%d -STALL", n->instructionNum);
						//n->state = "STALL";
						n = n->next;
						continue;
					}

					else if (n->MEM == true && n->prev->WB == true)
					{
						performWB(*n, fp);

						//// Update transition registers
						//output_regMEM_WB.n = n;
						//output_regMEM_WB.setWriteRegister(*n);
					}
					// Check if conditions to perform MEM were passed
					else if (n->EX == true && (n->prev->WB == true || n->prev->MEM == true))
					{
						performMEM(*n, fp);
						// Update transition registers
						output_regMEM_WB.n = n;
						output_regMEM_WB.setWriteRegister(*n);
					}
					// Check if conditions to perform EX were passed
					else if((n->ID == true && (n->prev->WB == true || n->prev->MEM == true)))
					{
						performEX(*n, _registers, _memory, fp);

						// Update transition registers
						output_regEX_MEM.setWriteRegister(*n);

						// At EX, if branching is needed
						if (doBranch)
						{
							Instructions *skipTo = _list.top;

							// Target = current + 4*immediate
							int targetPC = ((n->immediate) * 4) + currPC;

							// Go through every
							while (skipTo != NULL)
							{
								//std::cout << "\t CurrentPC " << currPC << std::endl;
								//std::cout << "\t Need to branch to " << targetPC << std::endl;

								// If current instruction PC is equal to target PC
								if (skipTo->PC == targetPC)
								{
									// Make current Instruction point to the target Instruction
									n->next = skipTo;
									skipTo->prev = n;
									break;
								}
								// Else, check next instruction
								skipTo = skipTo->next;
							}
							/*doBranch = false;*/
							
						}

					}

					// Can now perform ID if stalls are done
					else if (((output_regIF_ID.rS == output_regEX_MEM.rD ||
							output_regIF_ID.rS == output_regMEM_WB.rD ||
							output_regIF_ID.rT == output_regEX_MEM.rD ||
							output_regIF_ID.rT == output_regMEM_WB.rD) &&
							(n->prev->WB == true ||
							n->prev->EX == true) && n->IF == true))
					{
						performID(*n, _registers, fp);
						// Update transition registers
						output_regID_EX.setWriteRegister(*n);
					}
					// Condition if source values are different from the future destination values
					else if ((output_regIF_ID.rS != output_regEX_MEM.rD ||
						output_regIF_ID.rS != output_regMEM_WB.rD ||
						output_regIF_ID.rT != output_regEX_MEM.rD ||
						output_regIF_ID.rT != output_regMEM_WB.rD) &&
						(n->prev->WB == true ||
							n->prev->EX == true) && n->IF == true)
					{
						performID(*n, _registers, fp);
						// Update transition registers
						output_regID_EX.setWriteRegister(*n);
					}
					
				}
				else if (n->IF == false && n->prev->ID == true)
				{
					performIF(*n, _registers, fp);
					currPC = n->PC;
					output_regIF_ID.setValues(*n);
					
				}
			}	
			
			if (doBranch)
			{
				doBranch = false;
				break;
			}
			

			
			//performMEM(*n);
			//performWB(*n);


			
			n = n->next;
		}
		
		if (_list.tail->done)
		{
			std::cout << std::endl;
			fprintf(fp, "\n\r", "");
			std::vector<int>::iterator it;
			it = find(_registers.usedRegisters.begin(), _registers.usedRegisters.end(), 0);

			if (it != _registers.usedRegisters.end())
			{
				_registers.usedRegisters.erase(it, it + 1);
			}

			fprintf(fp, "REGISTERS\n\r", "");

			for (int i = 0; i < _registers.usedRegisters.size(); i++)
			{
				fprintf(fp, "R%d %d\n\r", _registers.usedRegisters[i], _registers.R[_registers.usedRegisters[i]]);
			}

			fprintf(fp, "MEMORY\n", "");
			for (int i = 0; i < _memory.usedMemory.size(); i++)
			{
				fprintf(fp, "%d %d\n\r", _memory.usedMemory[i] * 4, _memory.M[_memory.usedMemory[i]]);
			}

			fclose(fp);
			return;
		}
		std::cout << std::endl;
		fprintf(fp, "\n\r", "");
		cycleNum++;
	}
	
}

int main()
{
	
	Register registers;
	Mem memory;
	std::vector<std::string> instructions;
	//std::vector<Instructions> list_instructions;
	list_Instructions List_Instructions;
	std::vector<int> usedRegisters;
	std::string inputFileName;
	std::string outputFileName;
	std::vector<int>::iterator it;
	std::string cont;

	do
	{
		std::cout << "Please enter the name of your input file" << std::endl;
		std::cin >> inputFileName;

		std::cout << "Please enter the name of your output file" << std::endl;
		std::cin >> outputFileName;

		// Get all necessary register, memory, instruction values
		initFile(inputFileName.c_str(), &memory, &registers, List_Instructions);


		Perform_Simulation(List_Instructions, registers, memory, outputFileName.c_str());
		//std::cout << "Double linked list version " << std::endl;

		//List_Instructions.display_List();

		//std::cout << List_Instructions.Get_Size() << std::endl;
		it = find(registers.usedRegisters.begin(), registers.usedRegisters.end(), 0);

		if (it != registers.usedRegisters.end())
		{
			registers.usedRegisters.erase(it, it + 1);
		}

		std::cout << "REGISTERS" << std::endl;

		for (int i = 0; i < registers.usedRegisters.size(); i++)
		{
			std::cout << "R" << registers.usedRegisters[i] << " " << registers.R[registers.usedRegisters[i]] << std::endl;
		}

		std::cout << "MEMORY" << std::endl;
		for (int i = 0; i < memory.usedMemory.size(); i++)
		{
			std::cout << memory.usedMemory[i] * 4 << " " << memory.M[memory.usedMemory[i]] << std::endl;
		}

		std::cin >> cont;
		std::cin >> cont;
	} while (cont == "y" || cont == "Y");	// If y is pressed

}

