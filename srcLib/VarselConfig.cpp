/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <psc_format.hpp>


#include "VarselConfig.hpp"

VarselConfig::VarselConfig(const char* conf)
: KeyConfig(conf)
{

}

Gdk::RGBA
VarselConfig::getColor(const char* grp, const Glib::ustring& key)
{
    Gdk::RGBA rgba{getString(grp, key, "rgba(0,0,0,1)")};
    std::cout << "VarselConfig::getColor " << getString(grp, key, "rgba(0,0,0,1)")
              << " red " << rgba.get_red()
              << " green " << rgba.get_green()
              << " blue " << rgba.get_blue()
              << " alpha " << rgba.get_alpha()
              << std::endl;
    return rgba;
}

void
VarselConfig::setColor(const char* grp, const Glib::ustring& key, const Gdk::RGBA& rgba)
{
    auto backgrdHtml = psc::fmt::format("rgba({},{},{},{:.5f})",
                               static_cast<int>(rgba.get_red() * COLOR_SCALE)
                             , static_cast<int>(rgba.get_green() * COLOR_SCALE)
                             , static_cast<int>(rgba.get_blue() * COLOR_SCALE)
                             , rgba.get_alpha());
    std::cout << "VarselConfig::setColor " << backgrdHtml
              << " red " << rgba.get_red()
              << " green " << rgba.get_green()
              << " blue " << rgba.get_blue()
              << " alpha " << rgba.get_alpha() << std::endl;
    setString(grp, key, backgrdHtml);
}
