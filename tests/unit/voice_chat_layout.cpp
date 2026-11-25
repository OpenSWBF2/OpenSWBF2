#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "Battlefront2/Source/GameVoiceChatDecoder.h"
#include "Battlefront2/Source/GameVoiceChatEncoder.h"
#include "Battlefront2/Source/GameVoiceChatNode.h"
#include "Battlefront2/Source/VoiceChatLayout.h"

namespace
{

struct Region
{
    size_t uOffset;
    size_t uBytes;
    size_t uAlign;
};

struct DecoderPin
{
    unsigned int uiSamples;
    unsigned int uiSlots;
    unsigned int auiOffsets[10];
    unsigned int uiSize;
};

struct EncoderPin
{
    unsigned int uiSamples;
    unsigned int uiEncodedBytes;
    unsigned int auiOffsets[4];
    unsigned int uiSize;
};

const DecoderPin kDecoderPinsX86[] = {
    { 160u, 1u, { 0x18u, 0x30u, 0x304u, 0x444u, 0x45Cu, 0x464u, 0x46Cu, 0x470u, 0x6F0u, 0x720u }, 0x9ACu },
    { 160u, 2u, { 0x30u, 0x60u, 0x608u, 0x888u, 0x8B8u, 0x8C8u, 0x8D8u, 0x8E0u, 0xB60u, 0xB90u }, 0xE1Cu },
    { 160u, 3u, { 0x48u, 0x90u, 0x90Cu, 0xCCCu, 0xD14u, 0xD2Cu, 0xD44u, 0xD50u, 0xFD0u, 0x1000u }, 0x128Cu },
    { 160u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x1110u, 0x1170u, 0x1190u, 0x11B0u, 0x11C0u, 0x1440u, 0x1470u }, 0x16FCu },
    { 160u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x1554u, 0x15CCu, 0x15F4u, 0x161Cu, 0x1630u, 0x18B0u, 0x18E0u }, 0x1B6Cu },
    { 160u, 6u, { 0x90u, 0x120u, 0x1218u, 0x1998u, 0x1A28u, 0x1A58u, 0x1A88u, 0x1AA0u, 0x1D20u, 0x1D50u }, 0x1FDCu },
    { 160u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0x1DDCu, 0x1E84u, 0x1EBCu, 0x1EF4u, 0x1F10u, 0x2190u, 0x21C0u }, 0x244Cu },
    { 160u, 8u, { 0xC0u, 0x180u, 0x1820u, 0x2220u, 0x22E0u, 0x2320u, 0x2360u, 0x2380u, 0x2600u, 0x2630u }, 0x28BCu },
    { 160u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0x2664u, 0x273Cu, 0x2784u, 0x27CCu, 0x27F0u, 0x2A70u, 0x2AA0u }, 0x2D2Cu },
    { 160u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x2AA8u, 0x2B98u, 0x2BE8u, 0x2C38u, 0x2C60u, 0x2EE0u, 0x2F10u }, 0x319Cu },
    { 160u, 11u, { 0x108u, 0x210u, 0x212Cu, 0x2EECu, 0x2FF4u, 0x304Cu, 0x30A4u, 0x30D0u, 0x3350u, 0x3380u }, 0x360Cu },
    { 160u, 12u, { 0x120u, 0x240u, 0x2430u, 0x3330u, 0x3450u, 0x34B0u, 0x3510u, 0x3540u, 0x37C0u, 0x37F0u }, 0x3A7Cu },
    { 320u, 1u, { 0x18u, 0x30u, 0x304u, 0x584u, 0x59Cu, 0x5A4u, 0x5ACu, 0x5B0u, 0xAB0u, 0xAE0u }, 0xFECu },
    { 320u, 2u, { 0x30u, 0x60u, 0x608u, 0xB08u, 0xB38u, 0xB48u, 0xB58u, 0xB60u, 0x1060u, 0x1090u }, 0x159Cu },
    { 320u, 3u, { 0x48u, 0x90u, 0x90Cu, 0x108Cu, 0x10D4u, 0x10ECu, 0x1104u, 0x1110u, 0x1610u, 0x1640u }, 0x1B4Cu },
    { 320u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x1610u, 0x1670u, 0x1690u, 0x16B0u, 0x16C0u, 0x1BC0u, 0x1BF0u }, 0x20FCu },
    { 320u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x1B94u, 0x1C0Cu, 0x1C34u, 0x1C5Cu, 0x1C70u, 0x2170u, 0x21A0u }, 0x26ACu },
    { 320u, 6u, { 0x90u, 0x120u, 0x1218u, 0x2118u, 0x21A8u, 0x21D8u, 0x2208u, 0x2220u, 0x2720u, 0x2750u }, 0x2C5Cu },
    { 320u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0x269Cu, 0x2744u, 0x277Cu, 0x27B4u, 0x27D0u, 0x2CD0u, 0x2D00u }, 0x320Cu },
    { 320u, 8u, { 0xC0u, 0x180u, 0x1820u, 0x2C20u, 0x2CE0u, 0x2D20u, 0x2D60u, 0x2D80u, 0x3280u, 0x32B0u }, 0x37BCu },
    { 320u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0x31A4u, 0x327Cu, 0x32C4u, 0x330Cu, 0x3330u, 0x3830u, 0x3860u }, 0x3D6Cu },
    { 320u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x3728u, 0x3818u, 0x3868u, 0x38B8u, 0x38E0u, 0x3DE0u, 0x3E10u }, 0x431Cu },
    { 320u, 11u, { 0x108u, 0x210u, 0x212Cu, 0x3CACu, 0x3DB4u, 0x3E0Cu, 0x3E64u, 0x3E90u, 0x4390u, 0x43C0u }, 0x48CCu },
    { 320u, 12u, { 0x120u, 0x240u, 0x2430u, 0x4230u, 0x4350u, 0x43B0u, 0x4410u, 0x4440u, 0x4940u, 0x4970u }, 0x4E7Cu },
    { 800u, 1u, { 0x18u, 0x30u, 0x304u, 0x944u, 0x95Cu, 0x964u, 0x96Cu, 0x970u, 0x15F0u, 0x1620u }, 0x22ACu },
    { 800u, 2u, { 0x30u, 0x60u, 0x608u, 0x1288u, 0x12B8u, 0x12C8u, 0x12D8u, 0x12E0u, 0x1F60u, 0x1F90u }, 0x2C1Cu },
    { 800u, 3u, { 0x48u, 0x90u, 0x90Cu, 0x1BCCu, 0x1C14u, 0x1C2Cu, 0x1C44u, 0x1C50u, 0x28D0u, 0x2900u }, 0x358Cu },
    { 800u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x2510u, 0x2570u, 0x2590u, 0x25B0u, 0x25C0u, 0x3240u, 0x3270u }, 0x3EFCu },
    { 800u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x2E54u, 0x2ECCu, 0x2EF4u, 0x2F1Cu, 0x2F30u, 0x3BB0u, 0x3BE0u }, 0x486Cu },
    { 800u, 6u, { 0x90u, 0x120u, 0x1218u, 0x3798u, 0x3828u, 0x3858u, 0x3888u, 0x38A0u, 0x4520u, 0x4550u }, 0x51DCu },
    { 800u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0x40DCu, 0x4184u, 0x41BCu, 0x41F4u, 0x4210u, 0x4E90u, 0x4EC0u }, 0x5B4Cu },
    { 800u, 8u, { 0xC0u, 0x180u, 0x1820u, 0x4A20u, 0x4AE0u, 0x4B20u, 0x4B60u, 0x4B80u, 0x5800u, 0x5830u }, 0x64BCu },
    { 800u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0x5364u, 0x543Cu, 0x5484u, 0x54CCu, 0x54F0u, 0x6170u, 0x61A0u }, 0x6E2Cu },
    { 800u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x5CA8u, 0x5D98u, 0x5DE8u, 0x5E38u, 0x5E60u, 0x6AE0u, 0x6B10u }, 0x779Cu },
    { 800u, 11u, { 0x108u, 0x210u, 0x212Cu, 0x65ECu, 0x66F4u, 0x674Cu, 0x67A4u, 0x67D0u, 0x7450u, 0x7480u }, 0x810Cu },
    { 800u, 12u, { 0x120u, 0x240u, 0x2430u, 0x6F30u, 0x7050u, 0x70B0u, 0x7110u, 0x7140u, 0x7DC0u, 0x7DF0u }, 0x8A7Cu },
    { 1600u, 1u, { 0x18u, 0x30u, 0x304u, 0xF84u, 0xF9Cu, 0xFA4u, 0xFACu, 0xFB0u, 0x28B0u, 0x28E0u }, 0x41ECu },
    { 1600u, 2u, { 0x30u, 0x60u, 0x608u, 0x1F08u, 0x1F38u, 0x1F48u, 0x1F58u, 0x1F60u, 0x3860u, 0x3890u }, 0x519Cu },
    { 1600u, 3u, { 0x48u, 0x90u, 0x90Cu, 0x2E8Cu, 0x2ED4u, 0x2EECu, 0x2F04u, 0x2F10u, 0x4810u, 0x4840u }, 0x614Cu },
    { 1600u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x3E10u, 0x3E70u, 0x3E90u, 0x3EB0u, 0x3EC0u, 0x57C0u, 0x57F0u }, 0x70FCu },
    { 1600u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x4D94u, 0x4E0Cu, 0x4E34u, 0x4E5Cu, 0x4E70u, 0x6770u, 0x67A0u }, 0x80ACu },
    { 1600u, 6u, { 0x90u, 0x120u, 0x1218u, 0x5D18u, 0x5DA8u, 0x5DD8u, 0x5E08u, 0x5E20u, 0x7720u, 0x7750u }, 0x905Cu },
    { 1600u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0x6C9Cu, 0x6D44u, 0x6D7Cu, 0x6DB4u, 0x6DD0u, 0x86D0u, 0x8700u }, 0xA00Cu },
    { 1600u, 8u, { 0xC0u, 0x180u, 0x1820u, 0x7C20u, 0x7CE0u, 0x7D20u, 0x7D60u, 0x7D80u, 0x9680u, 0x96B0u }, 0xAFBCu },
    { 1600u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0x8BA4u, 0x8C7Cu, 0x8CC4u, 0x8D0Cu, 0x8D30u, 0xA630u, 0xA660u }, 0xBF6Cu },
    { 1600u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x9B28u, 0x9C18u, 0x9C68u, 0x9CB8u, 0x9CE0u, 0xB5E0u, 0xB610u }, 0xCF1Cu },
    { 1600u, 11u, { 0x108u, 0x210u, 0x212Cu, 0xAAACu, 0xABB4u, 0xAC0Cu, 0xAC64u, 0xAC90u, 0xC590u, 0xC5C0u }, 0xDECCu },
    { 1600u, 12u, { 0x120u, 0x240u, 0x2430u, 0xBA30u, 0xBB50u, 0xBBB0u, 0xBC10u, 0xBC40u, 0xD540u, 0xD570u }, 0xEE7Cu },
    { 3200u, 1u, { 0x18u, 0x30u, 0x304u, 0x1C04u, 0x1C1Cu, 0x1C24u, 0x1C2Cu, 0x1C30u, 0x4E30u, 0x4E60u }, 0x806Cu },
    { 3200u, 2u, { 0x30u, 0x60u, 0x608u, 0x3808u, 0x3838u, 0x3848u, 0x3858u, 0x3860u, 0x6A60u, 0x6A90u }, 0x9C9Cu },
    { 3200u, 3u, { 0x48u, 0x90u, 0x90Cu, 0x540Cu, 0x5454u, 0x546Cu, 0x5484u, 0x5490u, 0x8690u, 0x86C0u }, 0xB8CCu },
    { 3200u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x7010u, 0x7070u, 0x7090u, 0x70B0u, 0x70C0u, 0xA2C0u, 0xA2F0u }, 0xD4FCu },
    { 3200u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x8C14u, 0x8C8Cu, 0x8CB4u, 0x8CDCu, 0x8CF0u, 0xBEF0u, 0xBF20u }, 0xF12Cu },
    { 3200u, 6u, { 0x90u, 0x120u, 0x1218u, 0xA818u, 0xA8A8u, 0xA8D8u, 0xA908u, 0xA920u, 0xDB20u, 0xDB50u }, 0x10D5Cu },
    { 3200u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0xC41Cu, 0xC4C4u, 0xC4FCu, 0xC534u, 0xC550u, 0xF750u, 0xF780u }, 0x1298Cu },
    { 3200u, 8u, { 0xC0u, 0x180u, 0x1820u, 0xE020u, 0xE0E0u, 0xE120u, 0xE160u, 0xE180u, 0x11380u, 0x113B0u }, 0x145BCu },
    { 3200u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0xFC24u, 0xFCFCu, 0xFD44u, 0xFD8Cu, 0xFDB0u, 0x12FB0u, 0x12FE0u }, 0x161ECu },
    { 3200u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x11828u, 0x11918u, 0x11968u, 0x119B8u, 0x119E0u, 0x14BE0u, 0x14C10u }, 0x17E1Cu },
    { 3200u, 11u, { 0x108u, 0x210u, 0x212Cu, 0x1342Cu, 0x13534u, 0x1358Cu, 0x135E4u, 0x13610u, 0x16810u, 0x16840u }, 0x19A4Cu },
    { 3200u, 12u, { 0x120u, 0x240u, 0x2430u, 0x15030u, 0x15150u, 0x151B0u, 0x15210u, 0x15240u, 0x18440u, 0x18470u }, 0x1B67Cu },
    { 162u, 1u, { 0x18u, 0x30u, 0x304u, 0x448u, 0x460u, 0x468u, 0x470u, 0x474u, 0x6FCu, 0x72Cu }, 0x9C0u },
    { 162u, 2u, { 0x30u, 0x60u, 0x608u, 0x890u, 0x8C0u, 0x8D0u, 0x8E0u, 0x8E8u, 0xB70u, 0xBA0u }, 0xE34u },
    { 162u, 3u, { 0x48u, 0x90u, 0x90Cu, 0xCD8u, 0xD20u, 0xD38u, 0xD50u, 0xD5Cu, 0xFE4u, 0x1014u }, 0x12A8u },
    { 162u, 4u, { 0x60u, 0xC0u, 0xC10u, 0x1120u, 0x1180u, 0x11A0u, 0x11C0u, 0x11D0u, 0x1458u, 0x1488u }, 0x171Cu },
    { 162u, 5u, { 0x78u, 0xF0u, 0xF14u, 0x1568u, 0x15E0u, 0x1608u, 0x1630u, 0x1644u, 0x18CCu, 0x18FCu }, 0x1B90u },
    { 162u, 6u, { 0x90u, 0x120u, 0x1218u, 0x19B0u, 0x1A40u, 0x1A70u, 0x1AA0u, 0x1AB8u, 0x1D40u, 0x1D70u }, 0x2004u },
    { 162u, 7u, { 0xA8u, 0x150u, 0x151Cu, 0x1DF8u, 0x1EA0u, 0x1ED8u, 0x1F10u, 0x1F2Cu, 0x21B4u, 0x21E4u }, 0x2478u },
    { 162u, 8u, { 0xC0u, 0x180u, 0x1820u, 0x2240u, 0x2300u, 0x2340u, 0x2380u, 0x23A0u, 0x2628u, 0x2658u }, 0x28ECu },
    { 162u, 9u, { 0xD8u, 0x1B0u, 0x1B24u, 0x2688u, 0x2760u, 0x27A8u, 0x27F0u, 0x2814u, 0x2A9Cu, 0x2ACCu }, 0x2D60u },
    { 162u, 10u, { 0xF0u, 0x1E0u, 0x1E28u, 0x2AD0u, 0x2BC0u, 0x2C10u, 0x2C60u, 0x2C88u, 0x2F10u, 0x2F40u }, 0x31D4u },
    { 162u, 11u, { 0x108u, 0x210u, 0x212Cu, 0x2F18u, 0x3020u, 0x3078u, 0x30D0u, 0x30FCu, 0x3384u, 0x33B4u }, 0x3648u },
    { 162u, 12u, { 0x120u, 0x240u, 0x2430u, 0x3360u, 0x3480u, 0x34E0u, 0x3540u, 0x3570u, 0x37F8u, 0x3828u }, 0x3ABCu },
};

