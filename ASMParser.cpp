#include "ASMParser.h"

ASMParser::ASMParser(string filename)
// Specify a text file containing MIPS assembly instructions. Function
// checks syntactic correctness of file and creates a list of Instructions.
{

  // Init instruction obj
  Instruction i;
  myFormatCorrect = true;

  // Starting address
  myLabelAddress = 0x400000;

  // Try to load in file, end if can't
  ifstream in;
  in.open(filename.c_str());
  if (in.bad())
  {
    myFormatCorrect = false;
  }
  else
  {

    vector<string> lines = preProcessFile(in);

    // Init string for line
    string line;

    // Keep getting lines until file ends
    // while (getline(in, line))
    for (string line : lines)
    {
      string opcode("");
      string operand[80];
      int operand_count = 0;

      // Set opcode, operand, and num operands for the line
      getTokens(line, opcode, operand, operand_count);

      // Check that opcode exists / is supposed to exist
      if (opcode.length() == 0 && operand_count != 0)
      {
        // No opcode but operands
        myFormatCorrect = false;
        break;
      }

      // Match parsed opcode to enum
      Opcode o = opcodes.getOpcode(opcode);
      if (o == UNDEFINED)
      {
        // invalid opcode specified
        myFormatCorrect = false;
        break;
      }

      //
      bool success = getOperands(i, o, operand, operand_count);
      if (!success)
      {
        myFormatCorrect = false;
        break;
      }

      string encoding = encode(i);
      i.setEncoding(encoding);

      myInstructions.push_back(i);
    }
  }

  myIndex = 0;
}

vector<string> ASMParser::preProcessFile(ifstream &in)
{

  // Init string for line
  string line;
  vector<string> lines;

  while (getline(in, line))
  {

    // Logic if a label is found
    int colonPos = line.find(":");
    if (colonPos > -1)
    {

      // Parse label
      string label = line.substr(0, colonPos);

      // Store label with addr
      labelsMap.insert({label, myLabelAddress});

      // Remove label from line
      line = line.substr(colonPos + 1, line.length());
    }

    myLabelAddress += 4;
    lines.push_back(line);
  }

  // reset to starting addr
  myLabelAddress = 0x400000;
  return lines;
}

Instruction ASMParser::getNextInstruction()
// Iterator that returns the next Instruction in the list of Instructions.
{
  if (myIndex < (int)(myInstructions.size()))
  {
    myIndex++;
    return myInstructions[myIndex - 1];
  }

  Instruction i;
  return i;
}

void ASMParser::getTokens(string line,
                          string &opcode,
                          string *operand,
                          int &numOperands)
// Decomposes a line of assembly code into strings for the opcode field and operands,
// checking for syntax errors and counting the number of operands.
{
  // locate the start of a comment
  string::size_type idx = line.find('#');
  if (idx != string::npos) // found a ';'
    line = line.substr(0, idx);
  int len = line.length();
  opcode = "";
  numOperands = 0;

  if (len == 0)
    return;
  int p = 0; // position in line

  // line.at(p) is whitespace or p >= len
  while (p < len && isWhitespace(line.at(p)))
    p++;
  // opcode starts
  while (p < len && !isWhitespace(line.at(p)))
  {
    opcode = opcode + line.at(p);
    p++;
  }
  //    for(int i = 0; i < 3; i++){
  int i = 0;
  while (p < len)
  {
    while (p < len && isWhitespace(line.at(p)))
      p++;

    // operand may start
    bool flag = false;
    while (p < len && !isWhitespace(line.at(p)))
    {
      if (line.at(p) != ',')
      {
        operand[i] = operand[i] + line.at(p);
        flag = true;
        p++;
      }
      else
      {
        p++;
        break;
      }
    }
    if (flag == true)
    {
      numOperands++;
    }
    i++;
  }

  idx = operand[numOperands - 1].find('(');
  string::size_type idx2 = operand[numOperands - 1].find(')');

  if (idx == string::npos || idx2 == string::npos ||
      ((idx2 - idx) < 2))
  { // no () found
  }
  else
  { // split string
    string offset = operand[numOperands - 1].substr(0, idx);
    string regStr = operand[numOperands - 1].substr(idx + 1, idx2 - idx - 1);

    operand[numOperands - 1] = offset;
    operand[numOperands] = regStr;
    numOperands++;
  }

  // ignore anything after the whitespace after the operand
  // We could do a further look and generate an error message
  // but we'll save that for later.
  return;
}

bool ASMParser::isNumberString(string s)
// Returns true if s represents a valid decimal integer
{
  int len = s.length();
  if (len == 0)
    return false;
  if ((isSign(s.at(0)) && len > 1) || isDigit(s.at(0)))
  {
    // check remaining characters
    for (int i = 1; i < len; i++)
    {
      if (!isdigit(s.at(i)))
        return false;
    }
    return true;
  }
  return false;
}

int ASMParser::cvtNumString2Number(string s)
// Converts a string to an integer.  Assumes s is something like "-231" and produces -231
{
  if (!isNumberString(s))
  {
    cerr << "Non-numberic string passed to cvtNumString2Number"
         << endl;
    return 0;
  }
  int k = 1;
  int val = 0;
  for (int i = s.length() - 1; i > 0; i--)
  {
    char c = s.at(i);
    val = val + k * ((int)(c - '0'));
    k = k * 10;
  }
  if (isSign(s.at(0)))
  {
    if (s.at(0) == '-')
      val = -1 * val;
  }
  else
  {
    val = val + k * ((int)(s.at(0) - '0'));
  }
  return val;
}

