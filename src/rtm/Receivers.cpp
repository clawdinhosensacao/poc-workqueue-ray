#include "Receivers.hpp"

#include <algorithm>

namespace rtm3d::rtm_internal {

std::vector<std::size_t> make_receiver_positions(const Volume3D& vel, std::size_t receiver_stride) {
  const std::size_t nrec = std::max<std::size_t>(2, vel.nx() / receiver_stride);
  std::vector<std::size_t> rx(nrec, 1);
  for (std::size_t ir = 0; ir < nrec; ++ir) {
    rx[ir] = std::min(1 + ir * receiver_stride, vel.nx() - 2);
  }
  return rx;
}

void record_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& src_field,
                      std::vector<float>& rec_data, std::size_t it) {
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    rec_data[it * rx.size() + ir] = src_field[vel.index(rx[ir], sy, sz)];
  }
}

void inject_receivers(const Volume3D& vel, std::size_t sy, std::size_t sz,
                      const std::vector<std::size_t>& rx, const std::vector<float>& rec_data,
                      std::size_t it, std::vector<float>& rec_field) {
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    rec_field[vel.index(rx[ir], sy, sz)] += rec_data[it * rx.size() + ir];
  }
}

}  // namespace rtm3d::rtm_internal
