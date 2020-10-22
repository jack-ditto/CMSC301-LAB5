OBJS = ASM.o ASMParser.o Instruction.o Opcode.o RegisterTable.o
HEAD = ASMParser.h Instruction.h Opcode.h RegisterTable.h

CC = g++
CCFLAGS = -std=c++11 -Wall

main: $(OBJS)
	$(CC) -o main $(OBJS)

ASM.o: ASM.cpp
	$(CC) $(CCFLAGS) -c $<

ASMParser.o: ASMParser.cpp $(HEAD)
	$(CC) $(CCFLAGS) -c $<

Instruction.o: Instruction.cpp $(HEAD)
	$(CC) $(CCFLAGS) -c $<

Opcode.o: Opcode.cpp $(HEAD)
	$(CC) $(CCFLAGS) -c $<

RegisterTable.o: RegisterTable.cpp $(HEAD)
	$(CC) $(CCFLAGS) -c $<

clean:
	/bin/rm -f main $(OBJS)
