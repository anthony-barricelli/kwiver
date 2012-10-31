/*ckwg +5
 * Copyright 2011-2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VISTK_PIPELINE_UTIL_PIPE_DECLARATION_TYPES_H
#define VISTK_PIPELINE_UTIL_PIPE_DECLARATION_TYPES_H

#include <vistk/pipeline/config.h>
#include <vistk/pipeline/process.h>
#include <vistk/pipeline/process_registry.h>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <string>
#include <vector>

/**
 * \file pipe_declaration_types.h
 *
 * \brief Types for the AST of a pipeline declaration.
 */

namespace vistk
{

/// The type for a token in the AST.
typedef std::string token_t;

/// The type for a flag on a configuration key.
typedef token_t config_flag_t;
/// The type for a collection of flags on a configuration key.
typedef std::vector<config_flag_t> config_flags_t;

/// The type for a configuration provider.
typedef token_t config_provider_t;

/**
 * \struct config_key_options_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief Options for a configuration key.
 */
struct config_key_options_t
{
  /// Flags on the configuration (if requested).
  boost::optional<config_flags_t> flags;
  /// The configuration provider (if requested).
  boost::optional<config_provider_t> provider;
};

/**
 * \struct config_key_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for information on a configuration key.
 */
struct config_key_t
{
  /// The configuration path.
  config::keys_t key_path;
  /// Options for the key.
  config_key_options_t options;
};

/**
 * \struct config_value_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a configuration value.
 */
struct config_value_t
{
  /// The key for the configuration.
  config_key_t key;
  /// The value of the configuration.
  config::value_t value;
};

/// The type for a collection of configuration values.
typedef std::vector<config_value_t> config_values_t;

/**
 * \struct config_pipe_block pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a configuration block.
 */
struct config_pipe_block
{
  /// The common path of the configuration.
  config::keys_t key;
  /// The values for the configuration block.
  config_values_t values;
};

/**
 * \struct process_pipe_block pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a process block.
 */
struct process_pipe_block
{
  /// The name of the process.
  process::name_t name;
  /// The type of the process.
  process::type_t type;
  /// Associated configuration values.
  config_values_t config_values;
};

/**
 * \struct connect_pipe_block pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a connection block.
 */
struct connect_pipe_block
{
  /// The address of the upstream port.
  process::port_addr_t from;
  /// The address of the downstream port.
  process::port_addr_t to;
};

/// A discriminating union over all available pipeline block types.
typedef boost::variant
  < config_pipe_block
  , process_pipe_block
  , connect_pipe_block
  > pipe_block;

/// A type for a collection of pipe blocks.
typedef std::vector<pipe_block> pipe_blocks;

/**
 * \struct cluster_config_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for cluster config.
 */
struct cluster_config_t
{
  /// Description of the configuration value.
  config::description_t description;
  /// The configuration declaration.
  config_value_t config_value;
};

/**
 * \struct cluster_input_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a cluster input mapping.
 */
struct cluster_input_t
{
  /// Description of the cluster's port.
  process::port_description_t description;
  /// The name of the cluster's input port.
  process::port_t from;
  /// The addresses of the mapped port.
  process::port_addrs_t targets;
};

/**
 * \struct cluster_output_t pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a cluster output mapping.
 */
struct cluster_output_t
{
  /// Description of the cluster's port.
  process::port_description_t description;
  /// The address of the mapped upstream port.
  process::port_addr_t from;
  /// The name of the cluster's output port.
  process::port_t to;
};

/// A variant over the possible blocks that may be contained within a cluster.
typedef boost::variant<cluster_config_t, cluster_input_t, cluster_output_t> cluster_subblock_t;
/// A type for a collection of cluster subblocks.
typedef std::vector<cluster_subblock_t> cluster_subblocks_t;

/**
 * \struct cluster_pipe_block pipe_declaration_types.h <vistk/pipeline_util/pipe_declaration_types.h>
 *
 * \brief A structure for a cluster block.
 */
struct cluster_pipe_block
{
  /// The name of the cluster.
  process::name_t name;
  /// The description of the cluster.
  process_registry::description_t description;
  /// The type of the cluster.
  process::type_t type;
  /// Subblocks of the cluster.
  cluster_subblocks_t subblocks;
};

/// A discriminating union over all available cluster block types.
typedef boost::variant
  < config_pipe_block
  , process_pipe_block
  , connect_pipe_block
  , cluster_pipe_block
  > cluster_block;

/// A type for a collection of cluster blocks.
typedef std::vector<cluster_block> cluster_blocks;

}

#endif // VISTK_PIPELINE_UTIL_PIPE_DECLARATION_TYPES_H
