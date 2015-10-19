/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

texture2D  g_YTexture;
texture2D  g_UTexture;
texture2D  g_VTexture;
float4x4   g_ColorMatrix;
float2     g_StepXY;
float2     g_viewPort;


SamplerState YUVSampler : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_MIP_LINEAR;
};
#ifdef NV12_SNORM_UV
SamplerState UVSamplerSNORM : IMMUTABLE
{
  AddressU = CLAMP;
  AddressV = CLAMP;
  Filter   = MIN_MAG_MIP_POINT;
};
#endif

struct VS_INPUT
{
  float4 Position   : POSITION;
  float2 TextureY   : TEXCOORD0;
  float2 TextureU   : TEXCOORD1;
  float2 TextureV   : TEXCOORD2;
};

struct VS_OUTPUT
{
  float2 TextureY   : TEXCOORD0;
  float2 TextureU   : TEXCOORD1;
  float2 TextureV   : TEXCOORD2;
  float4 Position   : SV_POSITION;
};

VS_OUTPUT VS(VS_INPUT In)
{
  VS_OUTPUT output = (VS_OUTPUT)0;
  output.Position.x =  (In.Position.x / (g_viewPort.x  / 2.0)) - 1;
  output.Position.y = -(In.Position.y / (g_viewPort.y / 2.0)) + 1;
  output.Position.z = output.Position.z;
  output.Position.w = 1.0;
  output.TextureY   = In.TextureY;
  output.TextureU   = In.TextureU;
  output.TextureV   = In.TextureV;

  return output;
}

#ifdef NV12_SNORM_UV
inline float unormU(float c)
{
  c *= 0.5;
  if (c < 0.0) c += 1.0;
  return saturate(c);
}
inline float2 unormUV(float2 rg)
{
  return float2(unormU(rg.x), unormU(rg.y));
}
#endif

float4 YUV2RGB(VS_OUTPUT In) : SV_TARGET
{
#if defined(XBMC_YV12) //|| defined(XBMC_NV12)
  float4 YUV = float4(g_YTexture.Sample(YUVSampler, In.TextureY).r
                    , g_UTexture.Sample(YUVSampler, In.TextureU).r
                    , g_VTexture.Sample(YUVSampler, In.TextureV).r
                    , 1.0);
#elif defined(XBMC_NV12)
  float4 YUV = float4(g_YTexture.Sample(YUVSampler, In.TextureY).r
  #if defined(NV12_SNORM_UV)
                    , unormUV(g_UTexture.Sample(UVSamplerSNORM, In.TextureU).rg)
  #else
                    , g_UTexture.Sample(YUVSampler, In.TextureU).rg
  #endif
                    , 1.0);
#elif defined(XBMC_YUY2) || defined(XBMC_UYVY)
  // The HLSL compiler is smart enough to optimize away these redundant assignments.
  // That way the code is almost identical to the OGL shader.
  float2 stepxy = g_StepXY;
  float2 pos    = In.TextureY;
  pos           = float2(pos.x - (stepxy.x * 0.25), pos.y);
  float2 f      = frac(pos / stepxy);

  //y axis will be correctly interpolated by opengl
  //x axis will not, so we grab two pixels at the center of two columns and interpolate ourselves
  float4 c1 = g_YTexture.Sample(YUVSampler, float2(pos.x + ((0.5 - f.x) * stepxy.x), pos.y));
  float4 c2 = g_YTexture.Sample(YUVSampler, float2(pos.x + ((1.5 - f.x) * stepxy.x), pos.y));

  /* each pixel has two Y subpixels and one UV subpixel
      YUV  Y  YUV
      check if we're left or right of the middle Y subpixel and interpolate accordingly*/
  #if defined(XBMC_YUY2) // BGRA = YUYV
    float  leftY  = lerp(c1.b, c1.r, f.x * 2.0);
    float  rightY = lerp(c1.r, c2.b, f.x * 2.0 - 1.0);
    float2 outUV  = lerp(c1.ga, c2.ga, f.x);
  #elif defined(XBMC_UYVY) // BGRA = UYVY
    float  leftY  = lerp(c1.g, c1.a, f.x * 2.0);
    float  rightY = lerp(c1.a, c2.g, f.x * 2.0 - 1.0);
    float2 outUV  = lerp(c1.br, c2.br, f.x);
  #endif
    float  outY   = lerp(leftY, rightY, step(0.5, f.x));
    float4 YUV    = float4(outY, outUV, 1.0);
#endif

  return float4(mul(YUV, g_ColorMatrix).rgb, 1.0);
}

technique11 YUV2RGB_T
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0_level_9_1, VS() ) );
    SetPixelShader( CompileShader( ps_4_0_level_9_1, YUV2RGB() ) );
  }
};