const EncoderPin kEncoderPinsX86[] = {
    { 1600u, 330u, { 0x1900u, 0x1A4Cu, 0x1A7Cu, 0x1A94u }, 0x1AA8u },
    { 1920u, 396u, { 0x1E00u, 0x1F8Cu, 0x1FBCu, 0x1FD4u }, 0x1FE8u },
    { 2400u, 495u, { 0x2580u, 0x2770u, 0x27A0u, 0x27B8u }, 0x27CCu },
    { 3200u, 660u, { 0x3200u, 0x3494u, 0x34C4u, 0x34DCu }, 0x34F0u },
};

struct NodeStaticPin
{
    unsigned int uiNodes;
    unsigned int uiBitmapBytes;
    unsigned int auiOffsets[3];
    unsigned int uiClearBytes;
    unsigned int uiSize;
};

struct NodePin
{
    unsigned int uiNodes;
    unsigned int uiPackets;
    unsigned int uiBytes;
    unsigned int auiOffsets[3];
    unsigned int uiSize;
};

const NodeStaticPin kNodeStaticPinsX86[] = {
    { 1u, 2u, { 0x4u, 0x8u, 0x2Cu }, 0x4Cu, 0x4Cu },
    { 2u, 2u, { 0x4u, 0x8u, 0x50u }, 0x90u, 0x90u },
    { 3u, 3u, { 0x4u, 0x8u, 0x74u }, 0xD4u, 0xD4u },
    { 4u, 3u, { 0x4u, 0x8u, 0x98u }, 0x118u, 0x118u },
    { 5u, 3u, { 0x4u, 0x8u, 0xBCu }, 0x15Cu, 0x15Cu },
    { 6u, 3u, { 0x4u, 0x8u, 0xE0u }, 0x1A0u, 0x1A0u },
    { 7u, 3u, { 0x4u, 0x8u, 0x104u }, 0x1E4u, 0x1E4u },
    { 8u, 3u, { 0x4u, 0x8u, 0x128u }, 0x228u, 0x228u },
    { 9u, 4u, { 0x4u, 0x8u, 0x14Cu }, 0x26Cu, 0x26Cu },
    { 10u, 4u, { 0x4u, 0x8u, 0x170u }, 0x2B0u, 0x2B0u },
    { 11u, 4u, { 0x4u, 0x8u, 0x194u }, 0x2F4u, 0x2F4u },
    { 12u, 4u, { 0x4u, 0x8u, 0x1B8u }, 0x338u, 0x338u },
    { 13u, 4u, { 0x4u, 0x8u, 0x1DCu }, 0x37Cu, 0x37Cu },
    { 14u, 4u, { 0x4u, 0x8u, 0x200u }, 0x3C0u, 0x3C0u },
    { 15u, 4u, { 0x4u, 0x8u, 0x224u }, 0x404u, 0x404u },
    { 16u, 5u, { 0x8u, 0x10u, 0x250u }, 0x450u, 0x450u },
    { 17u, 5u, { 0x8u, 0x10u, 0x274u }, 0x494u, 0x494u },
    { 18u, 5u, { 0x8u, 0x10u, 0x298u }, 0x4D8u, 0x4D8u },
    { 19u, 5u, { 0x8u, 0x10u, 0x2BCu }, 0x51Cu, 0x51Cu },
    { 20u, 5u, { 0x8u, 0x10u, 0x2E0u }, 0x560u, 0x560u },
    { 21u, 5u, { 0x8u, 0x10u, 0x304u }, 0x5A4u, 0x5A4u },
    { 22u, 5u, { 0x8u, 0x10u, 0x328u }, 0x5E8u, 0x5E8u },
    { 23u, 5u, { 0x8u, 0x10u, 0x34Cu }, 0x62Cu, 0x62Cu },
    { 24u, 6u, { 0x8u, 0x10u, 0x370u }, 0x670u, 0x670u },
    { 25u, 6u, { 0x8u, 0x10u, 0x394u }, 0x6B4u, 0x6B4u },
    { 26u, 6u, { 0x8u, 0x10u, 0x3B8u }, 0x6F8u, 0x6F8u },
    { 27u, 6u, { 0x8u, 0x10u, 0x3DCu }, 0x73Cu, 0x73Cu },
    { 28u, 6u, { 0x8u, 0x10u, 0x400u }, 0x780u, 0x780u },
    { 29u, 6u, { 0x8u, 0x10u, 0x424u }, 0x7C4u, 0x7C4u },
    { 30u, 6u, { 0x8u, 0x10u, 0x448u }, 0x808u, 0x808u },
    { 31u, 6u, { 0x8u, 0x10u, 0x46Cu }, 0x84Cu, 0x84Cu },
    { 32u, 7u, { 0x8u, 0x10u, 0x490u }, 0x890u, 0x890u },
    { 33u, 7u, { 0x8u, 0x10u, 0x4B4u }, 0x8D4u, 0x8D4u },
    { 34u, 7u, { 0x8u, 0x10u, 0x4D8u }, 0x918u, 0x918u },
    { 35u, 7u, { 0x8u, 0x10u, 0x4FCu }, 0x95Cu, 0x95Cu },
    { 36u, 7u, { 0x8u, 0x10u, 0x520u }, 0x9A0u, 0x9A0u },
    { 37u, 7u, { 0x8u, 0x10u, 0x544u }, 0x9E4u, 0x9E4u },
    { 38u, 7u, { 0x8u, 0x10u, 0x568u }, 0xA28u, 0xA28u },
    { 39u, 8u, { 0x8u, 0x10u, 0x58Cu }, 0xA6Cu, 0xA6Cu },
    { 40u, 8u, { 0x8u, 0x10u, 0x5B0u }, 0xAB0u, 0xAB0u },
    { 41u, 8u, { 0x8u, 0x10u, 0x5D4u }, 0xAF4u, 0xAF4u },
    { 42u, 8u, { 0x8u, 0x10u, 0x5F8u }, 0xB38u, 0xB38u },
    { 43u, 8u, { 0x8u, 0x10u, 0x61Cu }, 0xB7Cu, 0xB7Cu },
    { 44u, 8u, { 0x8u, 0x10u, 0x640u }, 0xBC0u, 0xBC0u },
    { 45u, 8u, { 0x8u, 0x10u, 0x664u }, 0xC04u, 0xC04u },
    { 46u, 8u, { 0x8u, 0x10u, 0x688u }, 0xC48u, 0xC48u },
    { 47u, 9u, { 0xCu, 0x18u, 0x6B4u }, 0xC94u, 0xC94u },
    { 48u, 9u, { 0xCu, 0x18u, 0x6D8u }, 0xCD8u, 0xCD8u },
    { 49u, 9u, { 0xCu, 0x18u, 0x6FCu }, 0xD1Cu, 0xD1Cu },
    { 50u, 9u, { 0xCu, 0x18u, 0x720u }, 0xD60u, 0xD60u },
};

