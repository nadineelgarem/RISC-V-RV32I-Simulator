#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype> // For isdigit

using namespace std;

// Function prototypes
string toBinary(int num, int bits);
string assembleInstruction(const string& instruction);
void loadInstructions(const vector<string>& program, int startAddress);
void loadData(const unordered_map<int, int>& data);
void executeInstruction(const string& instruction);
void runSimulation();
void displayMemory();
void displayState();
void displayProgramCounter();
vector<string> loadProgramFromFile(const string& filename);
unordered_map<int, int> loadDataFromFile(const string& filename);
void runTestCase(const string& programFile, const string& dataFile);

string toBinary(int num, int bits) {
    string binary;
    for (int i = bits - 1; i >= 0; --i) {
        binary += ((num >> i) & 1) ? '1' : '0';
    }
    return binary;
}

string toBinaryString(int num, int bits = 32) {
    return toBinary(num, bits);
}

string toHexString(int num) {
    stringstream ss;
    ss << hex << uppercase << num;
    return ss.str();
}

int registers[32] = {0};
unordered_map<int, string> memory;
unordered_map<int, string> assemblyMemory;
int pc = 0;

vector<string> loadProgramFromFile(const string& filename) {
    vector<string> program;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        program.push_back(line);
    }
    return program;
}

unordered_map<int, int> loadDataFromFile(const string& filename) {
    unordered_map<int, int> data;
    ifstream file(filename);
    int address, value;
    while (file >> address >> value) {
        data[address] = value;
    }
    return data;
}

string assembleInstruction(const string& instruction) {
    stringstream ss(instruction);
    string opcode, machineCode;
    int rd, rs1, imm;
    ss >> opcode;
    if (opcode == "ADDI") {
        ss >> rd >> rs1 >> imm;
        machineCode = toBinary(imm, 12) + toBinary(rs1, 5) + "000" + toBinary(rd, 5) + "0010011";
    }
    return machineCode;
}

void loadInstructions(const vector<string>& program, int startAddress) {
    pc = startAddress;
    for (int i = 0; i < program.size(); ++i) {
        string machineCode = assembleInstruction(program[i]);
        memory[startAddress + i * 4] = machineCode;
        assemblyMemory[startAddress + i * 4] = program[i];
    }
}

void loadData(const unordered_map<int, int>& data) {
    for (const auto& entry : data) {
        memory[entry.first] = to_string(entry.second);
    }
}

void executeInstruction(const string& instruction) {
    stringstream ss(instruction);
    string opcode;
    ss >> opcode;

    int rd, rs1, rs2, imm;
    if (opcode == "ADD") {
        ss >> rd >> rs1 >> rs2;
        registers[rd] = registers[rs1] + registers[rs2];
    } else if (opcode == "SUB") {
        ss >> rd >> rs1 >> rs2;
        registers[rd] = registers[rs1] - registers[rs2];
    } else if (opcode == "AND") {
        ss >> rd >> rs1 >> rs2;
        registers[rd] = registers[rs1] & registers[rs2];
    } else if (opcode == "OR") {
        ss >> rd >> rs1 >> rs2;
        registers[rd] = registers[rs1] | registers[rs2];
    } else if (opcode == "ADDI") {
        ss >> rd >> rs1 >> imm;
        registers[rd] = registers[rs1] + imm;
    } else if (opcode == "LW") {
        ss >> rd >> imm >> rs1;
        int address = registers[rs1] + imm;
        if (memory.find(address) != memory.end() && !memory[address].empty()) {
            try {
                registers[rd] = stoi(memory[address]);
            } catch (const out_of_range& e) {
                cout << "Error: Value at memory address " << address << " is out of range." << endl;
                registers[rd] = 0;
            } catch (const invalid_argument& e) {
                cout << "Error: Invalid value at memory address " << address << " for LW operation." << endl;
                registers[rd] = 0;
            }
        } else {
            cout << "Warning: Attempted to load from an invalid or empty memory address " << address << endl;
            registers[rd] = 0;
        }
    } else if (opcode == "SW") {
        ss >> rs2 >> imm >> rs1;
        int address = registers[rs1] + imm;
        memory[address] = to_string(registers[rs2]);
    } else if (opcode == "BEQ") {
        ss >> rs1 >> rs2 >> imm;
        if (registers[rs1] == registers[rs2]) {
            pc += imm;
            return;
        }
    } else if (opcode == "BNE") {
        ss >> rs1 >> rs2 >> imm;
        if (registers[rs1] != registers[rs2]) {
            pc += imm;
            return;
        }
    } else if (opcode == "JAL") {
        ss >> rd >> imm;
        registers[rd] = pc + 4;
        pc += imm;
        return;
    }

    registers[0] = 0;
}

