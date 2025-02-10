/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4 -*-  */
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
#include <vector>
#include <utility>
#include <memory>

namespace psc {
namespace ui {

class TreeNodeModel;

class TreeNode
{
public:
    // "invisible" root will use depth = 0
    TreeNode(unsigned long depth = 1);
    virtual ~TreeNode();
    // remove a node from the model
    void remove(TreeNodeModel* model, int removeDepth = 0);
    virtual void on_remove();

    Gtk::TreeModel::Path getPath();
    std::shared_ptr<TreeNode> getItem(std::size_t idx);
    std::size_t getSize();
    //void setParent(TreeNode* parent);
    TreeNode* getParent();
    unsigned long getDepth();
    size_t getChildIdx();
    // this just cares about the nodes, use model::append to keep the model up to date
    virtual void addChild(const std::shared_ptr<TreeNode>& item);
    void renumberChilds(size_t renumFrom = 0);
    // override these to make it work
    virtual void getValue(int column, Glib::ValueBase& value);
    virtual void setValue(int column, const Glib::ValueBase& value);
protected:
    TreeNode* m_parent{nullptr};
    unsigned long m_depth;      // the visible root instances use a depth of 1, any node beyond parent+1
    size_t m_childIdx{0};       // to simplify access keep a index (no need to search nodes from list)
    std::vector<std::shared_ptr<TreeNode>> m_clds;
};

#undef MODEL_DEBUG

class TreeNodeModel
: public Gtk::TreeModel
, public Glib::Object
{
public:
    virtual ~TreeNodeModel();

    static Glib::RefPtr<TreeNodeModel> create(const std::shared_ptr<Gtk::TreeModel::ColumnRecord>& treeColumns);

    void memory_row_inserted(const std::shared_ptr<TreeNode>& item, bool incrementStamp = true);
    void memory_row_deleted(const Gtk::TreeModel::Path& path);
    // add a top level node
    iterator append(const std::shared_ptr<TreeNode>& item);
    // this also cares about linking the nodes
    iterator append(const std::shared_ptr<TreeNode>& parentNode, const std::shared_ptr<TreeNode>& childNode);
    void remove(const std::shared_ptr<TreeNode>& item);
    TreeNode* get_node(const const_iterator& iter) const;

protected:
    TreeNodeModel(const std::shared_ptr<Gtk::TreeModel::ColumnRecord>& treeColumns);
    // Overrides:
    Gtk::TreeModelFlags get_flags_vfunc() const override;
    int get_n_columns_vfunc() const override;
    GType get_column_type_vfunc(int index) const override;
    void get_value_vfunc(const const_iterator& iter, int column, Glib::ValueBase& value) const override;
    void set_value_impl(const iterator &row, int column, const Glib::ValueBase & value) override;
    bool iter_next_vfunc(const iterator& iter, iterator& iter_next) const override;
    bool iter_children_vfunc(const iterator& parent, iterator& iter) const override;
    bool iter_has_child_vfunc(const const_iterator& iter) const override;
    int iter_n_children_vfunc(const const_iterator& iter) const override;
    int iter_n_root_children_vfunc() const override;
    bool iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const override;
    bool iter_nth_root_child_vfunc(int n, iterator& iter) const override;
    bool iter_parent_vfunc(const iterator& child, iterator& iter) const override;
    Path get_path_vfunc(const const_iterator& iter) const override;
    bool get_iter_vfunc(const Path& path, iterator& iter) const override;

    void add(TreeNode* parent, const std::shared_ptr<TreeNode>& item);
    // These vfuncs are optional to implement.
    // void ref_node_vfunc(const iterator& iter) const override;
    // void unref_node_vfunc(const iterator& iter) const override;

    bool is_valid(const const_iterator& iter) const;
private:


    // The gate for the model to the information to be shown.
    // The MemoryTreeModel does not own the data.
    //Memory* m_Memory = nullptr;
    // When the model's stamp and the TreeIter's stamp are equal, the TreeIter is valid.
    int m_stamp = 0;
    std::shared_ptr<TreeNode> m_root;
    std::shared_ptr<Gtk::TreeModel::ColumnRecord> m_columns;
};

} /* namespace ui */
} /* namespace psc */
