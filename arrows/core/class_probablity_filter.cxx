/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "class_probablity_filter.h"

#include <vital/vital_foreach.h>

#include <sstream>

namespace kwiver {
namespace arrows {
namespace core {

// ------------------------------------------------------------------
class_probablity_filter::class_probablity_filter()
  : m_keep_all_classes( true )
  , m_threshold( 0.0 )
{
}


// ------------------------------------------------------------------
vital::config_block_sptr
class_probablity_filter::get_configuration() const
{
  // Get base config from base class
  vital::config_block_sptr config = vital::algorithm::get_configuration();

  config->set_value( "threshold", m_threshold, "The threshold to keep a detection." );
  std::string list_of_classes;
  for ( std::set< std::string >::const_iterator i = m_keep_classes.begin(); i != m_keep_classes.end(); ++i )
  {
    list_of_classes += ( list_of_classes.empty() ) ? "" : ";" + *i;
  }

  // Note that specifying a list of classes to keep and a keep-all can be ambiguous.
  // What to do if keep_classes is specified in addition to keep_all_classes?
  config->set_value( "keep_classes", list_of_classes, "What detection classes to keep." );
  config->set_value( "keep_all_classes", m_keep_all_classes, "Keeps all the classes" );

  return config;
}


// ------------------------------------------------------------------
void
class_probablity_filter::set_configuration( vital::config_block_sptr config_in )
{
  vital::config_block_sptr config = this->get_configuration();

  config->merge_config( config_in );
  this->m_threshold = config->get_value< double > ( "threshold" );
  std::string list = config->get_value< std::string > ( "keep_classes" );
  std::string parsed;
  {
    std::stringstream ss( list );

    while ( std::getline( ss, parsed, ';' ) )
    {
      if ( ! parsed.empty() )
      {
        m_keep_classes.insert( parsed );
      }
    }
  }

  m_keep_all_classes = config->get_value< bool > ( "keep_all_classes" );
}


// ------------------------------------------------------------------
bool
class_probablity_filter::check_configuration( vital::config_block_sptr config ) const
{
  return true;
}


// ------------------------------------------------------------------
vital::detected_object_set_sptr
class_probablity_filter::filter( const vital::detected_object_set_sptr input_set ) const
{
  auto ret_set = std::make_shared<vital::detected_object_set>();

  // Get list of all detections from the set.
  auto detections = input_set->select();

  // loop over all detections
  VITAL_FOREACH( auto det, detections )
  {
    bool det_selected( false );
    auto out_dot = std::make_shared<vital::detected_object_type>( );

    // Make sure that there is an associated DOT
    auto input_dot = det->type();
    if ( ! input_dot )
    {
      // This is unexpected - maybe log something
      continue;
    }

    // Get list of class names that are above threshold
    auto selected_names = input_dot->class_names( m_threshold );

    // Loop over all selected class names
    VITAL_FOREACH( const std::string& a_name, selected_names )
    {
      if ( m_keep_all_classes || m_keep_classes.count( a_name ) )
      {
        // insert class-name/score into DOT
        out_dot->set_score( a_name, input_dot->score( a_name ) );
        det_selected = true;
      }
    } // end foreach class-name

    // It this detection has been selected, add it to output list
    // Clone input detection and replace DOT.
    // Add to returned set
    if (det_selected)
    {
      auto out_det = det->clone();
      out_det->set_type( out_dot );
      ret_set->add( out_det );
    }
  } // end foreach detection

  return ret_set;
} // class_probablity_filter::filter

} } }     // end namespace