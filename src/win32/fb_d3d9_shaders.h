#define HLSL_SOURCE_CODE 0
#define SHADER_ASSEMBLY_CODE 0

// A paletted texture shader ------------------------------------------------

#if HLSL_SOURCE_CODE
// Technically, Palette only needs to be a sampler1D, but that
// produces assembly code to copy index.x to index.y, which is
// totally unnecessary.

sampler2D Image : register(s0);
sampler2D Palette : register(s1);
float4 Flash : register(c0);
float4 InvFlash : register(c1);
float4 PaletteMod : register(c2);

float4 main (float2 texCoord : TEXCOORD0) : COLOR
{
	float4 index = tex2D (Image, texCoord);
	index.x = index.x * PaletteMod.x + PaletteMod.y;
	float4 rgb = tex2D (Palette, index);
	return Flash + rgb * InvFlash;
}
#elif SHADER_ASSEMBLY_CODE
//
// Generated by Microsoft (R) D3DX9 Shader Compiler 9.15.779.0000
//
//   fxc paltex.ps /Tps_1_4 /VnPalTexShader14Def /Fh
//
//
// Parameters:
//
//   float4 Flash;
//   sampler2D Image;
//   float4 InvFlash;
//   sampler2D Palette;
//   float4 PaletteMod;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   Flash        c0       1
//   InvFlash     c1       1
//   PaletteMod   c2       1
//   Image        s0       1
//   Palette      s1       1
//

    ps_1_4
    texld r0, t0
    mad r0.x, r0.x, c2.x, c2.y
    phase
    texld r1, r0
    mad r0, r1, c1, c0

// approximately 4 instruction slots used (2 texture, 2 arithmetic)
#endif

const DWORD PalTexShader14Def[] =
{
    0xffff0104, 0x0043fffe, 0x42415443, 0x0000001c, 0x000000d3, 0xffff0104,
    0x00000005, 0x0000001c, 0x00000100, 0x000000cc, 0x00000080, 0x00000002,
    0x00020001, 0x00000088, 0x00000000, 0x00000098, 0x00000003, 0x00000001,
    0x000000a0, 0x00000000, 0x000000b0, 0x00010002, 0x00020001, 0x00000088,
    0x00000000, 0x000000b9, 0x00010003, 0x00000001, 0x000000a0, 0x00000000,
    0x000000c1, 0x00020002, 0x00020001, 0x00000088, 0x00000000, 0x73616c46,
    0xabab0068, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x67616d49,
    0xabab0065, 0x000c0004, 0x00010001, 0x00000001, 0x00000000, 0x46766e49,
    0x6873616c, 0x6c615000, 0x65747465, 0x6c615000, 0x65747465, 0x00646f4d,
    0x315f7370, 0x4d00345f, 0x6f726369, 0x74666f73, 0x29522820, 0x44334420,
    0x53203958, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e35312e,
    0x2e393737, 0x30303030, 0xababab00, 0x00000042, 0x800f0000, 0xb0e40000,
    0x00000004, 0x80010000, 0x80000000, 0xa0000002, 0xa0550002, 0x0000fffd,
    0x00000042, 0x800f0001, 0x80e40000, 0x00000004, 0x800f0000, 0x80e40001,
    0xa0e40001, 0xa0e40000, 0x0000ffff
};

#if SHADER_ASSEMBLY_CODE
    ps_2_0
    dcl t0.xy
    dcl_2d s0
    dcl_2d s1
    texld r0, t0, s0
    mad r0.x, r0.x, c2.x, c2.y
    texld r0, r0, s1
    mov r1, c1
    mad r0, r0, r1, c0
    mov oC0, r0

// approximately 6 instruction slots used (2 texture, 4 arithmetic)
#endif

const DWORD PalTexShader20Def[] =
{
    0xffff0200, 0x0043fffe, 0x42415443, 0x0000001c, 0x000000d3, 0xffff0200,
    0x00000005, 0x0000001c, 0x00000100, 0x000000cc, 0x00000080, 0x00000002,
    0x00020001, 0x00000088, 0x00000000, 0x00000098, 0x00000003, 0x00000001,
    0x000000a0, 0x00000000, 0x000000b0, 0x00010002, 0x00020001, 0x00000088,
    0x00000000, 0x000000b9, 0x00010003, 0x00000001, 0x000000a0, 0x00000000,
    0x000000c1, 0x00020002, 0x00020001, 0x00000088, 0x00000000, 0x73616c46,
    0xabab0068, 0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x67616d49,
    0xabab0065, 0x000c0004, 0x00010001, 0x00000001, 0x00000000, 0x46766e49,
    0x6873616c, 0x6c615000, 0x65747465, 0x6c615000, 0x65747465, 0x00646f4d,
    0x325f7370, 0x4d00305f, 0x6f726369, 0x74666f73, 0x29522820, 0x44334420,
    0x53203958, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e35312e,
    0x2e393737, 0x30303030, 0xababab00, 0x0200001f, 0x80000000, 0xb0030000,
    0x0200001f, 0x90000000, 0xa00f0800, 0x0200001f, 0x90000000, 0xa00f0801,
    0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800, 0x04000004, 0x80010000,
    0x80000000, 0xa0000002, 0xa0550002, 0x03000042, 0x800f0000, 0x80e40000,
    0xa0e40801, 0x02000001, 0x800f0001, 0xa0e40001, 0x04000004, 0x800f0000,
    0x80e40000, 0x80e40001, 0xa0e40000, 0x02000001, 0x800f0800, 0x80e40000,
    0x0000ffff
};

