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
#include <StringUtils.hpp>
#include <psc_i18n.hpp>

#include "SourceFactory.hpp"
#include "config.h"


SourceFactory::SourceFactory()
: EventBusListener::EventBusListener()
{
}

Gtk::MenuItem*
SourceFactory::createItem(const PtrEventItem& item, Gtk::Menu* gtkMenu)
{
    auto menuItem = Gtk::make_managed<Gtk::MenuItem>(item->getFile()->get_basename());
    gtkMenu->append(*menuItem);
    std::vector<PtrEventItem> items;
    items.push_back(item);
    menuItem->signal_activate().connect(
        sigc::bind(
            sigc::mem_fun(*this, &SourceFactory::createSourceWindow)
        , items));
    return menuItem;
}


void
SourceFactory::notify(const std::vector<PtrEventItem>& files, Gtk::Menu* gtkMenu)
{
    Gtk::MenuItem* allItem{};
    std::vector<PtrEventItem> editFiles;
    for (auto& item : files) {
        //GtkSourceLanguage* srcLang = SourceFile::getSourceLanguage(item);
        auto filetype = item->getFile()->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
        auto fileInfo = item->getFileInfo();
        if (filetype == Gio::FileType::FILE_TYPE_REGULAR
         && isEditable(fileInfo)) {   // at least rule out binaries?
            if (!allItem) {
                allItem = Gtk::make_managed<Gtk::MenuItem>(Glib::ustring::sprintf(_("Edit %s"), "all"));
                gtkMenu->append(*allItem);
            }
            editFiles.push_back(item);
            createItem(item, gtkMenu);
        }
        else {
            std::cout << "SourceFactory::notify skipped "
                      << item->getFile()->get_path()
                      << " type " << filetype
                      << " content " << fileInfo->get_attribute_string("standard::content-type") << std::endl;
        }
    }
    if (allItem) {
        allItem->signal_activate().connect(
            sigc::bind(
                  sigc::mem_fun(*this, &SourceFactory::createSourceWindow)
                , editFiles));
    }
}

bool
SourceFactory::isEditable(const Glib::RefPtr<Gio::FileInfo>& fileInfo)
{
    auto contentType = fileInfo->get_attribute_string("standard::content-type");
    if (StringUtils::startsWith(contentType, "text/")) {   // any text should work
        return true;
    }
    if ("application/x-gtk-builder" == contentType
     || "application/xml" == contentType
     || "application/x-shellscript" == contentType) {
        return true;
    }
    return false;
}

void
SourceFactory::createSourceWindow(const std::vector<PtrEventItem>& items)
{
    Glib::ustring cmd;
    cmd.reserve(64);
    cmd.append(DEBUG ? "srcEdit/va_edit" : "va_edit");
    for (auto& eventItem : items) {
        cmd.append(" ");
        cmd.append(eventItem->getFile()->get_path());
    }
    Glib::spawn_command_line_async(cmd);
}
