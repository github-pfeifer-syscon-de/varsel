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

#pragma once

#include <gtkmm.h>
#include <psc_i18n.hpp>
#include <KeyfileTableManager.hpp>
#include <TreeNodeModel.hpp>

#include "EventBus.hpp"
#include "ListColumns.hpp"

class TreeNodeModel;
class ListApp;


class BaseTreeNode
: public psc::ui::TreeNode
{
public:
    BaseTreeNode(const Glib::ustring& dir, unsigned long depth);
    virtual ~BaseTreeNode() = default;
    Glib::ustring getDir();
    virtual Glib::RefPtr<Gtk::ListStore> getEntries() = 0;
    virtual Gtk::TreeModel::iterator appendList() = 0;
    void getValue(int column, Glib::ValueBase& value) override;
    void setValue(int column, const Glib::ValueBase& value) override;

    void
    on_remove() override
    {
        auto parentBaseNode = dynamic_cast<BaseTreeNode*>(m_parent);
        if (parentBaseNode) {
            auto iter = parentBaseNode->m_nodes.find(getDir());
            if (iter != parentBaseNode->m_nodes.end()) {
                parentBaseNode->m_nodes.erase(iter);
            }
        }
    }


    void
    addChild(const std::shared_ptr<TreeNode>& item) override
    {
        psc::ui::TreeNode::addChild(item);
        auto baseNode = std::dynamic_pointer_cast<BaseTreeNode>(item);
        if (baseNode) {
            m_nodes.insert(std::pair(baseNode->getDir(), baseNode));
        }
    }

    std::shared_ptr<BaseTreeNode>
    findNode(const Glib::ustring& name)
    {
        std::shared_ptr<BaseTreeNode> node;
        auto entry = m_nodes.find(name);
        if (entry != m_nodes.end()) {
            node = (*entry).second;
        }
//        for (auto iter = m_clds.begin(); iter != m_clds.end(); ++iter) {
//            auto& chl = *iter;
//            auto chld = std::dynamic_pointer_cast<BaseTreeNode>(chl);
//            if (chld
//             && chld->getDir() == name) {
//                node = chld;
//                break;
//            }
//            //else {
//            //    std::cout << "Unexpectd tree node " << std::endl;
//            //}
//        }
        return node;
    }
protected:
    std::map<Glib::ustring, std::shared_ptr<BaseTreeNode>> m_nodes;
    Glib::ustring m_dir;
};

enum class Severity
{
    Info,
    Warning,
    Error
};

class ListListener
{
public:
    virtual ~ListListener() = default;

    virtual void nodeAdded(const std::shared_ptr<BaseTreeNode>& baseTreeNode) = 0;
    virtual void listDone(Severity severity, const Glib::ustring& msg) = 0;
protected:
    ListListener() = default;
};

class FileTreeNode
: public BaseTreeNode
{
public:
    FileTreeNode(const Glib::ustring& dir, unsigned long depth);
    virtual ~FileTreeNode() = default;

     Gtk::TreeModel::iterator appendList() override;
     Glib::RefPtr<Gtk::ListStore> getEntries() override;
     static std::shared_ptr<ListColumns> getListColumns();
private:
    static std::shared_ptr<ListColumns> m_listColumns;
    Glib::RefPtr<Gtk::ListStore> m_entries;
};

class TreeColumns
: public Gtk::TreeModel::ColumnRecord
{
public:
    Gtk::TreeModelColumn<Glib::ustring> m_name;
    TreeColumns()
    {
        add(m_name);
    }
};


class FileTreeModel
: public psc::ui::TreeNodeModel
{
public:
    static Glib::RefPtr<FileTreeModel> create();
    virtual ~FileTreeModel() = default;

    static std::shared_ptr<TreeColumns> getTreeColumns();
protected:
    FileTreeModel(const std::shared_ptr<TreeColumns>& treeColumns);

    static std::shared_ptr<TreeColumns> m_treeColumns;
};


class EventNotifyContext
{
public:
    virtual void checkAfterSend(const std::shared_ptr<BusEvent>& event) = 0;
};


class DataAction
{
public:
    DataAction(const char* label, const char* name);
    virtual ~DataAction() = default;

    virtual void setContext(const std::vector<Glib::RefPtr<Gio::File>>& files) = 0;
    virtual void setEventNotifyContext(EventNotifyContext* eventNotifyContext) = 0;
    virtual bool isAvail() = 0;
    virtual void execute(const Glib::VariantBase& val) = 0;

    std::string getLabel();
    std::string getName();
    Glib::RefPtr<Gio::Action> getAction();
protected:
private:
    std::string m_label;
    std::string m_name;
    Glib::RefPtr<Gio::SimpleAction> m_action;
};

using ptrDataAction = std::shared_ptr<DataAction>;
class ListApp;

class OpenDataAction
: public DataAction
{
public:
    OpenDataAction(ListApp* application);
    virtual ~OpenDataAction() = default;

    bool isAvail() override;
    void execute(const Glib::VariantBase& val) override;
    void setContext(const std::vector<Glib::RefPtr<Gio::File>>& files) override;
    void setEventNotifyContext(EventNotifyContext* eventNotifyContext) override;
protected:
    std::shared_ptr<OpenEvent> m_openEvent;
private:
    ListApp* m_application;
    EventNotifyContext* m_eventNotifyContext{nullptr};
};


class DataSource
{
public:
    DataSource(ListApp* application);
    explicit DataSource(const DataSource& orig) = delete;
    virtual ~DataSource() = default;

    virtual void update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener) = 0;
    virtual Glib::RefPtr<psc::ui::TreeNodeModel> createTree();
    virtual const char* getConfigGroup() = 0;
    virtual Glib::RefPtr<Gio::File> getFileName(const std::string& name) = 0;
    virtual std::shared_ptr<ListColumns> getListColumns();
    virtual void paste(const std::vector<Glib::ustring>& uris, Gtk::Window* win) = 0;
    void addActions(std::vector<ptrDataAction>& actions);

    std::shared_ptr<TreeColumns> m_treeColumns;
    ListApp* m_application;
protected:
private:

};