// A shader that doesn't look up colors from a palette. ---------------------
// Can be used for RGB textures.

#if HLSL_SOURCE_CODE
sampler2D Image : register(s0);
float4 Flash : register(c0);
float4 InvFlash : register(c1);

float4 main (float2 texCoord : TEXCOORD0) : COLOR
{
  float4 index = tex2D (Image, texCoord);
  return Flash + index * InvFlash;
}
#elif SHADER_ASSEMBLY_CODE
//
// Generated by Microsoft (R) D3DX9 Shader Compiler 9.15.779.0000
//
//   fxc shadetex.ps /Tps_1_4 /VnPlainShaderDef /Fh
//
//
// Parameters:
//
//   float4 Flash;
//   sampler2D Image;
//   float4 InvFlash;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   Flash        c0       1
//   InvFlash     c1       1
//   Image        s0       1
//

    ps_1_4
    texld r0, t0
    mad r0, r0, c1, c0

// approximately 2 instruction slots used (1 texture, 1 arithmetic)
#endif

const DWORD PlainShaderDef[] =
{
    0xffff0104, 0x0034fffe, 0x42415443, 0x0000001c, 0x00000098, 0xffff0104,
    0x00000003, 0x0000001c, 0x00000100, 0x00000091, 0x00000058, 0x00000002,
    0x00020001, 0x00000060, 0x00000000, 0x00000070, 0x00000003, 0x00000001,
    0x00000078, 0x00000000, 0x00000088, 0x00010002, 0x00020001, 0x00000060,
    0x00000000, 0x73616c46, 0xabab0068, 0x00030001, 0x00040001, 0x00000001,
    0x00000000, 0x67616d49, 0xabab0065, 0x000c0004, 0x00010001, 0x00000001,
    0x00000000, 0x46766e49, 0x6873616c, 0x5f737000, 0x00345f31, 0x7263694d,
    0x666f736f, 0x52282074, 0x33442029, 0x20395844, 0x64616853, 0x43207265,
    0x69706d6f, 0x2072656c, 0x35312e39, 0x3937372e, 0x3030302e, 0xabab0030,
    0x00000042, 0x800f0000, 0xb0e40000, 0x00000004, 0x800f0000, 0x80e40000,
    0xa0e40001, 0xa0e40000, 0x0000ffff
};

// A shader that returns the value of c1 for color --------------------------
// but keeps the texture's alpha.

#if HLSL_SOURCE_CODE
sampler2D Image : register(s0);
float4 StencilColor : register(c1);

float4 main (float2 texCoord : TEXCOORD0) : COLOR
{
  float4 color = tex2D (Image, texCoord);
  color.rgb = StencilColor.rgb;
  return color;
}
#elif SHADER_ASSEMBLY_CODE
//
// Generated by Microsoft (R) D3DX9 Shader Compiler 9.15.779.0000
//
//   fxc plainstencil.ps /Tps_1_1 /VnPlainStencilDef /Fh
//
//
// Parameters:
//
//   sampler2D Image;
//   float4 StencilColor;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   StencilColor c1       1
//   Image        s0       1
//

    ps_1_1
    tex t0
    mov r0.xyz, c1
  + mov r0.w, t0.w

// approximately 2 instruction slots used (1 texture, 1 arithmetic)
#endif

const DWORD PlainStencilDef[] =
{
    0xffff0101, 0x002ffffe, 0x42415443, 0x0000001c, 0x00000083, 0xffff0101,
    0x00000002, 0x0000001c, 0x00000100, 0x0000007c, 0x00000044, 0x00000003,
    0x00000001, 0x0000004c, 0x00000000, 0x0000005c, 0x00010002, 0x00020001,
    0x0000006c, 0x00000000, 0x67616d49, 0xabab0065, 0x000c0004, 0x00010001,
    0x00000001, 0x00000000, 0x6e657453, 0x436c6963, 0x726f6c6f, 0xababab00,
    0x00030001, 0x00040001, 0x00000001, 0x00000000, 0x315f7370, 0x4d00315f,
    0x6f726369, 0x74666f73, 0x29522820, 0x44334420, 0x53203958, 0x65646168,
    0x6f432072, 0x6c69706d, 0x39207265, 0x2e35312e, 0x2e393737, 0x30303030,
    0xababab00, 0x00000042, 0xb00f0000, 0x00000001, 0x80070000, 0xa0e40001,
    0x40000001, 0x80080000, 0xb0ff0000, 0x0000ffff
};

