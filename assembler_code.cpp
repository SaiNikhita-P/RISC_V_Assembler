
#include <iostream>
#include <vector>
#include <fstream>     
#include <sstream> 
#include <map>
#include <string>
#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

using namespace std;

class Register {
private:
    unordered_map<string, int> reg_map;

public:
    Register() {
        for (int i = 0; i <= 31; i++) {
            reg_map["x" + to_string(i)] = i;
        }
    }

    int getRegNumber(const string &reg) {
        if (reg_map.find(reg) != reg_map.end()) {
            return reg_map[reg];
        } else {
            cerr << "Error: Register " << reg << " not found." << endl;
            return -1;
        }
    }
};

class Instruction {
public:
    virtual string assemble(const vector<string> &tokens, Register &reg) = 0;
    virtual ~Instruction() {}
};


class RTypeInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 4) return "Invalid Format";

        int rd = reg.getRegNumber(tokens[1]);
        int rs1 = reg.getRegNumber(tokens[2]);
        int rs2 = reg.getRegNumber(tokens[3]);
        if (rd == -1 || rs1 == -1 || rs2 == -1) return "Invalid Register";

        string funct7, funct3, opcode = "0110011";

        if (tokens[0] == "ADD") {
            funct7 = "0000000";
            funct3 = "000";
        } else if (tokens[0] == "SUB") {
            funct7 = "0100000";
            funct3 = "000";
        } else if (tokens[0] == "AND") {
            funct7 = "0000000";
            funct3 = "111";
        } else if (tokens[0] == "OR") {
            funct7 = "0000000";
            funct3 = "110";
        } else {
            return "Unsupported R-type Instruction";
        }

        string rs2_bin = bitset<5>(rs2).to_string();
        string rs1_bin = bitset<5>(rs1).to_string();
        string rd_bin = bitset<5>(rd).to_string();

        return funct7 + rs2_bin + rs1_bin + funct3 + rd_bin + opcode;
    }
};

class ITypeInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 4) return "Invalid Format";

        int rd = reg.getRegNumber(tokens[1]);
        int rs1 = reg.getRegNumber(tokens[2]);
        int imm;

        if (rd == -1 || rs1 == -1) return "Invalid Register";
        try {
            imm = stoi(tokens[3]);
        } catch (const invalid_argument &) {
            return "Invalid Immediate";
        }

        string imm_bin = bitset<12>(imm).to_string();
        string rs1_bin = bitset<5>(rs1).to_string();
        string rd_bin = bitset<5>(rd).to_string();
        string funct3;

        if (tokens[0] == "ADDI") {
            funct3 = "000";
        } else if (tokens[0] == "ANDI") {
            funct3 = "111";
        }
     else if (tokens[0] == "SRLI") {
            if (imm < 0 || imm > 31) return "Invalid Shift Amount"; // Valid range for shift is 0-31
            
            string shamt_bin = bitset<5>(imm).to_string(); // Immediate for SRLI is 5 bits
            string rs1_bin = bitset<5>(rs1).to_string();
            string rd_bin = bitset<5>(rd).to_string();
            string funct3 = "101"; // Funct3 for SRLI
            string opcode = "0010011"; // Opcode for I-type

            // Constructing the final binary string
            return "0000000" + shamt_bin + rs1_bin + funct3 + rd_bin + opcode; // 7 bits of 0 for immediate[31-25]
        }


         else {
            return "Unsupported I-type Instruction";
        }

        return imm_bin + rs1_bin + funct3 + rd_bin + "0010011"; // opcode for I-type
    }
};


class LoadInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 4) return "Invalid Format";

        int rd = reg.getRegNumber(tokens[1]);
        int rs1 = reg.getRegNumber(tokens[2]);
        int imm;

        if (rd == -1 || rs1 == -1) return "Invalid Register";
        try {
            imm = stoi(tokens[3]);
        } catch (const invalid_argument &) {
            return "Invalid Immediate";
        }

        // Check for valid immediate range if necessary
        if (imm < -2048 || imm > 2047) return "Immediate out of range"; // 12-bit signed immediate

        string imm_bin = bitset<12>(imm).to_string();
        string rd_bin = bitset<5>(rd).to_string();
        string rs1_bin = bitset<5>(rs1).to_string();
        string funct3;

        // Determine funct3 based on the instruction type
        if (tokens[0] == "LW") {
            funct3 = "010";
        } else if (tokens[0] == "LB") {
            funct3 = "000";
        } else if (tokens[0] == "LH") {
            funct3 = "001";
        } else if (tokens[0] == "LBU") {
            funct3 = "100";
        } else if (tokens[0] == "LHU") {
            funct3 = "101";
        } else {
            return "Unsupported Load Instruction";
        }

        // Construct the final binary string according to the specified format
        return imm_bin + rs1_bin + funct3 + rd_bin + "0000011"; // Load opcode
    }
};




class StoreInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 4) return "Invalid Format";

        int rs2 = reg.getRegNumber(tokens[1]); // Source register (data to store)
        int rs1 = reg.getRegNumber(tokens[2]); // Base register (address)
        int imm;

        if (rs2 == -1 || rs1 == -1) return "Invalid Register";
        try {
            imm = stoi(tokens[3]);
        } catch (const invalid_argument &) {
            return "Invalid Immediate";
        }

        // Check for valid immediate range if necessary
        if (imm < -2048 || imm > 2047) return "Immediate out of range"; // 12-bit signed immediate

        string imm_bin = bitset<12>(imm).to_string();
        string rs2_bin = bitset<5>(rs2).to_string();
        string rs1_bin = bitset<5>(rs1).to_string();
        string funct3 = "010"; // For SW
        string opcode = "0100011"; // Store opcode

        // Construct the final binary string according to the specified format
        return imm_bin.substr(0, 7) + rs2_bin + rs1_bin + funct3 + imm_bin.substr(7, 5) + opcode;
    }
};



class BTypeInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 4) return "Invalid Format";

        int rs1 = reg.getRegNumber(tokens[1]);
        int rs2 = reg.getRegNumber(tokens[2]);
        int imm;

        if (rs1 == -1 || rs2 == -1) return "Invalid Register";
        try {
            imm = stoi(tokens[3]);
        } catch (const invalid_argument &) {
            return "Invalid Immediate";
        }

        string imm_bin = bitset<12>(imm).to_string();
        // reverse(imm_bin.begin(),imm_bin.end());
        string rs1_bin = bitset<5>(rs1).to_string();
        string rs2_bin = bitset<5>(rs2).to_string();
        string funct3;

        if (tokens[0] == "BEQ") {
            funct3 = "000";
        } else if (tokens[0] == "BNE") {
            funct3 = "001";
        } else if (tokens[0] == "BLT") {
            funct3 = "100";
        } else if (tokens[0] == "BGE") {
            funct3 = "101";
        } else {
            return "Unsupported B-type Instruction";
        }


         return string(1,imm_bin[0]) + imm_bin.substr(2, 6) + rs2_bin+rs1_bin + funct3 + string(1,imm_bin[1]) + imm_bin.substr(8, 4) + "1100011";
    }
};

// class BTypeInstruction : public Instruction {
// public:
//     string assemble(const vector<string> &tokens, Register &reg) override {
//         if (tokens.size() != 4) return "Invalid Format";

//         int rs1 = reg.getRegNumber(tokens[1]);
//         int rs2 = reg.getRegNumber(tokens[2]);
        

//         if (rs1 == -1 || rs2 == -1) return "Invalid Register";

//         // Convert label to immediate value
//         int offset=stoi(tokens[3]);
//         if (offset == -1) return "Label not found"; // Replace this with your label lookup logic

//         string imm_bin = bitset<12>(offset).to_string();
//         string rs1_bin = bitset<5>(rs1).to_string();
//         string rs2_bin = bitset<5>(rs2).to_string();
//         string funct3;

//         if (tokens[0] == "BEQ") {
//             funct3 = "000";
//         } else if (tokens[0] == "BNE") {
//             funct3 = "001";
//         } else if (tokens[0] == "BLT") {
//             funct3 = "100";
//         } else if (tokens[0] == "BGE") {
//             funct3 = "101";
//         } else {
//             return "Unsupported B-type Instruction";
//         }

//         // Construct the machine code
//         return string(1, imm_bin[11]) + imm_bin.substr(5, 5) + rs2_bin + funct3 + 
//                string(1, imm_bin[10]) + imm_bin.substr(1, 5) + "1100011"; // B-type opcode
//     }
// };


