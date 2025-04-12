/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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

#include "FileDataSource.hpp"
#include "ListApp.hpp"

FileDataSource::FileDataSource(const Glib::RefPtr<Gio::File>& file, ListApp* application)
: DataSource::DataSource(application)
, m_file{file}
{
}

void
FileDataSource::update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    Glib::RefPtr<Gio::FileEnumerator> enumerat;
    try {

        auto fileInfo = m_file->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
        auto treeItem = std::make_shared<FileTreeNode>(fileInfo->get_display_name(), 0);
        treeModel->append(treeItem);

        auto cancellable = Gio::Cancellable::create();
        enumerat = m_file->enumerate_children(
              cancellable
            , "*"
            , Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        while (true) {   //enumerat->has_pending()
            auto fileInfo = enumerat->next_file();
            if (fileInfo) {
                if (fileInfo->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY) {
                    auto subTreeItem = std::make_shared<FileTreeNode>(fileInfo->get_display_name(), treeItem->getDepth() + 1);
                    treeModel->append(treeItem, subTreeItem);
                }
                else if (fileInfo->get_file_type() == Gio::FileType::FILE_TYPE_REGULAR) {
                    auto iter = treeItem->appendList();
                    auto row = *iter;
                    //std::cout << "file name " << name << " size " << fileInfo->get_size() << std::endl;
                    auto listColumns = getListColumns();
                    auto file = m_file->get_child(fileInfo->get_name());
                    setFileValues(row, file, fileInfo, listColumns);
                }
            }
            else {
                break;
            }
        }
    }
    catch (const Gio::Error& err) {
        std::cout << "Error " << err.what() << std::endl;
    }
    if (enumerat
     && !enumerat->is_closed()) {
        enumerat->close();
    }
}

void
FileDataSource::setFileValues(
      Gtk::TreeRow& row
    , const Glib::RefPtr<Gio::File>& file
    , const Glib::RefPtr<Gio::FileInfo>& fileInfo
    , const std::shared_ptr<ListColumns>& listColumns)
{
    row.set_value<Glib::ustring>(listColumns->m_name, fileInfo->get_display_name());
    row.set_value(listColumns->m_size, fileInfo->get_size());
    //row.set_value(listColumns->m_type, "File");
    row.set_value(listColumns->m_mode, fileInfo->get_attribute_uint32("unix::mode"));
    Glib::ustring user{fileInfo->get_attribute_string("owner::user")};      // numeric = get_attribute_uint32("unix::uid")};
    row.set_value(listColumns->m_user, user);
    Glib::ustring group{fileInfo->get_attribute_string("owner::group")};    // numeric = get_attribute_uint32("unix::gid");
    row.set_value(listColumns->m_group, group);
    Glib::DateTime modified = fileInfo->get_modification_date_time();
    row.set_value(listColumns->m_modified, modified);
    Glib::ustring contentType{fileInfo->get_attribute_string("standard::content-type")};
    row.set_value(listColumns->m_contentType, contentType);
    auto glibObj = fileInfo->get_attribute_object("standard::symbolic-icon");
    auto themedIcon = Glib::RefPtr<Gio::ThemedIcon>::cast_dynamic(glibObj);
    if (themedIcon) {
        auto theme = Gtk::IconTheme::get_default();
        for (Glib::ustring iconName : themedIcon->get_names()) {
            if (theme->has_icon(iconName)) {
                auto pixBuf = theme->load_icon(iconName, LOOKUP_ICON_SIZE);
                if (pixBuf) {
                    //std::cout << "Theme icon for " << iconName
                    //          << " w " << pixBuf->get_width()
                    //          << " h " << pixBuf->get_height() << std::endl;
                    row.set_value(listColumns->m_icon, pixBuf);
                }
                //else {
                //    std::cout << "Theme no icon builtin for " << iconName << std::endl;
                //}
                break;
            }
            //else {
            //    std::cout << "Theme no icon for " << iconName << std::endl;
            //}
        }
    }
    else {
        std::cout << "No icon for "
                  << fileInfo->get_display_name()
                  << " value " << fileInfo->get_attribute_as_string("standard::symbolic-icon") << std::endl;
    }

    row.set_value(listColumns->m_fileInfo, fileInfo);
    row.set_value(listColumns->m_file, file);
}

const char*
FileDataSource::getConfigGroup()
{
    return "FileData";
}

Glib::RefPtr<Gio::File>
FileDataSource::getFileName(const std::string& name)
{
    auto file = m_file->get_child(name);
    return file;
}