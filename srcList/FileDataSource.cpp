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

FileDataSource::FileDataSource(ListApp* application)
: DataSource::DataSource(application)
{
}

void
FileDataSource::update(
          const Glib::RefPtr<Gio::File>& dir
        , std::shared_ptr<psc::ui::TreeNode> treeItem
        , const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    Glib::RefPtr<Gio::FileEnumerator> enumerat;
    try {
        auto fileInfo = dir->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
        if (!treeItem) {
            treeItem = std::make_shared<FileTreeNode>(dir, fileInfo->get_display_name(), 0);
            treeModel->append(treeItem);
        }
        auto fileTreeItem = std::dynamic_pointer_cast<FileTreeNode>(treeItem);
        fileTreeItem->setQueried(true);

        auto cancellable = Gio::Cancellable::create();
        enumerat = dir->enumerate_children(
              cancellable
            , "*"
            , Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        while (true) {
            auto fileInfo = enumerat->next_file();
            if (!fileInfo) {
                break;
            }
            auto child = dir->get_child(fileInfo->get_name());
            // this gives the type linked
            auto childType = child->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            switch(fileInfo->get_file_type()) {
            case Gio::FileType::FILE_TYPE_DIRECTORY: {
                auto subTreeItem = std::make_shared<FileTreeNode>(child, fileInfo->get_display_name(), treeItem->getDepth() + 1);
                treeModel->append(treeItem, subTreeItem);
                }
                break;
            case Gio::FileType::FILE_TYPE_REGULAR:
            case Gio::FileType::FILE_TYPE_SYMBOLIC_LINK: {  // show links in list as there are various additional attributes
                auto iter = fileTreeItem->appendList();
                auto row = *iter;
                //std::cout << "file name " << name << " size " << fileInfo->get_size() << std::endl;
                auto listColumns = getListColumns();
                setFileValues(row, child, fileInfo, listColumns);
                }
                break;
            default:        // Ignore
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

Glib::ustring
FileDataSource::readableFileType(Gio::FileType fileType)
{
    Glib::ustring type;
    switch (fileType) {
    case Gio::FileType::FILE_TYPE_NOT_KNOWN:
        type = _("Unknown");
        break;
    case Gio::FileType::FILE_TYPE_REGULAR:
        type = _("File");
        break;
    case Gio::FileType::FILE_TYPE_DIRECTORY:
        type = _("Directory");
        break;
    case Gio::FileType::FILE_TYPE_SYMBOLIC_LINK:
        type = _("Symbolic link");
        break;
    case Gio::FileType::FILE_TYPE_SPECIAL:
        type = _("Special");
        break;
    case Gio::FileType::FILE_TYPE_SHORTCUT:
        type = _("Shortcut");
        break;
    case Gio::FileType::FILE_TYPE_MOUNTABLE:
        type = _("Mountable");
        break;
    }
    return type;
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
    auto linkedFileType = file->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    row.set_value(listColumns->m_type, readableFileType(linkedFileType));
    row.set_value(listColumns->m_mode, fileInfo->get_attribute_uint32(G_FILE_ATTRIBUTE_UNIX_MODE));
    Glib::ustring user{fileInfo->get_attribute_string(G_FILE_ATTRIBUTE_OWNER_USER)};      // numeric = get_attribute_uint32("unix::uid")};
    row.set_value(listColumns->m_user, user);
    Glib::ustring group{fileInfo->get_attribute_string(G_FILE_ATTRIBUTE_OWNER_GROUP)};    // numeric = get_attribute_uint32("unix::gid");
    row.set_value(listColumns->m_group, group);
    Glib::DateTime modified = fileInfo->get_modification_date_time();
    row.set_value(listColumns->m_modified, modified);
    Glib::ustring contentType{fileInfo->get_content_type()};
    row.set_value(listColumns->m_contentType, contentType);
    if (fileInfo->has_attribute(G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON)) {
        auto glibObj = fileInfo->get_attribute_object(G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON);
        row.set_value(listColumns->m_icon, glibObj);
    }
    if (fileInfo->has_attribute(G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET)) {
        Glib::ustring symLink = Glib::strescape(fileInfo->get_attribute_byte_string(G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET));
        row.set_value(listColumns->m_symLink, symLink);
    }

    row.set_value(listColumns->m_fileInfo, fileInfo);
    row.set_value(listColumns->m_file, file);
}

const char*
FileDataSource::getConfigGroup()
{
    return "FileData";
}

void
FileDataSource::progress(goffset current_num_bytes, goffset total_num_bytes)
{
    std::cout << "FileDataSource::progress "
              << current_num_bytes <<  "/" << total_num_bytes << std::endl;
}

void
FileDataSource::paste(const Glib::RefPtr<Gio::File>& dir, const std::vector<Glib::ustring>& uris, Gtk::Window* win)
{
    std::cout << "FileDataSource::paste " << uris.size() << std::endl;
    for (auto& uri : uris) {
        auto file = Gio::File::create_for_uri(uri);
        auto fileType = file->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
        if (file->query_exists()
         && fileType == Gio::FileType::FILE_TYPE_REGULAR) {
            auto localFile = dir->get_child(file->get_basename());
            if (localFile->query_exists()) {
                auto msg = Glib::ustring::sprintf(_("Overwrite %s"), localFile->get_basename());
                Gtk::MessageDialog msgDlg(*win, msg, false, Gtk::MessageType::MESSAGE_QUESTION, Gtk::ButtonsType::BUTTONS_YES_NO, true);
                auto ret = msgDlg.run();
                if (ret == Gtk::RESPONSE_NO) {
                    continue;
                }
            }
            auto cancel = Gio::Cancellable::create();
            if (!file->copy(localFile
                      , sigc::mem_fun(*this,&FileDataSource::progress)
                      , cancel
                      , Gio::FILE_COPY_OVERWRITE)) {
                auto msg = Glib::ustring::sprintf(_("Error writing %s!\nContinue?"), localFile->get_basename());
                Gtk::MessageDialog msgDlg(*win, msg, false, Gtk::MessageType::MESSAGE_QUESTION, Gtk::ButtonsType::BUTTONS_YES_NO, true);
                if (!msgDlg.run()) {
                    break;
                }
            }
        }
        else {
            std::cout << "Skipping " << file->get_basename()
                      << " exists " << std::boolalpha << file->query_exists()
                      << " type " << fileType
                      << std::endl;
        }
    }
}

void
FileDataSource::distribute(const std::vector<PtrEventItem>& items, Gtk::Menu* menu, Gtk::Window* win)
{
     m_application->getEventBus()->distribute(items, menu);
}