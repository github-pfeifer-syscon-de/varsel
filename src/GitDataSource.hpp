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



#include "DataSource.hpp"
#include "GitRepository.hpp"


class StatusConverter
: public psc::ui::CustomConverter<psc::git::FileStatus>
{
public:
    StatusConverter(Gtk::TreeModelColumn<psc::git::FileStatus>& col)
    : psc::ui::CustomConverter<psc::git::FileStatus>(col)
    {
        // Create an empty style context
        auto style_ctx = Gtk::StyleContext::create();
        // Create an empty widget path
        auto widget_path = Gtk::WidgetPath();
        // Specify the widget class type you want to get colors from
        widget_path.prepend_type(Gtk::Button::get_type());
        style_ctx->set_path(widget_path);
        // Print style context colors of widget class Gtk.Button

        m_fg_color = style_ctx->get_color(Gtk::StateFlags::STATE_FLAG_NORMAL);
        m_red = Gdk::RGBA("#f00");
        m_green = Gdk::RGBA("#0f0");
        m_gray = Gdk::RGBA("#888");
        m_turquis = Gdk::RGBA("#0fc");
    }
    virtual ~StatusConverter() = default;

    void convert(Gtk::CellRenderer* rend, const Gtk::TreeModel::iterator& iter) override
    {
        psc::git::FileStatus value{psc::git::FileStatus::None};
        iter->get_value(m_col.index(), value);
        auto textRend = static_cast<Gtk::CellRendererText*>(rend);
        auto txt = psc::git::DirStatus::getStatus(value);
        textRend->property_text() = txt;
        using enum psc::git::FileStatus;
        switch(value) {
        case None:
        case Ignore:
            textRend->property_foreground_rgba() = m_gray;
            break;
        case Modified:
        case Renamed:
        case TypeChange:
            textRend->property_foreground_rgba() = m_turquis;
            break;
        case New:
            textRend->property_foreground_rgba() = m_fg_color;
            break;
        case Deleted:
            textRend->property_foreground_rgba() = m_red;
            break;
        case Current:
            textRend->property_foreground_rgba() = m_green;
            break;
        }
    }
private:
    Gdk::RGBA m_fg_color;
    Gdk::RGBA m_red;
    Gdk::RGBA m_gray;
    Gdk::RGBA m_green;
    Gdk::RGBA m_turquis;
};

class GitListColumns
: public ListColumns
{
public:
    Gtk::TreeModelColumn<psc::git::FileStatus> m_workdirState;
    Gtk::TreeModelColumn<psc::git::FileStatus> m_indexState;
    GitListColumns()
    : ListColumns::ListColumns()
    {
        auto workStatusConv = std::make_shared<StatusConverter>(m_workdirState);
        add<psc::git::FileStatus>(_("Workdir Status"), workStatusConv, 1.0f);
        auto indexStatusConv = std::make_shared<StatusConverter>(m_indexState);
        add<psc::git::FileStatus>(_("Index Status"), indexStatusConv, 1.0f);
    }
};

class GitTreeNode
: public BaseTreeNode
{
public:
    GitTreeNode(const Glib::ustring& dir, unsigned long depth);
    virtual ~GitTreeNode() = default;

     Gtk::TreeModel::iterator appendList() override;
     Glib::RefPtr<Gtk::ListStore> getEntries() override;
     static std::shared_ptr<ListColumns> getListColumns();
private:
    static std::shared_ptr<GitListColumns> m_gitListColumns;
    Glib::RefPtr<Gtk::ListStore> m_entries;
};


class GitDataSource
: public DataSource
{
public:
    GitDataSource(const Glib::RefPtr<Gio::File>& dir, VarselApp* application);
    explicit GitDataSource(const GitDataSource& orig) = delete;
    virtual ~GitDataSource() = default;

    void update(
          const Glib::RefPtr<psc::ui::TreeNodeModel>& treeModel
        , ListListener* listListener) override;
    const char* getConfigGroup() override;
    Glib::RefPtr<Gio::File> getFileName(const std::string& name) override;
    std::shared_ptr<ListColumns> getListColumns() override;

private:
    Glib::RefPtr<Gio::File> m_dir;

};

