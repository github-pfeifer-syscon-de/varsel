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
#include <glibmm.h>

#include "DataModel.hpp"



namespace psc {
namespace ui {


//////////////// TreeNode

TreeNode::TreeNode(unsigned long depth)
: m_depth{depth}
{
    m_clds.reserve(8);
}

TreeNode::~TreeNode()
{
    // node might be "reachable by other references" so reset parent
    for (auto& cld : m_clds) {
        cld->m_parent = nullptr;
    }
}

Gtk::TreeModel::Path
TreeNode::getPath()
{
    Gtk::TreeModel::Path path;
    TreeNode* child_node = this;
    TreeNode* parent_node = m_parent;
    while (parent_node) {
//        bool found = false;
//        for (size_t i = 0; i < parent_node->m_clds.size(); ++i) {
//            auto& chld = parent_node->m_clds[i];
//            if (chld.get() == child_node) {
//                found = true;
//                path.push_front(i);      // push_back descends path, so use front here
//                break;
//            }
//        }
//        if (!found) {
//            auto msg = Glib::ustring::format("The requested item %x", std::hex, parent_node, std::dec, "  was not found!");
//            throw std::runtime_error(msg);
//        }
        path.push_front(child_node->getChildIdx());
        child_node = parent_node;
        parent_node = parent_node->m_parent;
    }
    if (path.size() != m_depth) {
        std::cout << "This seems wrong node at depth " << m_depth
                  << " got a path of " <<  path.size() << std::endl;
    }
    return path;
}

//void
//TreeNode::setParent(TreeNode* parent)
//{
//    m_parent = parent;
//    m_depth = parent->m_depth + 1;
//}

TreeNode*
TreeNode::getParent()
{
    return m_parent;
}

unsigned long
TreeNode::getDepth()
{
    return m_depth;
}

size_t
TreeNode::getChildIdx()
{
    return m_childIdx;
}

void
TreeNode::addChild(const std::shared_ptr<TreeNode>& item)
{
    #ifdef MODEL_DEBUG
    std::cout << "TreeNode::addChild "
            << " this " << std::hex << this
            << " depth " << m_depth
            << " item " << item.get() << std::dec << std::endl;
    #endif
    item->m_parent = this;
    item->m_depth = m_depth + 1;
    item->m_childIdx = m_clds.size();
    m_clds.push_back(item);
}

void
TreeNode::renumberChilds(size_t renumFrom)
{
    for (size_t i = renumFrom; i < m_clds.size(); ++i) {
        m_clds[i]->m_childIdx = i;
    }
}

std::shared_ptr<TreeNode>
TreeNode::getItem(std::size_t idx)
{
    return m_clds[idx];
}

std::size_t
TreeNode::getSize()
{
    return m_clds.size();
}

void
TreeNode::getValue(int column, Glib::ValueBase& value)
{
    std::cout << "TreeNode::getValue was left unimplemented!" << std::endl;
}

void
TreeNode::setValue(int column, const Glib::ValueBase& value)
{
    std::cout << "TreeNode::setValue was left unimplemented!" << std::endl;
}


void
TreeNode::on_remove()
{
}

void
TreeNode::remove(TreeNodeModel* model, int removeDepth)
{
    if (!m_parent) {
        std::cout << "Try to remove node without parent, either this node was removed before, or is the invisible root, both attempts are useless." << std::endl;
        return;
    }
    // remove any child node recursively
    for (auto iter = m_clds.rbegin(); iter != m_clds.rend(); ) {    // use reverse as this fits better for vector
        auto& chld = *iter;
        chld->remove(model, removeDepth+1);    // descent first
        auto path = chld->getPath();
        iter = decltype(iter)(m_clds.erase( std::next(iter).base() ));  // update structure
        model->memory_row_deleted(path);                                // notify
    }
    if (removeDepth == 0) {        // if we are back at calling level, remove this node, from its parent
        on_remove();
        auto iter = m_parent->m_clds.begin();
        iter += getChildIdx();
        auto path = getPath();
        m_parent->m_clds.erase(iter);   // detach, first link
        m_parent->renumberChilds(getChildIdx());
        //std::cout << "model remove this path " << path.to_string() << std::endl;
        model->memory_row_deleted(path);
        m_parent = nullptr;             // detach, second link
    }
}


//////////////// MemoryTreeModel

TreeNodeModel::TreeNodeModel(const std::shared_ptr<Gtk::TreeModel::ColumnRecord>& treeColumns)
: Glib::ObjectBase(typeid(TreeNodeModel)) // Register a custom GType.
, Glib::Object() // The custom GType is actually registered here.
, m_root{std::make_shared<TreeNode>(0)}
, m_columns{treeColumns}
{
}

TreeNodeModel::~TreeNodeModel()
{
    // empty model, as Gtk may complain if not empty on destruction?
    //   seems less desirable in this case ...(as model is already going down)
    //m_root->remove(this);
}

Glib::RefPtr<TreeNodeModel>
TreeNodeModel::create(const std::shared_ptr<Gtk::TreeModel::ColumnRecord>& treeColumns)
{
    // this is the version from Glib2.8x (use with gtkmm4)
    //return Glib::make_refptr_for_instance(new MemoryTreeModel());
    // so we stick to the old variant
    return Glib::RefPtr<TreeNodeModel>(new TreeNodeModel(treeColumns));
}

void
TreeNodeModel::memory_row_inserted(const std::shared_ptr<TreeNode>& item, bool incrementStamp)
{
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::memory_row_inserted" << std::endl;
#   endif
    // Inform TreeView that a new node has been inserted.
    if (incrementStamp) {
        ++m_stamp;
    }
    iterator iter;
    iter.set_stamp(m_stamp);
    iter.gobj()->user_data = reinterpret_cast<TreeNode*>(item.get());    // row index
    auto path = get_path(iter);
    //std::cout << "TreeNodeModel::memory_row_inserted path " << path.to_string()
    //          << " item " << std::hex << item.get() << std::dec << std::endl;
    row_inserted(path, iter);
    // if the item happen to have child nodes add them to model as well
    for (size_t i = 0; i < item->getSize(); ++i) {
        auto chld = item->getItem(i);
        memory_row_inserted(chld, false);      // handels recursion, avoid using a new stamp for each insertion
    }
}

void
TreeNodeModel::memory_row_deleted(const Gtk::TreeModel::Path& path)
{
    //std::cout << "TreeNodeModel::memory_row_deleted"
    //        << " path " << path.to_string() << std::endl;
    // Inform TreeView that a node has been deleted,
    //    dependent deletion is handled by item
    ++m_stamp;
    row_deleted(path);
}

Gtk::TreeModel::iterator
TreeNodeModel::append(const std::shared_ptr<TreeNode>& item)
{
    add(nullptr, item);
    // the ++m_stamp will be done in response to insertion
    Gtk::TreeModel::iterator iter_new(this);
    iter_new.set_stamp(m_stamp);
    iter_new.gobj()->user_data = reinterpret_cast<void*>(item.get());
    return iter_new;
}

Gtk::TreeModel::iterator
TreeNodeModel::append(const std::shared_ptr<TreeNode>& parentNode, const std::shared_ptr<TreeNode>& childNode)
{
    //const auto parentItem = reinterpret_cast<TreeNode*>(node.gobj()->user_data);
    //std::cout << "TreeNodeModel::append parentItem " << std::hex << parentItem << std::dec << std::endl;
    //auto childNode = std::make_shared<TreeNode>();
    add(parentNode.get(), childNode);
    // the ++m_stamp will be done in response to insertion
    Gtk::TreeModel::iterator iter_new(this);
    iter_new.set_stamp(m_stamp);
    iter_new.gobj()->user_data = reinterpret_cast<void*>(childNode.get());
    return iter_new;
}

TreeNode*
TreeNodeModel::get_node(const const_iterator& iter) const
{
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
    if (!is_valid(iter) || !item) {
        return nullptr;
    }
    return item;
}


void
TreeNodeModel::remove(const std::shared_ptr<TreeNode>& item)
{
    item->remove(this);
}

Gtk::TreeModelFlags
TreeNodeModel::get_flags_vfunc() const
{
   return Gtk::TreeModelFlags(0);
}

int
TreeNodeModel::get_n_columns_vfunc() const
{
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::get_n_columns_vfunc"
              << " n " << (m_columns ? m_columns->size() : -1) << std::endl;
#   endif
   return m_columns->size();
}

void
TreeNodeModel::get_value_vfunc(const const_iterator& iter, int column, Glib::ValueBase& value) const
{
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::get_value_vfunc"
              << " item " << std::hex << item << std::dec << " col " << column << std::endl;
#   endif
    if (!is_valid(iter) || !item) {
        return;
    }
    item->getValue(column, value);
}

void
TreeNodeModel::set_value_impl(const iterator& row, int column, const Glib::ValueBase& value)
{
    const auto item = reinterpret_cast<TreeNode*> (row.gobj()->user_data);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::set_value_impl"
              << " item " << std::hex << item << std::dec << std::endl;
#   endif
    if (!is_valid(row) || !item) {
        return;
    }
    item->setValue(column, value);
    row_changed(item->getPath(), row);
}


// What is the next node from this one?
bool
TreeNodeModel::iter_next_vfunc(const iterator& iter, iterator& iter_next) const
{
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
    TreeNode* parent = item->getParent();
    if (!is_valid(iter) || !parent) {
#       ifdef MODEL_DEBUG
        std::cout << "No parent (or invalid iter) " << std::hex << parent << std::dec << std::endl;
#       endif
        iter_next = iterator(); // There is no next row.
        return false;
    }
    size_t idx = item->getChildIdx();
    if (idx+1 >= parent->getSize()) {
#       ifdef MODEL_DEBUG
        std::cout << "TreeNodeModel::iter_next_vfunc idx " << idx+1 << " exceeds " << parent->getSize() << " (this is expected)." << std::endl;
#       endif
        iter_next = iterator(); // The model is testing the limits by this
        return false;
    }
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_next_vfunc " << std::hex << parent << " idx " << idx << std::endl;
#   endif
    iter_next.set_stamp(m_stamp);
    iter_next.gobj()->user_data = reinterpret_cast<void*>(parent->getItem(idx+1).get());
    return true;
}

// Get the first child of this parent node
bool
TreeNodeModel::iter_children_vfunc(const iterator& parent, iterator& iter) const
{
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_children_vfunc "  << std::endl;
#   endif
    return iter_nth_child_vfunc(parent, 0, iter);
}

bool
TreeNodeModel::iter_has_child_vfunc(const const_iterator& iter) const
{
#   ifdef MODEL_DEBUG
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
    std::cout << "TreeNodeModel::iter_has_child_vfunc " << std::hex << item << std::dec << std::endl;
#   endif
    return iter_n_children_vfunc(iter) > 0;
}

int
TreeNodeModel::iter_n_children_vfunc(const const_iterator& iter) const
{
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_n_children_vfunc "
             << std::hex << item << std::dec
             << " n " << (item != nullptr ? item->getSize() : -1) << std::endl;
#   endif
    if (!is_valid(iter)
     || !item) {// invalid or uninitialized node
        return 0;
    }
    return item->getSize(); // no nodes
}

int
TreeNodeModel::iter_n_root_children_vfunc() const
{
    size_t size = m_root->getSize();
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_n_root_children_vfunc size " << size << std::endl;
#   endif
    return size; // number of rows
}

// Get the nth child of this parent node
bool
TreeNodeModel::iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const
{
    const auto parentItem = reinterpret_cast<TreeNode*>(parent.gobj()->user_data);
    if (!is_valid(parent)
    || static_cast<size_t>(n) >= parentItem->getSize()) {
#       ifdef MODEL_DEBUG
        std::cout << "TreeNodeModel::iter_nth_child_vfunc requesting " << n << " from " << parentItem->getSize() << std::endl;
#       endif
        iter = iterator();
        return false; // There are no children.
    }
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_nth_child_vfunc  "
              << std::hex << parentItem << std::dec
              << " n " << n << std::endl;
#   endif
    iter.set_stamp(m_stamp);
    iter.gobj()->user_data = reinterpret_cast<void*>(parentItem->getItem(n).get());
    return true;
}

// Get the nth row
bool
TreeNodeModel::iter_nth_root_child_vfunc(int n, iterator& iter) const
{
    if (static_cast<size_t>(n) >= m_root->getSize()) {
#       ifdef MODEL_DEBUG
        std::cout << "TreeNodeModel::iter_nth_root_child_vfunc requesting " << n << " from " << m_root->getSize() << std::endl;
#       endif
        iter = iterator();
        return false; // No such row
    }
    auto item = m_root->getItem(n);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_nth_root_child_vfunc  "
              << " n " << n
              << " item " << std::hex <<  item.get()<< std::dec << std::endl;
#   endif
    iter.set_stamp(m_stamp);
    iter.gobj()->user_data = reinterpret_cast<void*>(item.get());
    return true; // n < available rows
}

// Get the parent for this child
bool
TreeNodeModel::iter_parent_vfunc(const iterator& child, iterator& iter) const
{
    const auto item = reinterpret_cast<TreeNode*>(child.gobj()->user_data);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::iter_parent_vfunc  "
              << " item " << std::hex << item
              << " parent " << (item != nullptr ? item->getParent() : nullptr)  << std::dec
              << " depth " << (item != nullptr ? item->getDepth() : -1)  << std::endl;
#   endif
    if (!item
     || !item->getParent()
     || item->getParent()->getDepth() == 0) {    // do not reveal "hidden" root
        iter = iterator();
        return false; // There is no parent.
    }
    iter.set_stamp(m_stamp);
    iter.gobj()->user_data = reinterpret_cast<void*>(item->getParent());
    return true;
}

// Define a node
bool
TreeNodeModel::get_iter_vfunc(const Path& path, iterator& iter) const
{
    if (path.empty()) {
#       ifdef MODEL_DEBUG
        std::cout << "TreeNodeModel::get_iter_vfunc empty path "  << std::endl;
#       endif
        iter = iterator();
        return false;
    }
    auto item = m_root;
    for (size_t i = 0; i < path.size(); ++i) {
        size_t idx = path[i];
        if (idx >= item->getSize()) {
            std::cout << "searching idx " << idx << " from " << item->getSize() << " failed" << std::endl;
            iter = iterator();
            return false;
        }
        item = item->getItem(idx);
    }
    iter.set_stamp(m_stamp);
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::get_iter_vfunc "
              << " path " << path.to_string()
              << " item " << std::hex <<  item.get() << std::dec << std::endl;
#   endif
    iter.gobj()->user_data = reinterpret_cast<void*>(item.get()); // store instances, identifies node best
    return true;
}


void
TreeNodeModel::add(TreeNode* parent, const std::shared_ptr<TreeNode>& item)
{
    if (parent) {
        parent->addChild(item);
    }
    else {
        if (!m_root) {
            throw std::runtime_error("The expected root was not set!"); // don't cause indefinite recursion
        }
        add(m_root.get(), item);
        return;     // do not fire event a second time
    }
    memory_row_inserted(item);
}

Gtk::TreeModel::Path
TreeNodeModel::get_path_vfunc(const const_iterator& iter) const
{
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
    Path path = item->getPath();
#   ifdef MODEL_DEBUG
    std::cout << "TreeNodeModel::get_path_vfunc "
              << " item " << std::hex << item << std::dec
              << " path " << path.to_string()
              << " stamp " << m_stamp << std::endl;
#   endif
    return path;
}

GType TreeNodeModel::get_column_type_vfunc(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_columns->size()) {
        return 0;
    }
    return m_columns->types()[index];
}

bool
TreeNodeModel::is_valid(const const_iterator& iter) const
{
    // Anything that modifies the model's structure should change the model's stamp,
    // so that old iterators are ignored.
    bool valid = (m_stamp == iter.get_stamp());
#   ifdef MODEL_DEBUG
    const auto item = reinterpret_cast<TreeNode*>(iter.gobj()->user_data);
    std::cout << "TreeNodeModel::types "
              << " item " << std::hex << item << std::dec
              << " valid " << std::boolalpha << valid << std::endl;
#   endif
    return valid;
}

} /* namespace ui */
} /* namespace psc */
