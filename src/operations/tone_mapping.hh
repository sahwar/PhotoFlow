/* 
 */

/*

  Copyright (C) 2014 Ferrero Andrea

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.


 */

/*

  These files are distributed with PhotoFlow - http://aferrero2707.github.io/PhotoFlow/

 */

#ifndef PF_TONE_MAPPING_H
#define PF_TONE_MAPPING_H

#include <string>

//#include <glibmm.h>

#include "../base/processor.hh"
#include "Filmic/FilmicCurve/FilmicToneCurve.h"


namespace PF 
{


enum tone_mapping_method_t
{
  TONE_MAPPING_EXP_GAMMA,
  TONE_MAPPING_REINHARD,
  TONE_MAPPING_HEJL,
  TONE_MAPPING_FILMIC,
  TONE_MAPPING_FILMIC2
};


class ToneMappingPar: public OpParBase
{
  PropertyBase method;
  Property<float> exposure;
  Property<float> gamma;

  Property<float> filmic_A;
  Property<float> filmic_B;
  Property<float> filmic_C;
  Property<float> filmic_D;
  Property<float> filmic_E;
  Property<float> filmic_F;
  Property<float> filmic_W;

  Property<float> filmic2_TS;
  Property<float> filmic2_TL;
  Property<float> filmic2_SS;
  Property<float> filmic2_SL;
  Property<float> filmic2_SA;

  Property<float> lumi_blend_frac;

  ICCProfile* icc_data;

  float exponent;

public:

  ToneMappingPar();

  tone_mapping_method_t get_method() { return (tone_mapping_method_t)method.get_enum_value().first; }
  float get_exposure() { return exposure.get(); }
  float get_gamma() { return gamma.get(); }
  float get_exponent() { return exponent; }

  float get_filmic_A() { return filmic_A.get(); }
  float get_filmic_B() { return filmic_B.get(); }
  float get_filmic_C() { return filmic_C.get(); }
  float get_filmic_D() { return filmic_D.get(); }
  float get_filmic_E() { return filmic_E.get(); }
  float get_filmic_F() { return filmic_F.get(); }
  float get_filmic_W() { return filmic_W.get(); }

  float get_filmic2_TS() { return filmic2_TS.get(); }
  float get_filmic2_TL() { return filmic2_TL.get(); }
  float get_filmic2_SS() { return filmic2_SS.get(); }
  float get_filmic2_SL() { return filmic2_SL.get(); }
  float get_filmic2_SA() { return filmic2_SA.get(); }

  float get_lumi_blend_frac() { return lumi_blend_frac.get(); }

  ICCProfile* get_icc_data() { return icc_data; }

  bool has_intensity() { return false; }
  bool has_opacity() { return true; }
  bool needs_input() { return true; }

