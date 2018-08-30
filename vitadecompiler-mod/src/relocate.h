
/// 
/// \file relocate.h
/// \brief Performs SCE ELF relocations
/// \defgroup load Executable Loader
/// \brief Relocates ELFs
/// @{
/// 
#ifndef UVL_RELOCATE
#define UVL_RELOCATE
#ifdef __cplusplus
extern "C" {
#endif
#include "elf.h"

#define PT_SCE_RELA 0x60000000

/** \name SCE Relocation
 *  @{
 */
typedef union sce_reloc
{
    uint32_t       r_type;
    struct
    {
        uint32_t   r_opt1;
        uint32_t   r_opt2;
    } r_short;
    struct
    {
        uint32_t   r_type;
        uint32_t   r_addend;
        uint32_t   r_offset;
    } r_long;
} sce_reloc_t;
/** @}*/

/** \name Macros to get SCE reloc values
 *  @{
 */
//#define SCE_RELOC_SHORT_OFFSET(x) (((x).r_opt1 >> 20) | ((x).r_opt2 & 0xFFFFF) << 12)
//#define SCE_RELOC_SHORT_ADDEND(x) ((x).r_opt2 >> 20)
#define SCE_RELOC_SHORT_OFFSET(x) (((x).r_opt1 >> 20) | ((x).r_opt2 & 0x3FF) << 12)
#define SCE_RELOC_SHORT_ADDEND(x) ((x).r_opt2 >> 10)
#define SCE_RELOC_LONG_OFFSET(x) ((x).r_offset)
#define SCE_RELOC_LONG_ADDEND(x) ((x).r_addend)
#define SCE_RELOC_LONG_CODE2(x) (((x).r_type >> 20) & 0xFF)
#define SCE_RELOC_LONG_DIST2(x) (((x).r_type >> 28) & 0xF)
#define SCE_RELOC_IS_SHORT(x) (((x).r_type) & 0xF)
#define SCE_RELOC_CODE(x) (((x).r_type >> 8) & 0xFF)
#define SCE_RELOC_SYMSEG(x) (((x).r_type >> 4) & 0xF)
#define SCE_RELOC_DATSEG(x) (((x).r_type >> 16) & 0xF)
/** @}*/

/** \name Vita supported relocations
 *  @{
 */
#define R_ARM_NONE              0
#define R_ARM_ABS32             2
#define R_ARM_REL32             3
#define R_ARM_THM_CALL          10
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_TARGET1           38
#define R_ARM_V4BX              40
#define R_ARM_TARGET2           41
#define R_ARM_PREL31            42
#define R_ARM_MOVW_ABS_NC       43
#define R_ARM_MOVT_ABS          44
#define R_ARM_THM_MOVW_ABS_NC   47
#define R_ARM_THM_MOVT_ABS      48
/** @}*/

/** \cond predefined-types
 *  @{
 */
//typedef struct Elf32_Phdr Elf32_Phdr_t;
/** @}*/

/** \name Relocation functions
 *  @{
 */
int uvl_segment_write (Elf32_Phdr *seg, uint32_t offset, uint32_t value, uint32_t size, uint8_t *data_seg);
int uvl_relocate (uint32_t *reloc, uint32_t size, Elf32_Phdr *segs, uint8_t *data_seg, uint8_t *text_seg);
/** @}*/

#ifdef __cplusplus
}
#endif

#endif

/// @}

