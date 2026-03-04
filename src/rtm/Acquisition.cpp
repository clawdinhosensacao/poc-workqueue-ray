#include "Acquisition.hpp"

#include "Receivers.hpp"

namespace rtm3d::rtm_internal {

ShotGeometry make_default_shot_geometry(const Volume3D& vel, std::size_t receiver_stride) {
  ShotGeometry g;
  g.sx = vel.nx() / 2;
  g.sy = vel.ny() / 2;
  g.sz = 2;
  g.rx = make_receiver_positions(vel, receiver_stride);
  return g;
}

ShotGeometry make_shot_geometry(const Volume3D& vel, std::size_t sx, std::size_t sz,
                                std::size_t receiver_stride) {
  ShotGeometry g;
  g.sx = sx;
  g.sy = vel.ny() / 2;
  g.sz = sz;
  g.rx = make_receiver_positions(vel, receiver_stride);
  return g;
}

}  // namespace rtm3d::rtm_internal