const NodePin kNodePinsX86[] = {
    { 1u, 5u, 330u, { 0x18u, 0x68u, 0x6E4u }, 0x6F4u },
    { 2u, 5u, 330u, { 0x30u, 0xD0u, 0xDC8u }, 0xDDCu },
    { 3u, 5u, 330u, { 0x48u, 0x138u, 0x14ACu }, 0x14C4u },
    { 4u, 5u, 330u, { 0x60u, 0x1A0u, 0x1B90u }, 0x1BACu },
    { 5u, 5u, 330u, { 0x78u, 0x208u, 0x2274u }, 0x2294u },
    { 6u, 5u, 330u, { 0x90u, 0x270u, 0x2958u }, 0x297Cu },
    { 7u, 5u, 330u, { 0xA8u, 0x2D8u, 0x303Cu }, 0x3064u },
    { 8u, 5u, 330u, { 0xC0u, 0x340u, 0x3720u }, 0x374Cu },
    { 9u, 5u, 330u, { 0xD8u, 0x3A8u, 0x3E04u }, 0x3E34u },
    { 10u, 5u, 330u, { 0xF0u, 0x410u, 0x44E8u }, 0x451Cu },
    { 11u, 5u, 330u, { 0x108u, 0x478u, 0x4BCCu }, 0x4C04u },
    { 12u, 5u, 330u, { 0x120u, 0x4E0u, 0x52B0u }, 0x52ECu },
    { 13u, 5u, 330u, { 0x138u, 0x548u, 0x5994u }, 0x59D4u },
    { 14u, 5u, 330u, { 0x150u, 0x5B0u, 0x6078u }, 0x60BCu },
    { 15u, 5u, 330u, { 0x168u, 0x618u, 0x675Cu }, 0x67A4u },
    { 16u, 5u, 330u, { 0x180u, 0x680u, 0x6E40u }, 0x6E8Cu },
    { 17u, 5u, 330u, { 0x198u, 0x6E8u, 0x7524u }, 0x7574u },
    { 18u, 5u, 330u, { 0x1B0u, 0x750u, 0x7C08u }, 0x7C5Cu },
    { 19u, 5u, 330u, { 0x1C8u, 0x7B8u, 0x82ECu }, 0x8344u },
    { 20u, 5u, 330u, { 0x1E0u, 0x820u, 0x89D0u }, 0x8A2Cu },
    { 21u, 5u, 330u, { 0x1F8u, 0x888u, 0x90B4u }, 0x9114u },
    { 22u, 5u, 330u, { 0x210u, 0x8F0u, 0x9798u }, 0x97FCu },
    { 23u, 5u, 330u, { 0x228u, 0x958u, 0x9E7Cu }, 0x9EE4u },
    { 24u, 5u, 330u, { 0x240u, 0x9C0u, 0xA560u }, 0xA5CCu },
    { 25u, 5u, 330u, { 0x258u, 0xA28u, 0xAC44u }, 0xACB4u },
    { 26u, 5u, 330u, { 0x270u, 0xA90u, 0xB328u }, 0xB39Cu },
    { 27u, 5u, 330u, { 0x288u, 0xAF8u, 0xBA0Cu }, 0xBA84u },
    { 28u, 5u, 330u, { 0x2A0u, 0xB60u, 0xC0F0u }, 0xC16Cu },
    { 29u, 5u, 330u, { 0x2B8u, 0xBC8u, 0xC7D4u }, 0xC854u },
    { 30u, 5u, 330u, { 0x2D0u, 0xC30u, 0xCEB8u }, 0xCF3Cu },
    { 31u, 5u, 330u, { 0x2E8u, 0xC98u, 0xD59Cu }, 0xD624u },
    { 32u, 5u, 330u, { 0x300u, 0xD00u, 0xDC80u }, 0xDD0Cu },
    { 33u, 5u, 330u, { 0x318u, 0xD68u, 0xE364u }, 0xE3F4u },
    { 34u, 5u, 330u, { 0x330u, 0xDD0u, 0xEA48u }, 0xEADCu },
    { 35u, 5u, 330u, { 0x348u, 0xE38u, 0xF12Cu }, 0xF1C4u },
    { 36u, 5u, 330u, { 0x360u, 0xEA0u, 0xF810u }, 0xF8ACu },
    { 37u, 5u, 330u, { 0x378u, 0xF08u, 0xFEF4u }, 0xFF94u },
    { 38u, 5u, 330u, { 0x390u, 0xF70u, 0x105D8u }, 0x1067Cu },
    { 39u, 5u, 330u, { 0x3A8u, 0xFD8u, 0x10CBCu }, 0x10D64u },
    { 40u, 5u, 330u, { 0x3C0u, 0x1040u, 0x113A0u }, 0x1144Cu },
    { 41u, 5u, 330u, { 0x3D8u, 0x10A8u, 0x11A84u }, 0x11B34u },
    { 42u, 5u, 330u, { 0x3F0u, 0x1110u, 0x12168u }, 0x1221Cu },
    { 43u, 5u, 330u, { 0x408u, 0x1178u, 0x1284Cu }, 0x12904u },
    { 44u, 5u, 330u, { 0x420u, 0x11E0u, 0x12F30u }, 0x12FECu },
    { 45u, 5u, 330u, { 0x438u, 0x1248u, 0x13614u }, 0x136D4u },
    { 46u, 5u, 330u, { 0x450u, 0x12B0u, 0x13CF8u }, 0x13DBCu },
    { 47u, 5u, 330u, { 0x468u, 0x1318u, 0x143DCu }, 0x144A4u },
    { 48u, 5u, 330u, { 0x480u, 0x1380u, 0x14AC0u }, 0x14B8Cu },
    { 49u, 5u, 330u, { 0x498u, 0x13E8u, 0x151A4u }, 0x15274u },
    { 50u, 5u, 330u, { 0x4B0u, 0x1450u, 0x15888u }, 0x1595Cu },
    { 1u, 4u, 396u, { 0x18u, 0x58u, 0x688u }, 0x698u },
    { 2u, 4u, 396u, { 0x30u, 0xB0u, 0xD10u }, 0xD24u },
    { 3u, 4u, 396u, { 0x48u, 0x108u, 0x1398u }, 0x13B0u },
    { 49u, 4u, 396u, { 0x498u, 0x10D8u, 0x14008u }, 0x140D8u },
    { 50u, 4u, 396u, { 0x4B0u, 0x1130u, 0x14690u }, 0x14764u },
    { 1u, 3u, 495u, { 0x18u, 0x48u, 0x618u }, 0x628u },
    { 2u, 3u, 495u, { 0x30u, 0x90u, 0xC30u }, 0xC44u },
    { 3u, 3u, 495u, { 0x48u, 0xD8u, 0x1248u }, 0x1260u },
    { 49u, 3u, 495u, { 0x498u, 0xDC8u, 0x12A98u }, 0x12B68u },
    { 50u, 3u, 495u, { 0x4B0u, 0xE10u, 0x130B0u }, 0x13184u },
    { 1u, 2u, 660u, { 0x18u, 0x38u, 0x560u }, 0x570u },
    { 2u, 2u, 660u, { 0x30u, 0x70u, 0xAC0u }, 0xAD4u },
    { 3u, 2u, 660u, { 0x48u, 0xA8u, 0x1020u }, 0x1038u },
    { 49u, 2u, 660u, { 0x498u, 0xAB8u, 0x10760u }, 0x10830u },
    { 50u, 2u, 660u, { 0x4B0u, 0xAF0u, 0x10CC0u }, 0x10D94u },
};

