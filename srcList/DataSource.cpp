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
#include <psc_i18n.hpp>

#include "VarselList.hpp"
#include "DataSource.hpp"
#include "ListApp.hpp"

std::shared_ptr<ListColumns> FileTreeNode::m_listColumns;

BaseTreeNode::BaseTreeNode(const Glib::ustring& dir, unsigned long depth)
: psc::ui::TreeNode::TreeNode(depth)
, m_dir{dir}
{
    //std::cout << "BaseTreeNode::BaseTreeNode " << m_dir << std::endl;
}

Glib::ustring
BaseTreeNode::getDir()
{
    return m_dir;
}

void
BaseTreeNode::getValue(int column, Glib::ValueBase& value)
{
    using StringColumn = Gtk::TreeModelColumn<Glib::ustring>;
    switch (column) {
    case 0: // make this depend on real value
    {
        StringColumn::ValueType sValue;
        sValue.init(StringColumn::ValueType::value_type());
        sValue.set(m_dir);
        value.init(sValue.value_type());
        value = sValue;
        break;
    }
    }
}

void
BaseTreeNode::setValue(int column, const Glib::ValueBase& value)
{
    switch (column) {
    case 0: // make this depend...
    {
        //const char* cstr = g_value_get_string(value.gobj());    // how do this by glibmm?
        Glib::Value<std::string> svalue;
        svalue.init(value.gobj());
        m_dir = svalue.get();
        break;

    }
    }
}

FileTreeNode::FileTreeNode(const Glib::ustring& dir, unsigned long depth)
: BaseTreeNode::BaseTreeNode(dir, depth)
, m_entries{Gtk::ListStore::create(*getListColumns())}
{

}

std::shared_ptr<ListColumns>
FileTreeNode::getListColumns()
{
    if (!m_listColumns) {
        m_listColumns = std::make_shared<ListColumns>();
    }
    return m_listColumns;
}

Gtk::TreeModel::iterator
FileTreeNode::appendList()
{
    return m_entries->append();
}


Glib::RefPtr<Gtk::ListStore>
FileTreeNode::getEntries()
{
    return m_entries;
}

FileTreeModel::FileTreeModel(const std::shared_ptr<TreeColumns>& treeColumns)
: Glib::ObjectBase(typeid(FileTreeModel)) // Register a custom GType.
, psc::ui::TreeNodeModel::TreeNodeModel(treeColumns)
{

}

std::shared_ptr<TreeColumns> FileTreeModel::m_treeColumns;

Glib::RefPtr<FileTreeModel>
FileTreeModel::create()
{
    // this is the version from Glib2.8x (use with gtkmm4)
    //return Glib::make_refptr_for_instance(new MemoryTreeModel());
    // so we stick to the old variant
    return Glib::RefPtr<FileTreeModel>(new FileTreeModel(getTreeColumns()));
}

std::shared_ptr<TreeColumns>
FileTreeModel::getTreeColumns()
{
    if (!m_treeColumns) {
        m_treeColumns = std::make_shared<TreeColumns>();
    }
    return m_treeColumns;
}


DataSource::DataSource(ListApp* application)
: m_treeColumns{FileTreeModel::getTreeColumns()}
, m_application{application}
{
}

Glib::RefPtr<psc::ui::TreeNodeModel>
DataSource::createTree()
{
    return psc::ui::TreeNodeModel::create(m_treeColumns);
}

void
DataSource::open(std::vector<Glib::RefPtr<Gio::File>>& files)
{
    std::cout << "DataSource::open unexpected!!!" << std::endl;
    //m_application->getEventBus()->send(m_openEvent);
    //if (m_eventNotifyContext) {
    //    m_eventNotifyContext->checkAfterSend(m_openEvent);
    //}
}

std::shared_ptr<ListColumns>
DataSource::getListColumns()
{
    return FileTreeNode::getListColumns();
}