  VipsImage* build(std::vector<VipsImage*>& in, int first,
                   VipsImage* imap, VipsImage* omap, unsigned int& level);
};



template < OP_TEMPLATE_DEF >
class ToneMapping
{
public:
  void render(VipsRegion** ireg, int n, int in_first,
      VipsRegion* imap, VipsRegion* omap,
      VipsRegion* oreg, OpParBase* par)
  {
  }
};




template < class BLENDER, int CHMIN, int CHMAX, bool has_imap, bool has_omap, bool PREVIEW >
class ToneMapping< float, BLENDER, PF_COLORSPACE_RGB, CHMIN, CHMAX, has_imap, has_omap, PREVIEW >
{
public:
  void render(VipsRegion** ireg, int n, int in_first,
      VipsRegion* imap, VipsRegion* omap,
      VipsRegion* oreg, OpParBase* par)
  {
    ToneMappingPar* opar = dynamic_cast<ToneMappingPar*>(par);
    if( !opar ) return;
    Rect *r = &oreg->valid;
    int line_size = r->width * oreg->im->Bands;
    //int width = r->width;
    int height = r->height;

    tone_mapping_method_t method = opar->get_method();

    float exposure = opar->get_exposure();
    float gamma = opar->get_exponent();
    float lumi_blend_frac = opar->get_lumi_blend_frac();
    ICCProfile* prof = opar->get_icc_data();

    float* pin;
    float* pout;
    float RGB[3];
    int x, y, k;

    float A = opar->get_filmic_A();
    float B = opar->get_filmic_B();
    float C = opar->get_filmic_C();
    float D = opar->get_filmic_D();
    float E = opar->get_filmic_E();
    float F = opar->get_filmic_F();
    float W = opar->get_filmic_W();


    // Filmic #2 parameters
    FilmicToneCurve::CurveParamsUser filmic2_user;
    filmic2_user.m_toeStrength      = opar->get_filmic2_TS();
    filmic2_user.m_toeLength        = opar->get_filmic2_TL();
    filmic2_user.m_shoulderStrength = opar->get_filmic2_SS();
    filmic2_user.m_shoulderLength   = opar->get_filmic2_SL();
    if(filmic2_user.m_shoulderLength > 0.9999) filmic2_user.m_shoulderLength = 0.9999;
    filmic2_user.m_shoulderAngle    = opar->get_filmic2_SA();
    filmic2_user.m_gamma            = 1;
    FilmicToneCurve::CurveParamsDirect filmic2_direct;
    FilmicToneCurve::CalcDirectParamsFromUser(filmic2_direct, filmic2_user);
    FilmicToneCurve::FullCurve filmic2_curve;
    FilmicToneCurve::CreateCurve(filmic2_curve, filmic2_direct);

    for( y = 0; y < height; y++ ) {
      pin = (float*)VIPS_REGION_ADDR( ireg[0], r->left, r->top + y );
      pout = (float*)VIPS_REGION_ADDR( oreg, r->left, r->top + y );

      for( x = 0; x < line_size; x+=3 ) {
        RGB[0] = pin[x];
        RGB[1] = pin[x+1];
        RGB[2] = pin[x+2];

        float exposure2 = exposure;
        float gamma2 = gamma;


        if( exposure2 != 0 ) {
          for( k=0; k < 3; k++) {
            RGB[k] *= exposure2;
            //clip( exposure*RGB[k], RGB[k] );
          }
        }

        float minus = -1.f;

        switch( method ) {
        case TONE_MAPPING_EXP_GAMMA:
          if( gamma2 != 1 ) {
            for( k=0; k < 3; k++) {
              RGB[k] = powf( RGB[k], gamma2 );
              //clip( exposure*RGB[k], RGB[k] );
            }
          }
          break;
        case TONE_MAPPING_REINHARD: {
          for( k=0; k < 3; k++) {
            RGB[k] = RGB[k] / (RGB[k] + 1.f);
          }
          break;
        }
        case TONE_MAPPING_HEJL: {
          for( k=0; k < 3; k++) {
            float x = MAX(0,RGB[k]-0.004);
            RGB[k] = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
            RGB[k] = powf( RGB[k], 2.2f );
          }
          break;
        }
        case TONE_MAPPING_FILMIC: {
          float whiteScale = 1.0f/( ((W*(A*W+C*B)+D*E)/(W*(A*W+B)+D*F))-E/F );
          for( k=0; k < 3; k++) {
            RGB[k] *= 2;
            RGB[k] = ((RGB[k]*(A*RGB[k]+C*B)+D*E)/(RGB[k]*(A*RGB[k]+B)+D*F))-E/F;
            RGB[k] *= whiteScale;
          }
          break;
        }
        case TONE_MAPPING_FILMIC2: {
          for( k=0; k < 3; k++) {
            if(RGB[k] < 0) RGB[k] = minus*filmic2_curve.Eval(minus*RGB[k]);
            else RGB[k] = filmic2_curve.Eval(RGB[k]);
          }
          break;
        }
        default:
          break;
        }

        pout[x] = RGB[0];
        pout[x+1] = RGB[1];
        pout[x+2] = RGB[2];

        if( lumi_blend_frac > 0 && prof ) {
          float Lin = prof->get_lightness(pin[x],pin[x+1],pin[x+2]);
          float Lout = prof->get_lightness(pout[x],pout[x+1],pout[x+2]);
          float R = Lout / MAX(Lin, 0.0000000000000000001);
          pout[x]   = (1.f-lumi_blend_frac)*pout[x]   + lumi_blend_frac*R*pin[x];
          pout[x+1] = (1.f-lumi_blend_frac)*pout[x+1] + lumi_blend_frac*R*pin[x+1];
          pout[x+2] = (1.f-lumi_blend_frac)*pout[x+2] + lumi_blend_frac*R*pin[x+2];
        }
      }
    }
  }
};




template < OP_TEMPLATE_DEF_CS_SPEC >
class ToneMapping< OP_TEMPLATE_IMP_CS_SPEC(PF_COLORSPACE_LAB) >
{
public:
  void render(VipsRegion** ireg, int n, int in_first,
      VipsRegion* imap, VipsRegion* omap,
      VipsRegion* oreg, OpParBase* par)
  {
    ToneMappingPar* opar = dynamic_cast<ToneMappingPar*>(par);
    if( !opar ) return;
    Rect *r = &oreg->valid;
    int line_size = r->width * oreg->im->Bands;
    //int width = r->width;
    int height = r->height;

    float exposure = opar->get_exposure();
    float gamma = opar->get_gamma();

    T* pin;
    T* pout;
    int x, y;

    float a, b;

    for( y = 0; y < height; y++ ) {
      pin = (T*)VIPS_REGION_ADDR( ireg[in_first], r->left, r->top + y );
      pout = (T*)VIPS_REGION_ADDR( oreg, r->left, r->top + y );

      float exposure2 = exposure;
      float gamma2 = gamma;

      for( x = 0; x < line_size; x+=3 ) {

        pout[x] = pin[x];
        pout[x+1] = pin[x+1];
        pout[x+2] = pin[x+2];

        if( exposure2 != 0 ) {
          pout[x] *= exposure;
        }

        if( gamma2 != 1 ) {
          pout[x] = powf( pout[x], gamma );
        }
      }
    }
  }
};



ProcessorBase* new_tone_mapping();
}

#endif 


