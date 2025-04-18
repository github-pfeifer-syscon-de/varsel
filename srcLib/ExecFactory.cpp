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
#include <glibmm.h>
#include <psc_i18n.hpp>

#include "ExecFactory.hpp"
#include "config.h"

ExecFactory::ExecFactory()
: EventBusListener::EventBusListener()
{
}

Gtk::MenuItem *
ExecFactory::createItem(const PtrEventItem& item, Gtk::Menu* gtkMenu)
{
    auto menuItem = Gtk::make_managed<Gtk::MenuItem>(item->getFile()->get_basename());
    gtkMenu->append(*menuItem);
    std::vector<PtrEventItem> items;
    items.push_back(item);
    menuItem->signal_activate().connect(
        sigc::bind(
            sigc::mem_fun(*this, &ExecFactory::createShellWindow)
        , items));
    return menuItem;
}

void
ExecFactory::notify(const std::vector<PtrEventItem>& files, Gtk::Menu* gtkMenu)
{
    Gtk::MenuItem* allItem{};
    std::vector<PtrEventItem> archivItems;
    for (auto& item : files) {
        auto file = item->getFile();
        auto type = file->query_file_type();
#       ifdef DEBUG
        std::cout << "ExecFactory::notify"
                  << " testing " << file->get_path()
                  << " type " << type << std::endl;
#       endif
        if (type == Gio::FileType::FILE_TYPE_REGULAR) {
            if (isExecutable(item->getFileInfo())) {
                if (!allItem) {
                    allItem = Gtk::make_managed<Gtk::MenuItem>(Glib::ustring::sprintf(_("Exec %s"), "all"));
                    gtkMenu->append(*allItem);
                }
                archivItems.push_back(item);
                createItem(item, gtkMenu);
            }
        }
    }
    if (allItem) {
        allItem->signal_activate().connect(
            sigc::bind(
                  sigc::mem_fun(*this, &ExecFactory::createShellWindow)
                , archivItems));
    }
}


bool
ExecFactory::isExecutable(const Glib::RefPtr<Gio::FileInfo>& fileInfo)
{
    auto contentType = fileInfo->get_attribute_string("standard::content-type");
    if ("application/x-shellscript" == contentType) {
        return true;
    }
    return false;
}


void
ExecFactory::createShellWindow(const std::vector<PtrEventItem>& eventItem)
{
    for (auto& item : eventItem) {
        Glib::ustring cmd;
        cmd.reserve(64);
        cmd.append(DEBUG ? "src/varsel" : "varsel");
        cmd.append(" ");
        cmd.append(item->getFile()->get_path());
        Glib::spawn_command_line_async(cmd);
    }
}