// class JTypeInstruction : public Instruction {
// public:
//     string assemble(const vector<string> &tokens, Register &reg) override {
//         if (tokens.size() != 3) return "Invalid Format";

//         int rd = reg.getRegNumber(tokens[1]);
//         int imm;

//         if (rd == -1) return "Invalid Register";
//         try {
//             imm = stoi(tokens[2]);
//         } catch (const invalid_argument &) {
//             return "Invalid Immediate";
//         }

//         string imm_bin = bitset<20>(imm).to_string();
//         string rd_bin = bitset<5>(rd).to_string();
//         string opcode = "1101111";

//         return string(1,imm_bin[0]) + imm_bin.substr(10, 10) + string(1,imm_bin[9])+imm_bin.substr(1,8) + rd_bin + opcode;
//     }
// };
class JTypeInstruction : public Instruction {
public:
    string assemble(const vector<string> &tokens, Register &reg) override {
        if (tokens.size() != 3) return "Invalid Format";

        int rd = reg.getRegNumber(tokens[1]);
        int imm;

        if (rd == -1) return "Invalid Register";
        try {
            imm = stoi(tokens[2]);
            imm=imm/2;
        } catch (const invalid_argument &) {
            return "Invalid Immediate";
        }

        // Ensure imm is in the valid range for a 20-bit immediate
        if (imm < -1048576 || imm > 1048575) return "Immediate out of range"; // 20-bit signed immediate range

        string imm_bin = bitset<20>(imm).to_string();
        string rd_bin = bitset<5>(rd).to_string();
        string opcode = "1101111";

        // Construct the final binary string
        return string(1, imm_bin[0]) + imm_bin.substr(10, 10) + string(1, imm_bin[9]) + imm_bin.substr(1, 8) + rd_bin + opcode;
    }
};




class Assembler {
private:
    unordered_map<string, Instruction *> instructionSet;
    Register reg;

public:
    Assembler() {
        instructionSet["ADDI"] = new ITypeInstruction();
        instructionSet["ANDI"] = new ITypeInstruction();
        instructionSet["SRLI"] = new ITypeInstruction(); // Added SRLI
        instructionSet["SUB"] = new RTypeInstruction();
        instructionSet["ADD"] = new RTypeInstruction();
         instructionSet["AND"] = new RTypeInstruction();
          instructionSet["OR"] = new RTypeInstruction();
        // instructionSet["SW"] = new STypeInstruction();
        //  instructionSet["LW"] = new LoadInstruction(); 
          instructionSet["LB"] = new LoadInstruction();
        instructionSet["LH"] = new LoadInstruction();
        instructionSet["LW"] = new LoadInstruction();
        instructionSet["LBU"] = new LoadInstruction();
        instructionSet["LHU"] = new LoadInstruction();
        instructionSet["SB"] = new StoreInstruction();
        instructionSet["SH"] = new StoreInstruction();
        instructionSet["SW"] = new StoreInstruction();
        instructionSet["BGE"] = new BTypeInstruction(); // Added BGE
        instructionSet["JAL"] = new JTypeInstruction();
    }

    ~Assembler() {
        for (auto &pair : instructionSet) {
            delete pair.second;
        }
    }

    string encodeInstruction(const string &line) {
        vector<string> tokens = tokenize(line);
        if (tokens.empty()) return "";
        string opcode = tokens[0];
        if (instructionSet.find(opcode) != instructionSet.end()) {
            return instructionSet[opcode]->assemble(tokens, reg);
        } else {
            return "Unknown Instruction";
        }
    }

    vector<string> tokenize(const string &line) {
        vector<string> tokens;
        stringstream ss(line);
        string token;

        while (ss >> token) {
            token.erase(remove(token.begin(), token.end(), ','), token.end());
            tokens.push_back(token);
        }
        return tokens;
    }
};

int main() {
    Assembler assembler;
    ifstream infile("instructions.txt");

    if (!infile) {
        cerr << "Error opening file" << endl;
        return 1;
    }

    string line;
    while (getline(infile, line)) {
        string encoded = assembler.encodeInstruction(line);
        if (!encoded.empty()) {
            cout << "Instruction: " << line << " -> Encoded Instruction: " << encoded << endl;
        }
    }
    infile.close();
    return 0;
}