size_t StrideBytes(unsigned int uiSamples)
{
    return (uiSamples * 2 + 3) / 4 * 4;
}

unsigned int NodesLog2(unsigned int uiNodes)
{
    unsigned int uiLog2 = 1;
    while ((uiNodes >> uiLog2) != 0)
    {
        ++uiLog2;
    }
    return uiLog2;
}

template <class T>
void CheckStorage(const char* pcName)
{
    const uintptr_t uHigh = (uintptr_t)0x7FFF0000u * (sizeof(void*) == 8 ? 0x10000u : 1u);
    const size_t uBytes = VoiceChatLayout::StorageBytes<T>();
    CHECK(uBytes >= sizeof(T) + alignof(T) - 1);
    for (uintptr_t r = 0; r < 2 * alignof(T); ++r)
    {
        INFO("storage " << pcName << " residue " << r);
        unsigned char* const pStorage = (unsigned char*)(uHigh + r);
        const unsigned char* const pSlot = VoiceChatLayout::StorageSlot<T>(pStorage);
        CHECK((uintptr_t)pSlot % alignof(T) == 0);
        CHECK(pSlot >= pStorage);
        CHECK(pSlot + sizeof(T) <= pStorage + uBytes);
    }
}

void CheckRegions(uintptr_t uBase, const Region* pRegions, unsigned int uiCount, size_t uSize)
{
    CHECK(pRegions[0].uOffset == 0);
    size_t uEnd = 0;
    for (unsigned int i = 0; i < uiCount; ++i)
    {
        INFO("region " << i);
        CHECK((uBase + pRegions[i].uOffset) % pRegions[i].uAlign == 0);
        CHECK(pRegions[i].uOffset >= uEnd);
        uEnd = pRegions[i].uOffset + pRegions[i].uBytes;
    }
    CHECK(uEnd <= uSize);
}

}

