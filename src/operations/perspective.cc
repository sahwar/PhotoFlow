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

#include "perspective.hh"
#include "../base/new_operation.hh"

int vips_perspective( VipsImage* in, VipsImage **out, PF::ProcessorBase* proc, ... );


PF::PerspectivePar::PerspectivePar():
      OpParBase(),
      keystones("keystones",this)
{
  set_type( "perspective" );

  keystones.get().push_back( std::make_pair(0.2f, 0.2f) );
  keystones.get().push_back( std::make_pair(0.8f, 0.2f) );
  keystones.get().push_back( std::make_pair(0.8f, 0.8f) );
  keystones.get().push_back( std::make_pair(0.2f, 0.8f) );
  //keystones.get().push_back( std::make_pair(0.29f, 0.15f) );
  //keystones.get().push_back( std::make_pair(0.77f, 0.25f) );
  //keystones.get().push_back( std::make_pair(0.84f, 0.84f) );
  //keystones.get().push_back( std::make_pair(0.27f, 0.88f) );

  set_default_name( _("perspective correction") );
}



VipsImage* PF::PerspectivePar::build(std::vector<VipsImage*>& in, int first,
    VipsImage* imap, VipsImage* omap,
    unsigned int& level)
{
  VipsImage* srcimg = NULL;
  if( in.size() > 0 ) srcimg = in[0];
  if( srcimg == NULL ) return NULL;
  VipsImage* out, *rotated;

  if( is_editing() ) {
    //std::cout<<"PerspectivePar::build(): editing, returning source image"<<std::endl;
    PF_REF( srcimg, "PerspectivePar::build(): srcimg ref (editing mode)" );
    return srcimg;
  }

  if( vips_perspective(in[0], &out, get_processor(), NULL) ) {
    std::cout<<"vips_perspective() failed."<<std::endl;
    PF_REF( in[0], "PerspectivePar::build(): in[0] ref after viprs_perspective() failed")
    return in[0];
  }

  return out;

}


PF::ProcessorBase* PF::new_perspective()
{
  return( new PF::Processor<PF::PerspectivePar,PF::PerspectiveProc>() );
}
