/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_LDI:
        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_STI:
        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs2, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }
        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd,stage->imm);
            break;
        }
        case OPCODE_HALT:
        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

            /* Update PC for next instruction */
            cpu->pc += 4;

            /* Copy data from fetch latch to decode latch*/
            cpu->decode = cpu->fetch;
            
        if (ENABLE_DEBUG_MESSAGES && cpu->simulate && cpu->fetch.has_insn)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                if(cpu->regsStatus[cpu->decode.rs2] == TRUE){
                    cpu->decode.rs2_value = cpu->reg_values[cpu->decode.rs2];
                }
                break;
            }

            case OPCODE_ADDL:
            case OPCODE_SUBL:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                break;
            }

            case OPCODE_STI:
            case OPCODE_STORE:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                if(cpu->regsStatus[cpu->decode.rs2] == TRUE){
                    cpu->decode.rs2_value = cpu->reg_values[cpu->decode.rs2];
                }
                break;
            }

            case OPCODE_LDI:
            case OPCODE_LOAD:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_CMP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                if(cpu->regsStatus[cpu->decode.rs2] == TRUE){
                    cpu->decode.rs2_value = cpu->reg_values[cpu->decode.rs2];
                }
                break;
            }
            case OPCODE_BZ:
            {
                break;
            }
            case OPCODE_BP:
            {
                break;
            }
            case OPCODE_BNP:
            {
                break;
            }
            case OPCODE_NOP:
            {
                break;
            }
            case OPCODE_HALT:
            {
                break;
            }
            case OPCODE_JUMP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                if(cpu->regsStatus[cpu->decode.rs1] == TRUE){
                    cpu->decode.rs1_value = cpu->reg_values[cpu->decode.rs1];
                }
                break;
            }

        }

            /* Copy data from decode latch to execute latch*/
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;

            if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
            {
            print_stage_content("Decode/RF", &cpu->decode);
            }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                    cpu->pos_flag = FALSE;
                } 
                else if(cpu->execute.result_buffer > 0)
                {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                    cpu->pos_flag = FALSE;
                } else if(cpu->execute.result_buffer > 0)
                {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_SUB:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                    cpu->pos_flag = FALSE;
                } else if(cpu->execute.result_buffer > 0)
                {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                    cpu->pos_flag = FALSE;
                } else if(cpu->execute.result_buffer > 0)
                {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value * cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                    cpu->pos_flag = FALSE;
                } else if(cpu->execute.result_buffer > 0)
                {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                } else {
                    cpu->zero_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value & cpu->execute.rs2_value;
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value | cpu->execute.rs2_value;

                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }

            case OPCODE_LDI:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.result_buffer = cpu->execute.rs1_value + 4;
                
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break;
            }


            case OPCODE_STORE:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                break;
            }

            case OPCODE_STI:
            {
                cpu->execute.memory_address
                    = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.rs1_value = cpu->execute.rs1_value + 4;
                break;
            }

            case OPCODE_BP:
            {
                if (cpu->pos_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNP:
            {
                if (cpu->pos_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                cpu->regsStatus[cpu->execute.rd] = TRUE;
                cpu->reg_values[cpu->execute.rd] = cpu->execute.result_buffer;
                break; 
            }

            case OPCODE_CMP:
            {
                if(cpu->execute.rs1_value > cpu->execute.rs2_value){
                    cpu->pos_flag = TRUE;
                    cpu->zero_flag = FALSE;
                }
                else if(cpu->execute.rs1_value < cpu->execute.rs2_value){
                    cpu->pos_flag = FALSE;
                    cpu->zero_flag = FALSE;
                }
                else{
                    cpu->pos_flag = FALSE;
                    cpu->zero_flag = TRUE;
                }
                break;
            }
            case OPCODE_NOP:
            case OPCODE_JUMP:
            {
                cpu->pc = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->fetch_from_next_cycle = TRUE;
                cpu->fetch.has_insn = TRUE;
                cpu->decode.has_insn = FALSE;
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_LOAD:
            case OPCODE_LDI:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                break;
            }
            case OPCODE_STORE:
            case OPCODE_STI:
            {
                /*Store data from destination register to data memory */
                cpu->data_memory[cpu->memory.memory_address]
                     = cpu->memory.rs2_value;
                break;
            }
            case OPCODE_MOVC:
            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_BZ:
            case OPCODE_BNZ:
            case OPCODE_NOP:
            case OPCODE_HALT:
            case OPCODE_CMP:
            case OPCODE_JUMP:
            {
                /* No work */
                break;
            }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_MUL:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regsStatus[cpu->writeback.rd] = FALSE;
                break;
            }

            case OPCODE_LDI:
            {
                cpu->regs[cpu->writeback.rs1] = cpu->writeback.rs1_value;
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regsStatus[cpu->writeback.rd] = FALSE;
                break;
            }
            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regsStatus[cpu->writeback.rd] = FALSE;
                break;
            }

            case OPCODE_MOVC:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                cpu->regsStatus[cpu->writeback.rd] = FALSE;
                break;
            }
            case OPCODE_STI:
            {
                cpu->regs[cpu->writeback.rs1] = cpu->writeback.rs1_value;
                cpu->regsStatus[cpu->writeback.rs1] = FALSE;
                break;
            }
            case OPCODE_STORE:
            case OPCODE_HALT:
            case OPCODE_CMP:
            case OPCODE_BNZ:
            case OPCODE_BZ:
            case OPCODE_BP:
            case OPCODE_BNP:
            case OPCODE_NOP:
            case OPCODE_JUMP:
            {
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename, const char* fun, const int n)
{
    int i;
    APEX_CPU *cpu;

    if (!filename && !fun && !n)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    cpu->simulate = strcmp(fun,"simulate") == 0 ? 0 :1;
    cpu->cycle = n;
    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    memset(cpu->regsStatus, 1, sizeof(int) * REG_FILE_SIZE);
    cpu->single_step = strcmp(fun,"single_step") == 0 ? ENABLE_SINGLE_STEP : 0;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES & cpu->simulate)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu) || cpu->clock == cpu->cycle)
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        if(cpu->single_step){
            print_reg_file(cpu);
        }
        
        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}
static void
print_regstate(APEX_CPU *cpu){

    printf("\n");

    printf("-------------------------------------------\n%s\n-------------------------------------------\n", "STATE OF ARCHITECTURAL REGISTER FILE:");
    for (int i = 0; i < REG_FILE_SIZE ; ++i)
    {
        char status[10];
        if(cpu->regsStatus[i]){
            strcpy(status,"invalid");
        }
        else{
             strcpy(status,"valid");
        }
        printf("|\tR[%d]\t|\tValue=%d \t\t|\tstatus=%s\n", i, cpu->regs[i], status);
    }

    printf("\n");

}

static void
print_mem(APEX_CPU *cpu){

    printf("\n");

    printf("-------------------------------------------\n%s\n-------------------------------------------\n", " STATE OF DATA MEMORY:");
    for (int i = 0; i < 10 ; ++i)
    {
        
        printf("|\tMEM[%d]\t|\tData Value=%d\n", i, cpu->data_memory[i]);
    }

    printf("\n");

}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    if(!cpu->single_step){
        print_regstate(cpu);
        print_mem(cpu);
    }
    free(cpu->code_memory);
    free(cpu);
}
