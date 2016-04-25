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

#include "input_adapter_process.h"

#include <vital/vital_foreach.h>

#include <stdexcept>
#include <sstream>

namespace kwiver {

// ------------------------------------------------------------------
input_adapter_process
::input_adapter_process( kwiver::vital::config_block_sptr const& config )
  : process( config )
{
  // Attach our logger name to process logger
  attach_logger( kwiver::vital::get_logger( name() ) ); // could use a better approach
}


input_adapter_process
::~input_adapter_process()
{ }


// ------------------------------------------------------------------
kwiver::adapter::ports_info_t
input_adapter_process
::get_ports()
{
  kwiver::adapter::ports_info_t port_info;

  // formulate list of current input ports
  sprokit::process::ports_t ports = this->output_ports();
  VITAL_FOREACH( auto port, ports )
  {
    port_info[port] = this->input_port_info( port );
  }

  return port_info;
}


// ------------------------------------------------------------------
sprokit::process::port_info_t
input_adapter_process
::_input_port_info( sprokit::process::port_t const& port )
{
  // If we have not created the port, then make a new one.
  if ( m_active_ports.count( port ) == 0 )
  {
    port_flags_t required;
    required.insert(flag_required);

    // create a new port
    declare_input_port( port, // port name
                        type_any, // port type
                        required,
                        port_description_t("Input for " + port)
      );

    // Add to our list of existing ports
    m_active_ports.insert( port );
  }

  return process::_input_port_info(port);
}


// ------------------------------------------------------------------
void
input_adapter_process
::_configure()
{
  // handle config items here

}


// ------------------------------------------------------------------
void
input_adapter_process
::_init()
{
  // post connection initialization

}


// ------------------------------------------------------------------
void
input_adapter_process
::_step()
{
  auto set = this->get_interface_queue()->Receive(); // blocks
  std::set< sprokit::process::port_t > unused_ports = m_active_ports; // copy set of active ports

  // Handle end of input as last data supplied.
  if (set->type() == kwiver::adapter::adapter_data_set::end_of_input)
  {
    LOG_DEBUG( logger(), "End of input reached, process terminating" );

    // indicate done
    mark_process_as_complete();
    const sprokit::datum_t dat = sprokit::datum::complete_datum();

    auto ie = set->end();
    auto ix = set->begin();
    for ( ; ix != ie; ++ix )
    {
      // Push each datum to their port
      push_datum_to_port( ix->first, dat );
    }
    return;
  }

  // We have real data to send down the pipeline.
  // Need to assure that all defined ports have a datum, and
  // there are no unconnected ports specified.

  auto ie = set->end();
  auto ix = set->begin();
  for ( ; ix != ie; ++ix )
  {
    // validate the port name against our list of created ports
    if ( m_active_ports.count( ix->first ) == 1 )
    {
      // Push each datum to their port
      this->push_datum_to_port( ix->first, ix->second );

      // remove this port from set so it can not be used again
      unused_ports.erase( ix->first );
    }
    else
    {
      std::stringstream str;
      str << "Process " << name() << ": Unconnected port \"" << ix->first << "\" specified in data packet. ";
      LOG_ERROR( logger(), str.str() );
      throw std::runtime_error( str.str() );
    }
  } // end for

  // check to see if all ports have been supplied with a datum
  if ( unused_ports.size() != 0 )
  {
    std::stringstream str;
    str << "Process: " << name() << ": Port \"" << ix->first << "\" has not been supplied with a datum";
    LOG_ERROR( logger(), str.str() );
    throw std::runtime_error( str.str() );
  }

  return;
}

} // end namespace