void runSimulation() {
    while (pc != -1 && assemblyMemory.find(pc) != assemblyMemory.end()) {
        string instruction = assemblyMemory[pc];
        cout << "Executing at PC=" << pc << ": " << instruction << endl;
        executeInstruction(instruction);
        displayProgramCounter();
        pc += 4;
    }
}

bool isNumeric(const string& str) {
    for (char const &c : str) {
        if (!isdigit(c) && c != '-') return false;
    }
    return true;
}

// Function to display memory contents
void displayMemory() {
    cout << "\nMemory Contents (Decimal | Binary | Hexadecimal):\n";
    for (const auto& entry : memory) {
        // Check if the entry is machine code (not a numeric value)
        if (entry.second.find_first_not_of("01") == string::npos) {
            cout << "Address " << entry.first << ": " 
                 << entry.second << " (Machine code)" << endl;
        } else {  // Handle numeric values
            try {
                int value = stoi(entry.second);
                cout << "Address " << entry.first << ": " 
                     << value << " | " 
                     << toBinaryString(value) << " | 0x" 
                     << toHexString(value) << endl;
            } catch (const out_of_range& e) {
                cout << "Address " << entry.first << ": " 
                     << entry.second << " (Out of range)" << endl;
            }
        }
    }
}


void displayState() {
    cout << "\nRegister Values (Decimal | Binary | Hexadecimal):" << endl;
    for (int i = 0; i < 32; ++i) {
        cout << "x" << i << ": " 
             << registers[i] << " | " 
             << toBinaryString(registers[i]) << " | 0x" 
             << toHexString(registers[i]) << endl;
    }
}

void displayProgramCounter() {
    cout << "PC: " << pc 
         << " | " << toBinaryString(pc) 
         << " | 0x" << toHexString(pc) << endl;
}

void runTestCase(const string& programFile, const string& dataFile) {
    fill(begin(registers), end(registers), 0);
    memory.clear();
    assemblyMemory.clear();
    pc = 0;

    vector<string> program = loadProgramFromFile(programFile);
    unordered_map<int, int> data = loadDataFromFile(dataFile);

    loadInstructions(program, 0x0000);
    loadData(data);

    runSimulation();
    displayState();
}

int main() {
    cout << "Running Test Case 1: Basic Arithmetic and Logical Operations" << endl;
    runTestCase("../tests/programs/test_case_1.txt", "../tests/data/test_case_1_data.txt");

    cout << "\nRunning Test Case 2: Immediate Operations" << endl;
    runTestCase("../tests/programs/test_case_2.txt", "../tests/data/test_case_2_data.txt");

    cout << "\nRunning Test Case 3: Load and Store Operations" << endl;
    runTestCase("../tests/programs/test_case_3.txt", "../tests/data/test_case_3_data.txt");

    cout << "\nRunning Test Case 4: Branch Instructions" << endl;
    runTestCase("../tests/programs/test_case_4.txt", "../tests/data/test_case_4_data.txt");

    cout << "\nRunning Test Case 5: Jump Instructions" << endl;
    runTestCase("../tests/programs/test_case_5.txt", "../tests/data/test_case_5_data.txt");

    cout << "\nTesting Assembler with Sample Program:" << endl;
    vector<string> program;
    program.push_back("ADDI 1 0 10");
    program.push_back("ADDI 2 1 5");

    loadInstructions(program, 0x0000);
    displayMemory();

    return 0;
}