TEST_CASE_NAMED(FVoiceChatLayoutTest1, "Unit::VoiceChatLayout::voice chat decoder and encoder carving fits and aligns", "[EngineFilter]")
{
    const size_t uPtr = alignof(void*);
    const uintptr_t uHigh = (uintptr_t)0x7FFF0000u * (sizeof(void*) == 8 ? 0x10000u : 1u);
    const uintptr_t auBases[] = { 0, uPtr, 2 * uPtr, 3 * uPtr, uHigh + uPtr, uHigh + 3 * uPtr };
    for (unsigned int k = 0; k < sizeof(kDecoderPinsX86) / sizeof(kDecoderPinsX86[0]); ++k)
    {
        const DecoderPin& kPin = kDecoderPinsX86[k];
        const unsigned int uiSlots = kPin.uiSlots;
        const unsigned int uiSamples = kPin.uiSamples;
        const size_t uStride = StrideBytes(uiSamples);
        const size_t uSize = VoiceChatLayout::DecoderBlockSize(uiSlots, uiSamples);
        for (unsigned int b = 0; b < sizeof(auBases) / sizeof(auBases[0]); ++b)
        {
            INFO("decoder samples " << uiSamples << " slots " << uiSlots << " base " << auBases[b]);
            const VoiceChatLayout::DecoderCarve kCarve = VoiceChatLayout::CarveDecoder((const char*)auBases[b], uiSlots, uiSamples);
            const Region akRegions[] = {
                { 0, uiSlots * sizeof(Snd::StreamSourceProducer), alignof(Snd::StreamSourceProducer) },
                { kCarve.uProducerPackets, uiSlots * sizeof(Snd::StreamPacket), alignof(Snd::StreamPacket) },
                { kCarve.uDecoders, uiSlots * sizeof(Snd::StreamGSMDecoder), alignof(Snd::StreamGSMDecoder) },
                { kCarve.uDecoderBuffer, uiSlots * uStride, alignof(short) },
                { kCarve.uDecoderPackets, uiSlots * sizeof(Snd::StreamPacket), alignof(Snd::StreamPacket) },
                { kCarve.uMixerConnections, uiSlots * sizeof(Snd::StreamNodeConnection), alignof(Snd::StreamNodeConnection) },
                { kCarve.uMixerPacketIO, uiSlots * sizeof(Snd::StreamPacketIO), alignof(Snd::StreamPacketIO) },
                { kCarve.uMixerBufferOffsets, uiSlots * sizeof(unsigned int), alignof(unsigned int) },
                { kCarve.uMixerAccBuffer, uiSamples * sizeof(int), alignof(int) },
                { kCarve.uMixerPackets, 2 * sizeof(Snd::StreamPacket), alignof(Snd::StreamPacket) },
                { kCarve.uMixerOutBuffer, 2 * uStride, alignof(short) },
            };
            CheckRegions(auBases[b], akRegions, sizeof(akRegions) / sizeof(akRegions[0]), uSize);
            if (sizeof(void*) == 4)
            {
                for (unsigned int r = 1; r < sizeof(akRegions) / sizeof(akRegions[0]); ++r)
                {
                    INFO("region " << r);
                    CHECK(akRegions[r].uOffset == kPin.auiOffsets[r - 1]);
                }
                CHECK(uSize == kPin.uiSize);
            }
        }
    }
    for (unsigned int k = 0; k < sizeof(kEncoderPinsX86) / sizeof(kEncoderPinsX86[0]); ++k)
    {
        const EncoderPin& kPin = kEncoderPinsX86[k];
        const unsigned int uiSamples = kPin.uiSamples;
        const unsigned int uiEncodedBytes = kPin.uiEncodedBytes;
        const size_t uSize = VoiceChatLayout::EncoderBlockSize(uiSamples, uiEncodedBytes);
        for (unsigned int b = 0; b < sizeof(auBases) / sizeof(auBases[0]); ++b)
        {
            INFO("encoder samples " << uiSamples << " base " << auBases[b]);
            const VoiceChatLayout::EncoderCarve kCarve = VoiceChatLayout::CarveEncoder((const char*)auBases[b], uiSamples, uiEncodedBytes);
            const Region akRegions[] = {
                { 0, 2 * StrideBytes(uiSamples), alignof(short) },
                { kCarve.uEncoderBuffer, uiEncodedBytes, 1 },
                { kCarve.uCapturePackets, 2 * sizeof(Snd::StreamPacket), alignof(Snd::StreamPacket) },
                { kCarve.uEncoderPackets, sizeof(Snd::StreamPacket), alignof(Snd::StreamPacket) },
                { kCarve.uAudioPacket, sizeof(VoiceChat::AudioPacket), alignof(VoiceChat::AudioPacket) },
            };
            CheckRegions(auBases[b], akRegions, sizeof(akRegions) / sizeof(akRegions[0]), uSize);
            if (sizeof(void*) == 4)
            {
                for (unsigned int r = 1; r < sizeof(akRegions) / sizeof(akRegions[0]); ++r)
                {
                    INFO("region " << r);
                    CHECK(akRegions[r].uOffset == kPin.auiOffsets[r - 1]);
                }
                CHECK(uSize == kPin.uiSize);
            }
        }
    }
}

