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
#include <StringUtils.hpp>

#include "GitRepository.hpp"
#include "GitDataSource.hpp"
#include "FileDataSource.hpp"


std::shared_ptr<GitListColumns> GitTreeNode::m_gitListColumns;

GitTreeNode::GitTreeNode(const Glib::ustring& dir, unsigned long depth)
: BaseTreeNode::BaseTreeNode(dir, depth)
, m_entries{Gtk::ListStore::create(*getListColumns())}
{
}

std::shared_ptr<ListColumns>
GitTreeNode::getListColumns()
{
    if (!m_gitListColumns) {
        m_gitListColumns = std::make_shared<GitListColumns>();
    }
    return m_gitListColumns;
}

Gtk::TreeModel::iterator
GitTreeNode::appendList()
{
    return m_entries->append();
}

Glib::RefPtr<Gtk::ListStore>
GitTreeNode::getEntries()
{
    return m_entries;
}

GitDataSource::GitDataSource(const Glib::RefPtr<Gio::File>& dir, ListApp* application)
: DataSource(application)
, m_dir{dir}
{
}

bool
GitDataSource::isDisplayable(const Glib::RefPtr<Gio::File>& file)
{
    if (!file->query_exists()) {
        return false;
    }
    // QueryInfoFlags->None will pass thru type for symlinks
    auto fileType = file->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
    return (fileType == Gio::FileType::FILE_TYPE_REGULAR);
}

void
GitDataSource::update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener)
{
    try {
        auto treeNode = std::make_shared<GitTreeNode>(".", 1);
        treeModel->append(treeNode);
        psc::git::Repository repository(m_dir->get_path());
        auto status = repository.getStatus();
        for (auto iter = status.begin(); iter != status.end(); ++iter) {
            std::string name;
            if (!iter->getWorkdir().getNewPath().empty()) {
                //if (!iter->getWorkdir().getOldPath().empty()) {
                //    name += iter->getWorkdir().getOldPath() + " -> ";
                //}
                name = iter->getWorkdir().getNewPath();
            }
            else {
                name = iter->getWorkdir().getOldPath();
            }
            if (!name.empty()) {
                //std::cout << "GitDataSource::update got " << name << std::endl;
                auto node = treeNode;
                std::vector<Glib::ustring> parts;
                parts.reserve(8);
                StringUtils::split(name, '/', parts);
                for (size_t i = 0; i < parts.size() - 1; ++i) {
                    auto part = parts[i];
                    auto next = std::dynamic_pointer_cast<GitTreeNode>(node->findNode(part));
                    if (!next) {
                        next = std::make_shared<GitTreeNode>(part, node->getDepth() + 1);
                        node->addChild(next);
                    }
                    node = next;
                }

                auto file = getFileName(name);
                if (isDisplayable(file) ) {
                    auto list = node->appendList();
                    auto row = *list;
                    auto gitListColumns = std::dynamic_pointer_cast<GitListColumns>(getListColumns());
                    row.set_value<psc::git::FileStatus>(gitListColumns->m_workdirState, iter->getWorkdir().getStatus());
                    row.set_value<psc::git::FileStatus>(gitListColumns->m_indexState, iter->getIndex().getStatus());

                    auto info = file->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
                    FileDataSource::setFileValues(row, file, info, gitListColumns);
                }
                else {
                    std::cout << "Skippred " << name << " not a regular file." << std::endl;
                }
            }
        }
    }
    catch (const psc::git::GitException& ex) {
        std::cout << "Error " << ex.what() << " querying repos " << m_dir->get_path() << std::endl;
    }
}

const char*
GitDataSource::getConfigGroup()
{
    return "GitData";
}

Glib::RefPtr<Gio::File>
GitDataSource::getFileName(const std::string& name)
{
    auto file = m_dir->get_child(name);
    return file;
}

std::shared_ptr<ListColumns>
GitDataSource::getListColumns()
{
    return GitTreeNode::getListColumns();
}