// A shader that just returns the value of c1 -------------------------------

#if HLSL_SOURCE_CODE
float4 Color : register(c1);

float4 main () : COLOR
{
  return Color;
}
#elif SHADER_ASSEMBLY_CODE
//
// Generated by Microsoft (R) D3DX9 Shader Compiler 9.15.779.0000
//
//   fxc dimshader.ps /Tps_1_1 /VnDimShader /Fh
//
//
// Parameters:
//
//   float4 Color;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   Color        c1       1
//

    ps_1_1
    mov r0, c1

// approximately 1 instruction slot used
#endif

const DWORD DimShaderDef[] =
{
    0xffff0101, 0x0022fffe, 0x42415443, 0x0000001c, 0x0000004f, 0xffff0101,
    0x00000001, 0x0000001c, 0x00000100, 0x00000048, 0x00000030, 0x00010002,
    0x00020001, 0x00000038, 0x00000000, 0x6f6c6f43, 0xabab0072, 0x00030001,
    0x00040001, 0x00000001, 0x00000000, 0x315f7370, 0x4d00315f, 0x6f726369,
    0x74666f73, 0x29522820, 0x44334420, 0x53203958, 0x65646168, 0x6f432072,
    0x6c69706d, 0x39207265, 0x2e35312e, 0x2e393737, 0x30303030, 0xababab00,
    0x00000001, 0x800f0000, 0xa0e40001, 0x0000ffff
};

// A shader that just corrects gamma for windowed mode ----------------------

#if HLSL_SOURCE_CODE
sampler2D Image : register(s0);
float4 Gamma : register(c4);

float4 main (float2 texCoord : TEXCOORD0) : COLOR
{
  float4 color = tex2D (Image, texCoord);
  color.xyz = pow(color.xyz, Gamma.xyz);
  return color;
}
#elif SHADER_ASSEMBLY_CODE
//
// Generated by Microsoft (R) D3DX9 Shader Compiler 9.15.779.0000
//
//   fxc gammafixer.ps /Tps_2_0 /VnGammaFixerDef /Fh
//
//
// Parameters:
//
//   float4 Gamma;
//   sampler2D Image;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   Gamma        c4       1
//   Image        s0       1
//

    ps_2_0
    dcl t0.xy
    dcl_2d s0
    texld r0, t0, s0
    log r0.x, r0.x
    log r0.y, r0.y
    log r0.z, r0.z
    mul r0.xyz, r0, c4
    exp r0.x, r0.x
    exp r0.y, r0.y
    exp r0.z, r0.z
    mov oC0, r0

// approximately 9 instruction slots used (1 texture, 8 arithmetic)
#endif

const DWORD GammaFixerDef[] =
{
    0xffff0200, 0x002dfffe, 0x42415443, 0x0000001c, 0x0000007b, 0xffff0200,
    0x00000002, 0x0000001c, 0x00000100, 0x00000074, 0x00000044, 0x00040002,
    0x00020001, 0x0000004c, 0x00000000, 0x0000005c, 0x00000003, 0x00000001,
    0x00000064, 0x00000000, 0x6d6d6147, 0xabab0061, 0x00030001, 0x00040001,
    0x00000001, 0x00000000, 0x67616d49, 0xabab0065, 0x000c0004, 0x00010001,
    0x00000001, 0x00000000, 0x325f7370, 0x4d00305f, 0x6f726369, 0x74666f73,
    0x29522820, 0x44334420, 0x53203958, 0x65646168, 0x6f432072, 0x6c69706d,
    0x39207265, 0x2e35312e, 0x2e393737, 0x30303030, 0xababab00, 0x0200001f,
    0x80000000, 0xb0030000, 0x0200001f, 0x90000000, 0xa00f0800, 0x03000042,
    0x800f0000, 0xb0e40000, 0xa0e40800, 0x0200000f, 0x80010000, 0x80000000,
    0x0200000f, 0x80020000, 0x80550000, 0x0200000f, 0x80040000, 0x80aa0000,
    0x03000005, 0x80070000, 0x80e40000, 0xa0e40004, 0x0200000e, 0x80010000,
    0x80000000, 0x0200000e, 0x80020000, 0x80550000, 0x0200000e, 0x80040000,
    0x80aa0000, 0x02000001, 0x800f0800, 0x80e40000, 0x0000ffff
};