TEST_CASE_NAMED(FVoiceChatLayoutTest2, "Unit::VoiceChatLayout::voice chat node carving fits and aligns", "[EngineFilter]")
{
    typedef VoiceChat::Node VN;
    const size_t uPtr = alignof(void*);
    const uintptr_t uHigh = (uintptr_t)0x7FFF0000u * (sizeof(void*) == 8 ? 0x10000u : 1u);
    const uintptr_t auBases[] = { 0, uPtr, 2 * uPtr, 3 * uPtr, uHigh + uPtr, uHigh + 3 * uPtr };
    for (unsigned int uiNodes = 1; uiNodes <= 50; ++uiNodes)
    {
        const unsigned int uiBitmapBytes = VN::Header::GetMaxBytes(uiNodes, NodesLog2(uiNodes));
        const size_t uSize = VoiceChatLayout::NodeStaticBlockSize(uiNodes, uiBitmapBytes);
        const VoiceChatLayout::NodeStaticCarve kCarve = VoiceChatLayout::CarveNodeStatic(uiNodes, uiBitmapBytes);
        for (unsigned int b = 0; b < sizeof(auBases) / sizeof(auBases[0]); ++b)
        {
            INFO("static nodes " << uiNodes << " base " << auBases[b]);
            const Region akRegions[] = {
                { 0, uiBitmapBytes, 1 },
                { kCarve.uWriteBitmap, uiBitmapBytes, 1 },
                { kCarve.uReceiveMetrics, uiNodes * sizeof(VN::ReceiveMetrics), alignof(VN::ReceiveMetrics) },
                { kCarve.uSendMetrics, uiNodes * sizeof(VN::SendMetrics), alignof(VN::SendMetrics) },
            };
            CheckRegions(auBases[b], akRegions, sizeof(akRegions) / sizeof(akRegions[0]), uSize);
        }
        CHECK(kCarve.uWriteBitmap % 4 == 0);
        CHECK(kCarve.uReceiveMetrics % 4 == 0);
        CHECK(kCarve.uiClearBytes <= uSize);
        {
            unsigned char aucBlock[0x1000];
            REQUIRE(uSize <= sizeof(aucBlock));
            memset(aucBlock, 0xCD, sizeof(aucBlock));
            memset(aucBlock, 0, kCarve.uiClearBytes);
            const size_t uLastSend = kCarve.uSendMetrics + (uiNodes - 1) * sizeof(VN::SendMetrics);
            const VN::SendMetrics* const pLast = (const VN::SendMetrics*)(aucBlock + uLastSend);
            CHECK(pLast->bandwidth == 0.0f);
            CHECK(pLast->bandwidthAverage == 0.0f);
            size_t uDirty = 0;
            for (size_t i = 0; i < uLastSend + sizeof(VN::SendMetrics); ++i)
            {
                uDirty += aucBlock[i] != 0 ? 1u : 0u;
            }
            CHECK(uDirty == 0);
        }
        if (sizeof(void*) == 4)
        {
            const NodeStaticPin& kPin = kNodeStaticPinsX86[uiNodes - 1];
            REQUIRE(kPin.uiNodes == uiNodes);
            CHECK(uiBitmapBytes == kPin.uiBitmapBytes);
            CHECK(kCarve.uWriteBitmap == kPin.auiOffsets[0]);
            CHECK(kCarve.uReceiveMetrics == kPin.auiOffsets[1]);
            CHECK(kCarve.uSendMetrics == kPin.auiOffsets[2]);
            CHECK(kCarve.uiClearBytes == kPin.uiClearBytes);
            CHECK(uSize == kPin.uiSize);
        }
    }
    const unsigned int auiBytes[] = { 330u, 396u, 495u, 660u };
    const size_t uInputs = 0x0C;
    for (unsigned int uiNodes = 1; uiNodes <= 50; ++uiNodes)
    {
        for (unsigned int uiPackets = 0; uiPackets <= 5; ++uiPackets)
        {
            for (unsigned int y = 0; y < sizeof(auiBytes) / sizeof(auiBytes[0]); ++y)
            {
                const unsigned int uiBytes = auiBytes[y];
                const size_t uStride = (uiBytes + 3) / 4 * 4;
                const size_t uSize = VoiceChatLayout::NodeBlockSize(uiNodes, uiPackets, uiBytes);
                for (unsigned int b = 0; b < sizeof(auBases) / sizeof(auBases[0]); ++b)
                {
                    INFO("node nodes " << uiNodes << " packets " << uiPackets << " bytes " << uiBytes << " base " << auBases[b]);
                    const VoiceChatLayout::NodeCarve kCarve = VoiceChatLayout::CarveNode((const char*)auBases[b], uiNodes, uiPackets, uiBytes);
                    const Region akRegions[] = {
                        { 0, uiNodes * sizeof(VN::ReceiveStream), alignof(VN::ReceiveStream) },
                        { kCarve.uPackets, uiPackets * uiNodes * sizeof(VoiceChat::AudioPacket), alignof(VoiceChat::AudioPacket) },
                        { kCarve.uData, uiPackets * uiNodes * uStride, 4 },
                        { kCarve.uAllocatedInputs, uInputs, 1 },
                    };
                    CheckRegions(auBases[b], akRegions, sizeof(akRegions) / sizeof(akRegions[0]), uSize);
                }
            }
        }
    }
    if (sizeof(void*) == 4)
    {
        for (unsigned int k = 0; k < sizeof(kNodePinsX86) / sizeof(kNodePinsX86[0]); ++k)
        {
            const NodePin& kPin = kNodePinsX86[k];
            INFO("node pin nodes " << kPin.uiNodes << " packets " << kPin.uiPackets << " bytes " << kPin.uiBytes);
            const VoiceChatLayout::NodeCarve kCarve = VoiceChatLayout::CarveNode((const char*)uPtr, kPin.uiNodes, kPin.uiPackets, kPin.uiBytes);
            CHECK(kCarve.uPackets == kPin.auiOffsets[0]);
            CHECK(kCarve.uData == kPin.auiOffsets[1]);
            CHECK(kCarve.uAllocatedInputs == kPin.auiOffsets[2]);
            CHECK(VoiceChatLayout::NodeBlockSize(kPin.uiNodes, kPin.uiPackets, kPin.uiBytes) == kPin.uiSize);
        }
        CHECK(VoiceChatLayout::StorageBytes<GameVoiceChat::Decoder>() == 656u);
        CHECK(VoiceChatLayout::StorageBytes<GameVoiceChat::Encoder>() == 1296u);
        CHECK(VoiceChatLayout::StorageBytes<GameVoiceChat::Node>() == 84u);
    }
    CheckStorage<GameVoiceChat::Decoder>("decoder");
    CheckStorage<GameVoiceChat::Encoder>("encoder");
    CheckStorage<GameVoiceChat::Node>("node");
}