bool ASMParser::getOperands(Instruction &i, Opcode o,
                            string *operand, int operand_count)

// Given an Opcode, a string representing the operands, and the number of operands,
// breaks operands apart and stores fields into Instruction.
{

  if (operand_count != opcodes.numOperands(o))
    return false;

  int rs, rt, rd, imm;
  imm = 0;
  rs = rt = rd = NumRegisters;

  int rs_p = opcodes.RSposition(o);
  int rt_p = opcodes.RTposition(o);
  int rd_p = opcodes.RDposition(o);
  int imm_p = opcodes.IMMposition(o);

  if (rs_p != -1)
  {
    rs = registers.getNum(operand[rs_p]);
    if (rs == NumRegisters)
      return false;
  }

  if (rt_p != -1)
  {
    rt = registers.getNum(operand[rt_p]);
    if (rt == NumRegisters)
      return false;
  }

  if (rd_p != -1)
  {
    rd = registers.getNum(operand[rd_p]);
    if (rd == NumRegisters)
      return false;
  }

  if (imm_p != -1)
  {
    if (isNumberString(operand[imm_p]))
    {
      // does it have a numeric immediate field?
      imm = cvtNumString2Number(operand[imm_p]);
      if (((abs(imm) & 0xFFFF0000) << 1)) // too big a number to fit
        return false;
    }
    else
    {
      if (opcodes.isIMMLabel(o))
      { // Can the operand be a label?
        // Assign the immediate field an address
        // imm = myLabelAddress;
        // myLabelAddress += 4; // increment the label generator

        string label = operand[imm_p];

        // Check if label in map
        if (labelsMap.find(label) == labelsMap.end())
        {

          // Check if label is hex
          if (label.substr(0, 2) == "0x")
          {
            // Convert from hex to integer
            imm = strtoul(label.c_str(), 0, 16);
          }
          else
          {
            return false;
          }
        }
        else
        {
          imm = labelsMap.at(label);
        }
      }
      else // There is an error
        return false;
    }
  }

  i.setValues(o, rs, rt, rd, imm);

  return true;
}

string ASMParser::encode(Instruction i)
// Given a valid instruction, returns a string representing the 32 bit MIPS binary encoding
// of that instruction.
{
  // Your code here

  string encoded_str = "";

  // Gets opcode from Instruction - should be binary string
  Opcode op = i.getOpcode();

  // cout << i.getImmediate() << endl;

  if (opcodes.getInstType(op) == RTYPE)
  {
    encoded_str = this->encodeRType(i);
  }

  if (opcodes.getInstType(op) == JTYPE)
  {
    encoded_str = this->encodeJType(i);
  }

  if (opcodes.getInstType(op) == ITYPE)
  {
    encoded_str = this->encodeIType(i);
  }

  return encoded_str;
}

// Implement method to convert from int to binary
string ASMParser::intToBinaryString(int n, int length)
{

  int bin_arr[40] = {0};
  int a = 1, i;
  string str = "";

  for (i = 0; n > 0; i++)
  {
    bin_arr[i] = n % 2;
    n = n / 2;
  }

  for (i = length - 1; i >= 0; i--)
  {
    str = str + std::to_string(bin_arr[i]);
  }

  return str;
}

string ASMParser::encodeIType(Instruction i)
{

  string str = "";

  string opcode_field = opcodes.getOpcodeField(i.getOpcode());
  string rs = this->intToBinaryString(i.getRS(), 5);
  string rt = this->intToBinaryString(i.getRT(), 5);
  // string rd = this->intToBinaryString(i.getRD(), 5);
  string immediate = this->intToBinaryString(i.getImmediate(), 16);

  // cout << opcode_field << endl;
  // cout << rs << endl;
  // cout << rt << endl;
  // cout << immediate << endl;

  // Format: opcode $rs $rt immediate
  str += opcode_field + rs + rt + immediate;

  return str;
}

string ASMParser::encodeJType(Instruction i)
{
  // cout << i.getString() << endl;
  // cout << myLabelAddress << endl;
  string str = "";

  // Get immediate, which is the 32 bit addr of label or hex
  int imm = i.getImmediate();
  string imm_bin = intToBinaryString(imm, 32);

  // Remove first 4 bits (PC) and last 2 (2 bit shift)
  imm_bin = imm_bin.substr(4, 26);

  string opcode_field = opcodes.getOpcodeField(i.getOpcode());

  str = opcode_field + imm_bin;

  return str;
}

string ASMParser::encodeRType(Instruction i)
{

  string str = "";

  string opcode_field = opcodes.getOpcodeField(i.getOpcode());
  string func_field = opcodes.getFunctField(i.getOpcode());
  string rs = this->intToBinaryString(i.getRS(), 5);
  string rt = this->intToBinaryString(i.getRT(), 5);
  string rd = this->intToBinaryString(i.getRD(), 5);

  // In this case, the immediate == shamt
  string immediate = this->intToBinaryString(i.getImmediate(), 5);

  // Format: opcode $rs $rt $rd shamt func
  str = opcode_field + rs + rt + rd + immediate + func_field;

  return str;
}
