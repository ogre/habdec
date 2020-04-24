/*

	Copyright 2018 Michal Fratczak

	This file is part of habdec.

    habdec is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    habdec is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with habdec.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <memory>
#include <functional>
#include <utility>
#include "../ssdv/ssdv.h"

namespace habdec
{

class SSDV_wraper_t
{

public:
    bool push(const std::vector<char>& i_chars);

private:
    // incomming data buffer
    std::vector<uint8_t>    buff_;
    int packet_begin_ = -1;

    // ssdv packet with header
    struct packet_t {
        ssdv_packet_info_t          header_;
        std::array<uint8_t, 256>    data_;
    };

    using packet_t_ptr = std::shared_ptr<packet_t>;

    struct packet_t_ptr_less { // comparator
        bool operator()(const packet_t_ptr& lhs, const packet_t_ptr& rhs) {
            return lhs->header_.packet_id < rhs->header_.packet_id;
        }
    };

    using packet_set_t = std::set<packet_t_ptr, packet_t_ptr_less>;

    using image_map_t = std::map<  // indexed by (callsign,imageID)
                            std::pair<std::string,uint16_t>,
                            packet_set_t >;

    image_map_t images_;

};

}