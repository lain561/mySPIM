// Written by:
// Jesiel Reyes
// Ezra Stone
// Ethan McKissic

#include "spimcore.h"

// ALU 
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
    switch (ALUControl) 
    {
        //Arithmetic operations 
        case 0x0:
            *ALUresult = A + B; 
            break;

        case 0x1: 
            *ALUresult = A - B; 
            break; 

        case 0x2: 
            if((int)A < (int)B) //cast to signed int for comparison
                *ALUresult = 1;     
            else 
                *ALUresult = 0; 

        //Same comparasin as above, but keep unsigned 
        case 0x3: 
            if(A < B)
                *ALUresult = 1;     
            else 
                *ALUresult = 0; 
            break; 

        //AND
        case 0x4: 
            *ALUresult = A & B; 
            break; 
        
        //OR
        case 0x5: 
            *ALUresult = A | B; 
            break; 
        
        //LShift 16
        case 0x6: 
            *ALUresult = B<<16; 
            break; 
        
        //Negate A
        case 0x7: 
            *ALUresult = ~A; 
            break; 
    }

    //If result is 1
    if(*ALUresult)
        *Zero = 0; 
    
    //If result is 0 
    else 
        *Zero = 1; 
}

// Instruction fetch 
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
    //If misaligned 
    if(PC % 4 != 0)
        return 1; //Halt! 
    
    //if aligned
    else if(PC % 4 == 0) 
    {
        *instruction = Mem[PC>>2]; //Fetch instruction from the memory 
        return 0; //No halt! 
    }

    else
    {
        return 1;
    }
}


//Instruction partition 
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
    //Create designated bitmasks for each instruction 
    unsigned opMask = 0xfc000000; //option [31-26]

    unsigned r1Mask = 0x03e00000; //option [25-21]

    unsigned r2Mask = 0x001f0000; //option [20-16]
    
    unsigned r3Mask = 0x0000f800; //option [15-11]

    unsigned functMask = 0x0000003f; //option [5-0]

    unsigned offestMask = 0x0000ffff; //option [15-0]

    unsigned jsecMask = 0x03ffffff; //option [25-0]

    //Assign Masks to designated instruction 
    *op = (instruction & opMask) >> 26; //Assign opMask and shift bits

    *r1 = (instruction & r1Mask) >> 21; //Assign Register1 Mask and shift bits

    *r2 = (instruction & r2Mask) >> 16; //Assign Register2 Mask and shift bits

    *r3 = (instruction & r3Mask) >> 11; //Assign Register3 Mask and shift bits

    *funct = (instruction & functMask); //Assign funct mask

    *offset = (instruction & offestMask); //Assign offset mask

    *jsec = (instruction & offestMask); //Assign jsec mask
}


// Instruction decode
int instruction_decode(unsigned op,struct_controls *controls)
{
    //initializes all controls to 0
    controls->ALUOp = 0;
    controls->ALUSrc = 0;
    controls->Branch = 0;
    controls->Jump = 0;
    controls->MemRead = 0;
    controls->MemtoReg = 0;
    controls->MemWrite = 0;
    controls->RegDst = 0;
    controls->RegWrite = 0;

    //if one of the below if statements is entered the controls are set and 0 is returned
    //else 1 is returned

    //checks if it is an Rtype instruction
    if(op == 0x0)
    {
        controls->ALUOp = 7;
        controls->RegDst = 1;
        controls->RegWrite = 1;
    }

    //checks if it is jump
    else if(op == 0x2)
    {
        controls->ALUOp = 2;
        controls->ALUSrc = 2;
        controls->Branch = 2;
        controls->Jump = 1;
        controls->MemtoReg = 2;
        controls->RegDst = 2;
    }

    //checks if it is beq
    else if(op == 0x4)
    {
        controls->ALUOp = 1;
        controls->ALUSrc = 1;
        controls->Branch = 1;
        controls->MemtoReg = 2;
        controls->RegDst = 2;
    }

    //checks if it is addi
    else if(op == 0x8)
    {
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
    }

    //checks if it is slti
    else if(op == 0xa)
    {
        controls->ALUOp = 2;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
    }

    //checks if it is sltiu
    else if(op == 0xb)
    {
        controls->ALUOp = 3;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
    }

    //checks if it is lui
    else if(op == 0xf)
    {
        controls->ALUOp = 6;
        controls->ALUSrc = 1;
        controls->RegWrite = 1;
    }

    //checks if it is lw
    else if(op == 0x23)
    {
        controls->ALUSrc = 1;
        controls->MemRead = 1;
        controls->MemtoReg = 1;
        controls->RegWrite = 1;
    }

    //checks if it is sw
    else if(op == 0x2b)
    {
        controls->ALUSrc = 1;
        controls->MemtoReg = 2;
        controls->MemWrite = 1;
        controls->RegDst = 2;
    }

    //else meaning none of the above cases applied to op 1 is returned and signals a halt condition
    else
    {
        return 1;
    }

    //means one of the cases applied and no halt condition is signald
    return 0;
}

