/*ckwg +29
 * Copyright 2013-2018 by Kitware, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
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

/**
 * \file
 * \brief local_geo_cs implementation
 */

#include "local_geo_cs.h"

#include <fstream>
#include <iomanip>

#include <vital/math_constants.h>

#include <vital/types/geodesy.h>
#include <vital/types/metadata_traits.h>

using namespace kwiver::vital;

namespace kwiver {
namespace vital {

/// Constructor
local_geo_cs
::local_geo_cs()
: geo_origin_(),
  origin_alt_(0.0)
{
}


/// Set the geographic coordinate origin
void
local_geo_cs
::set_origin(const geo_point& origin)
{
  // convert the origin point into WGS84 UTM for the appropriate zone
  vector_2d lon_lat = origin.location( SRID::lat_lon_WGS84 );
  auto zone = utm_ups_zone( lon_lat );
  int crs = (zone.north ? SRID::UTM_WGS84_north : SRID::UTM_WGS84_south) + zone.number;
  geo_origin_ = geo_point(origin.location(crs), crs);
}

/// Use the pose data provided by metadata to update camera pose
bool
local_geo_cs
::update_camera(vital::metadata const& md,
                vital::simple_camera_perspective& cam,
                vital::rotation_d const& rot_offset) const
{
  bool rotation_set = false;
  bool translation_set = false;

  bool has_platform_yaw = false;
  bool has_platform_pitch = false;
  bool has_platform_roll = false;
  bool has_sensor_yaw = false;
  bool has_sensor_pitch = false;

  double platform_yaw = 0.0, platform_pitch = 0.0, platform_roll = 0.0;
  if (md.has(vital::VITAL_META_PLATFORM_HEADING_ANGLE))
  {
    md.find(vital::VITAL_META_PLATFORM_HEADING_ANGLE).data(platform_yaw);
    has_platform_yaw = true;
  }
  if (md.has(vital::VITAL_META_PLATFORM_PITCH_ANGLE))
  {
    md.find(vital::VITAL_META_PLATFORM_PITCH_ANGLE).data(platform_pitch);
    has_platform_pitch = true;
  }
  if (md.has(vital::VITAL_META_PLATFORM_ROLL_ANGLE))
  {
    md.find(vital::VITAL_META_PLATFORM_ROLL_ANGLE).data(platform_roll);
    has_platform_roll = true;
  }
  double sensor_yaw = 0.0, sensor_pitch = 0.0, sensor_roll = 0.0;
  if (md.has(vital::VITAL_META_SENSOR_REL_AZ_ANGLE))
  {
    md.find(vital::VITAL_META_SENSOR_REL_AZ_ANGLE).data(sensor_yaw);
    has_sensor_yaw = true;
  }
  if (md.has(vital::VITAL_META_SENSOR_REL_EL_ANGLE))
  {
    md.find(vital::VITAL_META_SENSOR_REL_EL_ANGLE).data(sensor_pitch);
    has_sensor_pitch = true;
  }
  if (md.has(vital::VITAL_META_SENSOR_REL_ROLL_ANGLE))
  {
    md.find(vital::VITAL_META_SENSOR_REL_ROLL_ANGLE).data(sensor_roll);
  }


  if ( has_platform_yaw && has_platform_pitch && has_platform_roll &&
       has_sensor_yaw && has_sensor_pitch &&  // Sensor roll is ignored here on purpose.
                                              // It is fixed on some platforms to zero.
      !(std::isnan(platform_yaw) || std::isnan(platform_pitch) || std::isnan(platform_roll) ||
        std::isnan(sensor_yaw) || std::isnan(sensor_pitch) || std::isnan(sensor_roll)))
  {
    //only set the camera's rotation if all metadata angles are present

    auto R = compose_rotations<double>(platform_yaw, platform_pitch, platform_roll,
                                       sensor_yaw, sensor_pitch, sensor_roll);

    cam.set_rotation(R);

    rotation_set = true;
  }

  if( md.has( vital::VITAL_META_SENSOR_LOCATION) &&
      md.has( vital::VITAL_META_SENSOR_ALTITUDE) )
  {
    double alt = md.find( vital::VITAL_META_SENSOR_ALTITUDE ).as_double();
    vital::geo_point gloc;
    md.find( vital::VITAL_META_SENSOR_LOCATION ).data( gloc );

    // get the location in the same UTM zone as the origin
    vector_2d loc = gloc.location(geo_origin_.crs());
    loc -= geo_origin_.location();
    cam.set_center(vector_3d(loc.x(), loc.y(), alt - origin_alt_));
    translation_set = true;
  }
  return rotation_set || translation_set;
}

/// Use the camera pose to update the metadata structure
void
local_geo_cs
::update_metadata(vital::simple_camera_perspective const& cam,
                  vital::metadata& md) const
{
  if (md.has(vital::VITAL_META_PLATFORM_HEADING_ANGLE) &&
      md.has(vital::VITAL_META_PLATFORM_PITCH_ANGLE) &&
      md.has(vital::VITAL_META_PLATFORM_ROLL_ANGLE) &&
      md.has(vital::VITAL_META_SENSOR_REL_AZ_ANGLE) &&
      md.has(vital::VITAL_META_SENSOR_REL_EL_ANGLE))
  {  //we have a complete metadata rotation.  Note that sensor roll is ignored here on purpose.
    double yaw, pitch, roll;
    cam.rotation().get_yaw_pitch_roll(yaw, pitch, roll);
    yaw *= rad_to_deg;
    pitch *= rad_to_deg;
    roll *= rad_to_deg;
    md.add(NEW_METADATA_ITEM(VITAL_META_SENSOR_YAW_ANGLE, yaw));
    md.add(NEW_METADATA_ITEM(VITAL_META_SENSOR_PITCH_ANGLE, pitch));
    md.add(NEW_METADATA_ITEM(VITAL_META_SENSOR_ROLL_ANGLE, roll));
  }

  if (md.has(vital::VITAL_META_SENSOR_LOCATION) &&
      md.has(vital::VITAL_META_SENSOR_ALTITUDE))
  {
    // we have a complete position from metadata.
    vital::vector_3d c = cam.get_center();
    vital::geo_point gc(vector_2d(c.x(), c.y()) + geo_origin_.location(),
      geo_origin_.crs());

    md.add(NEW_METADATA_ITEM(VITAL_META_SENSOR_LOCATION, gc));
    md.add(NEW_METADATA_ITEM(VITAL_META_SENSOR_ALTITUDE, c.z() + origin_alt_));
  }
}


/// Read a local_geo_cs from a text file
void
read_local_geo_cs_from_file(local_geo_cs& lgcs,
                            vital::path_t const& file_path)
{
  std::ifstream ifs(file_path);
  double lat, lon, alt;
  ifs >> lat >> lon >> alt;
  lgcs.set_origin( geo_point( vector_2d(lon, lat), SRID::lat_lon_WGS84) );
  lgcs.set_origin_altitude( alt );
}


/// Write a local_geo_cs to a text file
void
write_local_geo_cs_to_file(local_geo_cs const& lgcs,
                           vital::path_t const& file_path)
{
  // write out the origin of the local coordinate system
  auto lon_lat = lgcs.origin().location( SRID::lat_lon_WGS84 );
  std::ofstream ofs(file_path);
  if (ofs)
  {
    ofs << std::setprecision(12) << lon_lat[1] << " " << lon_lat[0]
        << " " << lgcs.origin_altitude();
  }
}

bool set_intrinsics_from_metadata(simple_camera_perspective &cam, std::map<vital::frame_id_t,
  vital::metadata_sptr> const& md_map, vital::image_container_sptr const& im)
{
  auto intrin = std::dynamic_pointer_cast<simple_camera_intrinsics>(cam.intrinsics());
  // TODO: Once the camera_intrinsics has the image width and height we won't
  //       need access to the image
  double im_w = double(im->width());
  double im_h = double(im->height());

  for (auto const &md : md_map)
  {
    if (md.second->has(vital::VITAL_META_SLANT_RANGE) && md.second->has(vital::VITAL_META_TARGET_WIDTH))
    {
      double slant_range=0.0, target_width=0.0;
      md.second->find(vital::VITAL_META_SLANT_RANGE).data(slant_range);
      md.second->find(vital::VITAL_META_TARGET_WIDTH).data(target_width);
      double f = im_w*(slant_range / target_width);
      intrin->set_focal_length(f);
    }
    else if (md.second->has(vital::VITAL_META_SENSOR_HORIZONTAL_FOV))
    {
      double hfov=0.0;
      md.second->find(vital::VITAL_META_SENSOR_HORIZONTAL_FOV).data(hfov);
      double f = (im_w / 2) / tan(0.5*hfov*deg_to_rad);

      intrin->set_focal_length(f);
    }
    else
    {
      continue;
    }

    vital::vector_2d pp(0.5*im_w, 0.5*im_h);
    intrin->set_principal_point(pp);
    intrin->set_aspect_ratio(1.0);
    intrin->set_skew(0.0);
    cam.set_intrinsics(intrin);
    return true;
  }

  return false;
}

/// Use a sequence of metadata objects to initialize a sequence of cameras
std::map<vital::frame_id_t, vital::camera_sptr>
initialize_cameras_with_metadata(std::map<vital::frame_id_t,
                                 vital::metadata_sptr> const& md_map,
                                 vital::simple_camera_perspective const& base_camera,
                                 local_geo_cs& lgcs,
                                 vital::rotation_d const& rot_offset)
{
  std::map<frame_id_t, camera_sptr> cam_map;
  vital::vector_3d mean(0,0,0);
  simple_camera_perspective active_cam(base_camera);

  bool update_local_origin = false;
  if( lgcs.origin().is_empty() && !md_map.empty())
  {
    // if a local coordinate system has not been established,
    // use the coordinates of the first camera
    for( auto m : md_map )
    {
      if( m.second && m.second->has(vital::VITAL_META_SENSOR_LOCATION) )
      {
        vital::geo_point gloc;
        m.second->find(vital::VITAL_META_SENSOR_LOCATION).data(gloc);

        lgcs.set_origin(gloc);
        lgcs.set_origin_altitude(0.0);
        update_local_origin = true;
        break;
      }
    }
  }
  for(auto const& p : md_map)
  {
    auto md = p.second;
    if( !md )
    {
      continue;
    }
    if ( lgcs.update_camera(*md, active_cam, rot_offset) )
    {
      mean += active_cam.center();
      cam_map[p.first] = camera_sptr(new simple_camera_perspective(active_cam));
    }
  }

  if( update_local_origin )
  {
    mean /= static_cast<double>(cam_map.size());
    // only use the mean easting and northing
    mean[2] = 0.0;

    // shift the UTM origin to the mean of the cameras easting and northing
    vector_2d mean_xy( mean.x(), mean.y() );
    lgcs.set_origin( geo_point( lgcs.origin().location() + mean_xy, lgcs.origin().crs() ) );

    // shift all cameras to the new coordinate system.
    typedef std::map<frame_id_t, camera_sptr>::value_type cam_map_val_t;
    for(cam_map_val_t const &p : cam_map)
    {
      simple_camera_perspective* cam = dynamic_cast<simple_camera_perspective*>(p.second.get());
      cam->set_center(cam->get_center() - mean);
    }
  }

  return cam_map;
}


/// Update a sequence of metadata from a sequence of cameras and local_geo_cs
void
update_metadata_from_cameras(std::map<frame_id_t, camera_sptr> const& cam_map,
                             local_geo_cs const& lgcs,
                             std::map<frame_id_t, vital::metadata_sptr>& md_map)
{
  if( lgcs.origin().is_empty() )
  {
    // TODO throw an exception here?
    vital::logger_handle_t
      logger( vital::get_logger( "update_metadata_from_cameras" ) );
    LOG_WARN( logger, "local geo coordinates do not have an origin");
    return;
  }

  typedef std::map<frame_id_t, camera_sptr>::value_type cam_map_val_t;
  for(cam_map_val_t const &p : cam_map)
  {
    auto active_md = md_map[p.first];
    if( !active_md )
    {
      md_map[p.first] = active_md = std::make_shared<vital::metadata>();
    }
    auto cam = dynamic_cast<vital::simple_camera_perspective*>(p.second.get());
    if( active_md && cam )
    {
      lgcs.update_metadata(*cam, *active_md);
    }
  }
}


} // end namespace vital
} // end namespace kwiver
