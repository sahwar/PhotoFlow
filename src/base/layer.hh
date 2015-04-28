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

#ifndef PF_LAYER_H
#define PF_LAYER_H

#include <stdlib.h>

#include <list>
#include <vector>
#include <set>

#include <sigc++/sigc++.h>

#include "pftypes.hh"
#include "processor.hh"
#include "cachebuffer.hh"

namespace PF
{

  class Image;

  class Layer
  {
    friend class LayerManager;

    int32_t id;
    std::string name;
    std::list<Layer*> sublayers;
    std::list<Layer*> imap_layers;
    std::list<Layer*> omap_layers;
    std::vector< std::pair< std::pair<int32_t,int32_t>,bool> > extra_inputs;

    // Flag indicating that the layer has been directly or indirectly
    // modified, and therefore that re-building is needed
    bool dirty;

    bool modified_flag;

    bool visible;

    bool normal;

    ProcessorBase* processor;
    ProcessorBase* blender;

    bool cached;
    std::map<rendermode_t,CacheBuffer*> cache_buffers;

    Image* image;

    bool insert(std::list<PF::Layer*>& list, Layer* l, int32_t lid);
    bool insert_before(std::list<PF::Layer*>& list, Layer* l, int32_t lid);

  public:
    sigc::signal<void> signal_modified;

    Layer(int32_t id, bool cached=false);
    virtual ~Layer()
    {
      std::cout<<"~Layer(): \""<<name<<"\" destructor called."<<std::endl;
      std::map<rendermode_t,CacheBuffer*>::iterator i;
      for(i = cache_buffers.begin(); i != cache_buffers.end(); i++ ) {
        if( i->second )
          delete( i->second );
      }
      //if( cache_buffer ) delete( cache_buffer );
      if( processor ) delete( processor );
      if( blender ) delete( blender );
    }

    std::string get_name() { return name; }
    void set_name( std::string n ) { name = n; modified(); }

    int32_t get_id() { return id; }

    bool is_modified() { return modified_flag; }
    void set_modified() { modified_flag = true; }
    void clear_modified() { modified_flag = false; }
    void modified() {  set_modified(); signal_modified.emit(); }

    bool is_dirty() { return dirty; }
    void set_dirty( bool d ) { dirty = d; }
    void clear_dirty( ) { dirty = false; }
    

    bool is_visible() { return visible; }
    void set_visible( bool d )
    {
      bool old = visible;
      visible = d;
      if( visible != old ) modified();
    }
    void clear_visible( )
    {
      bool old = visible;
      visible = false;
      if( visible != old ) modified();
    }
    
    bool is_group() { return( !normal ); }
    bool is_normal() { return normal; }
    void set_normal( bool d ) { normal = d; }
    
    bool is_cached() { return cached; }
    void set_cached( bool c ) 
    {
      bool changed = (cached != c);
      cached = c;
      if( cached && cache_buffers.empty() ) {
        cache_buffers.insert( std::make_pair(PF_RENDER_PREVIEW, new CacheBuffer()) );
        cache_buffers.insert( std::make_pair(PF_RENDER_NORMAL, new CacheBuffer()) );
      }
      if( cached && changed )
        reset_cache_buffers();
    }
    CacheBuffer* get_cache_buffer( rendermode_t mode )
    {
      std::map<rendermode_t,CacheBuffer*>::iterator i = cache_buffers.find( mode );
      if( i != cache_buffers.end() ) return i->second;
      return NULL;
    }
    void reset_cache_buffers()
    {
      std::map<rendermode_t,CacheBuffer*>::iterator i;
      for(i = cache_buffers.begin(); i != cache_buffers.end(); i++ ) {
        if( i->second )
          i->second->reset();
      }
    }


    ProcessorBase* get_processor() { return processor; }
    void set_processor(ProcessorBase* p);

    ProcessorBase* get_blender() { return blender; }
    void set_blender(ProcessorBase* b);

    Image* get_image() { return image; }
    void set_image( Image* img );

    void set_input(int n, int32_t lid, int32_t imgid, bool blended=true) {
      for( int i = extra_inputs.size(); i <= n; i++ )
        extra_inputs.push_back( std::make_pair(std::make_pair((int32_t)-1,(int32_t)-1),(bool)true) );
      extra_inputs[n].first.first = lid;
      extra_inputs[n].first.second = imgid;
      extra_inputs[n].second = blended; 
    }
    void add_input(int32_t lid, int32_t imgid, bool blended=true)
    {
      extra_inputs.push_back( std::make_pair(std::make_pair(lid,imgid), blended) );
    }
    void remove_input(int32_t lid);
    std::vector< std::pair< std::pair<int32_t,int32_t>,bool> >& get_extra_inputs() { return extra_inputs; }

    std::list<Layer*>& get_sublayers() { return sublayers; }
    std::list<Layer*>& get_imap_layers() { return imap_layers; }
    std::list<Layer*>& get_omap_layers() { return omap_layers; }

    bool sublayers_insert(Layer* l, int32_t lid=-1);
    bool sublayers_insert_before(Layer* l, int32_t lid);

    bool imap_insert(Layer* l, int32_t lid=-1);
    bool imap_insert_before(Layer* l, int32_t lid);

    bool omap_insert(Layer* l, int32_t lid=-1);
    bool omap_insert_before(Layer* l, int32_t lid);

    bool save( std::ostream& ostr, int level );
  };

};


#endif