// Read Register
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
    //gets the values of the registers from the register array
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}


// Sign Extend
void sign_extend(unsigned offset,unsigned *extended_value)
{
    //if statement to determine if the offsett is negative
    if((offset >> 15) == 1)
    {
        //if it is fill it with 1's
        *extended_value = offset | 0xffff0000;
    }
    else
    {
        //else fill it with 0's
        *extended_value = offset & 0x0000ffff;
    }
}

// ALU operations 
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
    // Check for improper instruction
    if (ALUOp < 0 || ALUOp > 7)
        return 1;

    // Handle R-type instructions
    if (ALUOp == 7) {
        switch (funct) {
            case 0x20: // Add
                *ALUresult = data1 + data2;
                break;
            case 0x22: // Subtract
                *ALUresult = data1 - data2;
                break;
            case 0x24: // And
                *ALUresult = data1 & data2;
                break;
            case 0x25: // Or
                *ALUresult = data1 | data2;
                break;
            case 0x2a: // Set less than
                *ALUresult = (data1 < data2) ? 1 : 0;
                break;
            case 0x2b: // Set less than unsigned
                *ALUresult = ((unsigned)data1 < (unsigned)data2) ? 1 : 0;
                break;
            default: // Invalid instruction: Halt
                return 1;
        }
    } else { // Handle other ALUOp values
        // Determine ALU operation based on ALUOp
        switch (ALUOp) {
            case 0: // Add
                *ALUresult = data1 + (ALUSrc ? extended_value : data2);
                break;
            case 1: // Subtract
                *ALUresult = data1 - (ALUSrc ? extended_value : data2);
                break;
            case 2: // Set less than
                *ALUresult = (data1 < (ALUSrc ? extended_value : data2)) ? 1 : 0;
                break;
            case 3: // Set less than unsigned
                *ALUresult = ((unsigned)data1 < (ALUSrc ? extended_value : data2)) ? 1 : 0;
                break;
            case 4: // And
                *ALUresult = data1 & (ALUSrc ? extended_value : data2);
                break;
            case 5: // Or
                *ALUresult = data1 | (ALUSrc ? extended_value : data2);
                break;
            case 6: // Shift left logical
                *ALUresult = data1 << (ALUSrc ? extended_value : data2);
                break;
            case 7: // Shift right logical
                *ALUresult = data1 >> (ALUSrc ? extended_value : data2);
                break;
            default: // Invalid instruction: Halt
                return 1;
        }
    }

    // Set Zero flag based on result
    *Zero = (*ALUresult == 0) ? 1 : 0;

    return 0; // Operation successful
}

//Read / Write Memory 
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
    unsigned mem_index = ALUresult >> 2;

    // Check if reading from memory
    if (MemRead && (ALUresult % 4) == 0) {
        *memdata = Mem[mem_index];
        return 0; // Successful read
    }

    // Check if writing to memory
    if (MemWrite && (ALUresult % 4) == 0) {
        Mem[mem_index] = data2;
        return 0; // Successful write
    }

    // Improper Address: Halt
    return 1;
}


// Write Register 
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
    if(RegWrite) {
        if (MemtoReg) {
            unsigned target_register = RegDst ? r3 : r2;
            Reg[target_register] = memdata;
        }
        // If Result to Register
        else {
            unsigned target_register = RegDst ? r3 : r2;
            Reg[target_register] = ALUresult;
        }
    }
}

// PC update 
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
    // Increments PC by 4 in memory, which should move it to the next instruction in the memory
    *PC += 4;

    // Checks if both brach and zero are both equal to 1 which would mean that the branch instruction is being executed and zero was true
    // Because of this we need to extend the value of PC by adding the extended value shifted 2 bits, essentially multiplying it by 4
    if(Branch ==  1 && Zero == 1){
        *PC += extended_value << 2;
    }
    // Checks if there is a jump instruction be executed
    // 
    else if ( Jump == 1) {
        *PC = (*PC & 0xf000000) | (jsec << 2);
    }
}
