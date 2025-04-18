/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
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

#pragma once

#include <vector>

#include "EventBus.hpp"

class ExecFactory
: public EventBusListener
{
public:
    ExecFactory();
    explicit ExecFactory(const ExecFactory& listener) = delete;
    virtual ~ExecFactory() = default;

    void notify(const std::vector<PtrEventItem>& files, Gtk::Menu* gtkMenu) override;
    void createShellWindow(const std::vector<PtrEventItem>& files);
protected:


private:
    bool isExecutable(const Glib::RefPtr<Gio::FileInfo>& fileInfo);

    Gtk::MenuItem * createItem(const PtrEventItem& item, Gtk::Menu* gtkMenu);

};
